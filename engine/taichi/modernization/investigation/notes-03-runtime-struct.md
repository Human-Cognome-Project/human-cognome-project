# Notes 03 — runtime and struct layer

Contemporaneous working notes. Appended as work proceeds. Agent: explore-03.
Territory: `taichi/runtime/`, `taichi/struct/`, `taichi/program/`.

## 1. Starting point

Read `modernization/PROJECT-PLAN.md` in full. Governing items for me:
6.1 (parameterise `taichi_max_num_snodes`), 6.2 (64-bit addressing), 5.2
(target tiers, baseline is a 2 GB GTX 750), 10 (standing instructions).

Read `taichi/inc/constants.h`. Confirmed verbatim:

- line 5  `constexpr int taichi_max_num_indices = 12;`
- line 12 `constexpr int taichi_max_num_snodes = 1024;`
- line 13 `constexpr int kMaxNumSnodeTreesLlvm = 512;`

Note immediately: the brief names 6.1 and 6.2 around `taichi_max_num_snodes`
and `taichi_max_num_indices`, but line 13 `kMaxNumSnodeTreesLlvm = 512` is a
second SNode-scaled ceiling that the brief does not mention. It is in my
territory (it sizes runtime tree tables). Flagging now, will chase.

Every constant in that file is `int` or `std::size_t`. The two SNode-related
ones are `int`.

## 2. `LLVMRuntime` struct — the SNode-scaled arrays

`taichi/runtime/llvm/runtime_module/runtime.cpp`. File is 1984 lines. Header
comment lines 1-2: "This file will only be compiled into llvm bitcode by
clang." So this is device-side runtime source, compiled to bitcode and linked
into every kernel module. Its struct layout is mirrored on the host side.

`Ptr` is `uint8 *` (line 114), so 8 bytes on all targets in scope.

Confirmed the brief's line numbers, and found two more arrays it does not
mention, sized by `kMaxNumSnodeTreesLlvm`:

- 562 `Ptr roots[kMaxNumSnodeTreesLlvm];`
- 563 `size_t root_mem_sizes[kMaxNumSnodeTreesLlvm];`
- 567 `ListManager *element_lists[taichi_max_num_snodes];`
- 568 `NodeManager *node_allocators[taichi_max_num_snodes];`
- 569 `Ptr ambient_elements[taichi_max_num_snodes];`

So the brief's "three static arrays" is right for `taichi_max_num_snodes`, but
the runtime struct carries five SNode/tree-scaled arrays in total.

## 3. Surprise: the real footprint is not the pointer arrays

`ListManager` at runtime.cpp:424-509 begins:

```
struct ListManager {
  static constexpr std::size_t max_num_chunks = 128 * 1024;
  Ptr chunks[max_num_chunks];
```

That is 131072 * 8 = **1 MiB of pointer table inside every single
ListManager**, and one ListManager is created per SNode with an element list
(runtime.cpp:1005), plus three more per `NodeManager` (free_list,
recycled_list, data_list — runtime.cpp:640 onward). So the per-SNode cost is
dominated by ListManager instances, not by the 8-byte slots in `LLVMRuntime`.

This is a genuinely different order of magnitude from what section 6.1
describes as "a footprint". Must quantify precisely.

Also at runtime.cpp:423, immediately above the struct, upstream's own comment:

```
// TODO: there are many i32 types in this class, which may be an issue if there
// are >= 2 ** 31 elements.
```

That is the 6.2 width problem, acknowledged in-tree.

## 4. Footprint arithmetic — verified with the compiler

I reproduced `LLVMRuntime`, `ListManager` and `NodeManager` field-for-field
from runtime.cpp into a throwaway translation unit that `#include`s the real
`taichi/inc/constants.h`, and forced the compiler to print the sizes via an
incomplete-template-argument error (`g++ -fsyntax-only`, nothing written to
disk). Results, which match my hand arithmetic exactly:

| Type | sizeof (bytes) |
|---|---|
| `LLVMRuntime` | 35256 |
| `ListManager` | 1048616 |
| `NodeManager` | 56 |

Offsets checked: `element_lists` at 8296, `temporaries` at 32872. So the three
`taichi_max_num_snodes` arrays occupy bytes 8296..32871, i.e. 24576 bytes
contiguous.

Breakdown of the 35256:

```
3 * 1024 * 8  = 24576   element_lists + node_allocators + ambient_elements
2 *  512 * 8  =  8192   roots + root_mem_sizes
                 2048   error_message_template
                  256   error_message_arguments
                  184   everything else (chunks, fn ptrs, locks, counters)
                -----
                35256
```

So **69.7%** of `LLVMRuntime` is the `taichi_max_num_snodes` arrays and
**23.2%** is the `kMaxNumSnodeTreesLlvm` arrays. 92.9% combined.

Where this cost is actually paid: `runtime_get_memory_requirements`
(runtime.cpp:889-905) adds `taichi::iroundup(i64(sizeof(LLVMRuntime)),
taichi_page_size)` to the device preallocation when a preallocated buffer is
used. 35256 rounded up to 4096 = **36864 bytes**. `runtime_initialize`
(runtime.cpp:908-958) then places the runtime object at the head of the
preallocated buffer and bumps the head by the same rounded amount
(runtime.cpp:927-930).

Scaling rule for 6.1, on the struct alone: **+24 bytes of `LLVMRuntime` per
additional SNode slot** (three 8-byte arrays). 4096 slots would be 98304 bytes
of arrays, sizeof ≈ 108984, rounded to 110592. 65536 slots would be 1572864
bytes of arrays, sizeof ≈ 1583544. Still small next to the point below.

## 5. The dominant per-SNode cost is `ListManager`, not the struct

`ListManager::chunks` is `Ptr chunks[128 * 1024]` (runtime.cpp:425-426), a flat
1 MiB pointer table, statically sized, allocated whether or not the chunks are
touched. `sizeof(ListManager)` = 1048616.

Per SNode that actually gets initialised in a non-`all_dense` tree
(runtime.cpp:1002-1008), one ListManager is allocated:

```
runtime->element_lists[i] =
    runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
```

`create<T>` (runtime.cpp:605-611) calls `allocate_aligned(runtime_memory_chunk,
sizeof(T), 4096, request=true)`.

Additionally, each pointer/dynamic/hash SNode gets a `NodeManager` via
`runtime_NodeAllocator_initialize` (runtime.cpp:1026-1031), and `NodeManager`'s
constructor (runtime.cpp:643-663) creates **three** more ListManagers
(free_list, recycled_list, data_list).

So the real device-memory cost per SNode is bounded below by:

```
element list                       1 * 1048616  =  1048616 B  ( ~1.00 MiB )
+ NodeManager (if allocating node) 1 *      56
+ its three ListManagers           3 * 1048616  =  3145848 B  ( ~3.00 MiB )
                                                  ---------
                                                   4194520 B  ( ~4.00 MiB )
```

plus up to 4096 bytes of alignment padding per allocation (4 allocations →
up to 16 KiB), plus whatever data chunks are actually touched.

Against the baseline tier in 5.2 (GTX 750, 2 GB): 1024 element lists alone are
1024 * 1048616 = 1073782784 B = **1024.0 MiB**, i.e. half the card, before any
particle data. A single non-dense SNode with a node allocator is ~4 MiB of
pointer table. This is the number that actually constrains 6.1, not the 24 KiB
in `LLVMRuntime`.

Caveat I must not overstate: element lists are only allocated for SNodes in
trees where `all_dense` is false (runtime.cpp:998-1000), and node allocators
only where the codegen asks. So this is a per-*used*-SNode cost, not a flat
one. Recording the mechanism; the sizing rule is the planner's call (8.1 item
2).

## 6. Who indexes the arrays, and with what

Full-tree grep for the two constants (excluding `build/`) returns only 8 sites:
`taichi/codegen/llvm/struct_llvm.cpp:266`, `taichi/inc/constants.h:12-13`, and
`runtime.cpp:562,563,567,568,569`. Nothing else in the tree mentions either
name. So the ceiling is enforced in exactly one place.

Traced the indices:

- `element_lists[i]` is written in `runtime_initialize_snodes`
  (runtime.cpp:1002-1008) over `i` in `[root_id, root_id + num_snodes)`.
- `node_allocators[snode_id]` written in `runtime_NodeAllocator_initialize`
  (runtime.cpp:1026-1031).
- `ambient_elements[snode_id]` written in `runtime_allocate_ambient`
  (runtime.cpp:1033-1040).

Host caller for all three:
`taichi/runtime/llvm/llvm_runtime_executor.cpp:441-468`
(`LlvmRuntimeExecutor::initialize_llvm_runtime_snodes`). `root_id` comes from
`field_cache_data.root_id` (line 400) and `snode_id` from
`snode_metas[i].id` (line 448).

`SNode::id` is assigned from a single global counter:
`taichi/ir/snode.cpp:12` (`std::atomic<int> SNode::counter{0}`) and
`taichi/ir/snode.cpp:220` (`id = counter++`). It is reset exactly once, in the
`Program` constructor, `taichi/program/program.cpp:144` (`SNode::counter = 0`).
`SNode::reset_counter()` at `taichi/ir/snode.h:348` exists but has no caller
anywhere in the tree (grepped .cpp/.h/.py).

**This matters.** The arrays are indexed by the *global, program-lifetime,
monotonically increasing* SNode id. But the only bound check,
`struct_llvm.cpp:266`, is `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes)`
where `snodes` is the collection for a *single* tree (`StructCompilerLLVM::run`
takes one `SNode &root` and calls `collect_snodes(root)` at line 250). Two
trees of 600 SNodes each pass that assertion individually and then index
`element_lists[600..1199]`, past the end of the 1024 array. Need to confirm
whether ids are reused when a tree is destroyed.

`Program::destroy_snode_tree` (program.cpp:214-236) pushes only the *tree* id
onto `free_snode_tree_ids_` (line 235). It does not return SNode ids.
`Program::allocate_snode_tree_id` (program.cpp:559-566) recycles tree ids.
So: tree ids are recycled, SNode ids are never recycled.

Also noted: `allocate_snode_tree_id` has **no bound check against
`kMaxNumSnodeTreesLlvm`** either. It returns `snode_trees_.size()` unbounded.
`roots[512]` / `root_mem_sizes[512]` are indexed by that id at
runtime.cpp:995-997 via `snode_tree_id`. Nothing in the tree checks it.

Flagging both as escalations rather than resolving. I am not going to decide
whether these are reachable in practice.

## 7. 32-bit width sweep — device runtime (`runtime.cpp` + `node_*.h`)

Read runtime.cpp end to end plus the five `node_*.h` headers. Recording every
32-bit width that sits on an addressing/index path.

### 7a. Verified narrowing conversion already in the tree

`StructMeta::max_num_elements` is `i64` (runtime.cpp:302). But every
`*_get_num_elements` returns `i32` and returns that field directly:

- `taichi/runtime/llvm/runtime_module/node_dense.h:10-12`
- `taichi/runtime/llvm/runtime_module/node_pointer.h:10-12`
- `taichi/runtime/llvm/runtime_module/node_bitmasked.h:10-12`

Each is `i32 X_get_num_elements(Ptr meta, Ptr node) { return ((StructMeta *)meta)->max_num_elements; }`
— an implicit i64→i32 truncation. `StructMeta::get_num_elements` is declared
`i32 (*)(Ptr, Ptr)` at runtime.cpp:311. So the field is already 64-bit and the
accessor already throws the top half away. Someone widened the field and not
the accessor.

`Dynamic_get_num_elements` (node_dynamic.h:116-119) returns `node->n`, an `i32`
in `DynamicNode` (node_dynamic.h:3-7). `Root_get_num_elements`
(node_root.h:21-23) returns literal 1.

### 7b. Integer-typed offset arithmetic in the node accessors

`node_pointer.h` computes byte offsets in `int`, not in a pointer-width type:

- line 44 `volatile Ptr lock = node + 8 * i;`
- line 45 `volatile Ptr *data_ptr = (Ptr *)(node + 8 * (num_elements + i));`
- line 69, 70 (same shape, `Pointer_deactivate`)
- line 86 (`Pointer_is_active`)
- line 92 (`Pointer_lookup_element`)

`8` is an `int` literal, `i` is `int`, `num_elements` is `i32`. The product is
evaluated in `int` and only then added to the pointer. That overflows at
i >= 2^28 (268435456). This is a genuine 32-bit ceiling on elements per
pointer node, tighter than the i32 index itself by a factor of 8.

By contrast `node_dense.h:22-23` and `node_bitmasked.h:41-43` do
`node + ((StructMeta *)meta)->element_size * i`, where `element_size` is
`std::size_t`, so the multiply is promoted to 64-bit. Those are only bounded by
the `int i` parameter, not by the arithmetic.

`node_bitmasked.h:18-20` and 27-29: `data_section_size = element_size * num_elements`
(size_t * i32 → size_t, promoted, fine); `mask_begin[i / 32]`, `1UL << (i % 32)`.
Noted in passing: `Bitmasked_is_active` (node_bitmasked.h:32-39) calls
`Dense_get_num_elements`, not `Bitmasked_get_num_elements`. Same body, so no
behavioural difference. Not my call to change; recording only.

`node_dynamic.h`: `chunk_start` is `int` (lines 22, 67, 99), `i` is `int`,
`chunk_size` is `int` (`DynamicMeta::chunk_size`, node_dynamic.h:11).
`(i - chunk_start) * meta->element_size` (lines 81, 105) promotes to size_t.
`atomic_max_i32(&node->n, i + 1)` (line 21) and `atomic_add_i32(&node->n, 1)`
(line 65) are i32.

### 7c. `ListManager` — the class upstream already flagged

runtime.cpp:423 carries upstream's own TODO: "there are many i32 types in this
class, which may be an issue if there are >= 2 ** 31 elements."

- 425-426 `static constexpr std::size_t max_num_chunks = 128 * 1024;` and
  `Ptr chunks[max_num_chunks];`
- 429 `i32 log2chunk_num_elements;`
- 430 `i32 lock;`
- 431 `i32 num_elements;`
- 449-454 `i32 reserve_new_element()` — `atomic_add_i32(&num_elements, 1)`,
  `chunk_id = i >> log2chunk_num_elements`
- 464 `void touch_chunk(int chunk_id);`
- 466-472 `i32 get_num_active_chunks()` — loops `int i` to `max_num_chunks`
- 478 `void resize(i32 n)`
- 481-484 `Ptr get_element_ptr(i32 i)` — the linearised offset:
  `chunks[i >> log2chunk_num_elements] + element_size * (i & ((1 << log2chunk_num_elements) - 1))`.
  `element_size` is size_t so that half promotes; the shift and mask are i32.
- 491-494 `Ptr touch_and_get(i32 i)`
- 496 `i32 size()`
- 500-508 `i32 ptr2index(Ptr ptr)` — returns `(i << log2chunk_num_elements) + i32((ptr - chunks[i]) / element_size)`.
  `i` is `int` there, so `i << log2chunk_num_elements` is int-shift.

Capacity implied: `max_num_chunks * max_num_elements_per_chunk`. For element
lists that is 131072 * 65536 = 2^33 slots, which already exceeds what the
`i32 num_elements` counter can index (2^31). So the chunk table is sized for
more than the index type can reach. Recording the inconsistency, not resolving
it.

### 7d. `NodeManager`

runtime.cpp:630-705. `i32 lock`, `i32 element_size`, `i32 chunk_num_elements`,
`i32 free_list_used`, `i32 recycle_list_size_backup`,
`using list_data_type = i32` (line 641). So the free/recycled lists store
element indices as **i32**. `allocate()` (665-676) and `locate()` (678-680)
are i32. `gc_serial()` (683-704) iterates in `int`.

Constructor (643-663): `chunk_num_elements` defaults to 128*1024, and the
128 MB cap loop at 653-656 does the compare in `(uint64)chunk_num_elements *
element_size`, i.e. this one *is* 64-bit-safe.

### 7e. Structural / listgen paths, all `int`

- `Element` (runtime.cpp:517-521): `int loop_bounds[2]`.
- `PhysicalCoordinates` (runtime.cpp:281-283): `i32 val[taichi_max_num_indices]`.
  This is the physical coordinate carrier, 12 x i32.
- `clear_list` (1269-1272), `element_listgen_root` (1281-1326),
  `element_listgen_nonroot` (1328-1377): loop counters `int c`, `int i`,
  `int j`, `int c_start`, `int c_step`, `num_parent_elements` is `int`.
  `c * ch_element_size < ch_num_elements` at 1316 is an int multiply that can
  overflow.
- `cpu_block_task_helper_context` (1381-1388): `int element_size`,
  `int element_split`.
- `cpu_struct_for_block_helper` (1397-1414): `int element_id = i / ctx->element_split`,
  `int lower`, `int upper`.
- `parallel_struct_for` (1416-1464): `int snode_id`, `int element_size`,
  `int element_split`, `auto list_tail = list->size()` (i32);
  line 1461-1462 `runtime->parallel_for(..., list_tail * element_split, ...)`
  — int multiply.
- `range_task_helper_context` (1471-1481): `int begin`, `int end`,
  `int block_size`, `int step`.
- `cpu_parallel_range_for_task` (1483-1508): `int block_start = ctx.begin + task_id * ctx.block_size` — int multiply.
- `cpu_parallel_range_for` (1510-1537): all `int`; grid size
  `(end - begin + block_dim - 1) / block_dim` in int.
- `gpu_parallel_range_for` (1539-1561): `int idx`, `int begin`, `int end`.
- `mesh_task_helper_context` (1563-1571) / `cpu_parallel_mesh_for` (1595-1622)
  / `gpu_parallel_mesh_for` (1624-1644): `int num_patches`, `int block_size`.
- `linear_thread_idx` (1646-1652) returns `i32`.
- gc: `node_gc` (1690-1692), `gc_parallel_impl_0` (1694-1718)
  (`int i = linear_thread_idx`), `gc_parallel_impl_1` (1725-1736)
  (`const i32 num_unused`), `gc_parallel_impl_2` (1742-1782) — all i32/int,
  `snode_id` is `int` in all three `gc_parallel_N` entry points (1720, 1737,
  1780).
- `runtime_initialize_snodes` (runtime.cpp:986-1017): `const int root_id`,
  `const int num_snodes`, `const int snode_tree_id`, loop `int i`.
- `runtime_NodeAllocator_initialize` (1026-1031) and `runtime_allocate_ambient`
  (1033-1040): `int snode_id`.
- `runtime_get_memory_requirements` (889-905): `i32 num_rand_states`; the
  accumulator is `i64 size`, so the size math itself is 64-bit.
- `LLVMRuntime::num_rand_states` is `i32` (runtime.cpp:594).

Not on an addressing path, recorded for completeness: `STRUCT_FIELD_ARRAY`
(runtime.cpp:68-76) and `RUNTIME_STRUCT_FIELD_ARRAY` (82-87) both take the
array index as `int i`. These are the generated host-callable accessors for
`element_lists`, `node_allocators`, `roots`, `root_mem_sizes` and
`PhysicalCoordinates::val`, so the accessor signature is `int`-indexed too.

Already 64-bit in the runtime, i.e. does not need changing for 6.2:
`ListManager::element_size` and `max_num_elements_per_chunk` (std::size_t),
`StructMeta::element_size` (std::size_t) and `max_num_elements` (i64),
`LLVMRuntime::error_code` and `total_requested_memory` (i64),
`root_mem_sizes` (size_t), all of `PreallocatedMemoryChunk` (Ptr/size_t),
`allocate_aligned` / `allocate_from_reserved_memory` (std::size_t),
`get_temporary_pointer(LLVMRuntime *, u64 offset)` (runtime.cpp:720-722),
the local-stack ops `stack_top_primal` / `stack_push` (1874-1897, u64 counter).

## 8. Host side of the LLVM runtime

`taichi/runtime/llvm/llvm_runtime_executor.cpp`.

`materialize_runtime` (580-747... actually 632-747): asks the device runtime
for its own size via `runtime_get_memory_requirements`
(llvm_runtime_executor.cpp:675-677, `call<void*, int32_t, int32_t>`), then
preallocates `iroundup(runtime_objects_prealloc_size + result_buffer_size,
taichi_page_size)` at line 691-694. So `sizeof(LLVMRuntime)` is a **device**
allocation on CUDA/AMDGPU, paid once per Program, up front. That is where the
24 KiB of SNode arrays lands on the GTX 750.

`initialize_llvm_runtime_snodes` (391-468) is the per-tree path:
- 417 `rounded_size = taichi::iroundup(root_size, taichi_page_size)`
- 419-420 root buffer from `snode_tree_buffer_manager_->allocate(rounded_size, tree_id, result_buffer)`
- 436 `snode_tree_allocs_[tree_id] = alloc` (an `std::unordered_map<int, DeviceAllocation>`, llvm_runtime_executor.h:152 — unbounded, so no ceiling here)
- 442-444 `runtime_jit->call<void *, std::size_t, int, int, int, std::size_t, Ptr>("runtime_initialize_snodes", ...)`.
  I initially read the 7 explicit template args against 8 call arguments as a
  mismatch. It is not. `JITModule::call` is
  `template <typename... Args> void call(const std::string &name, Args... args)`
  (taichi/jit/jit_module.h:54-62), so the explicit list is a prefix and the 8th
  (`all_dense`) is deduced as `bool`. Signature matches
  `runtime_initialize_snodes` (runtime.cpp:986-994). Dead end, recorded so a
  reviewer does not re-chase it.
- 446-468 per-SNode allocator/ambient init, `snode_id` is `int`.

`SNodeCacheData::id` is `int` (llvm_offline_cache.h:68); `FieldCacheData::tree_id`
and `root_id` are `int` (76-77); `root_size` and `cell_size_bytes` and
`chunk_size` are `size_t` (78, 70-71). Note llvm_offline_cache.h:115-118
carries upstream's own comment that "snode_tree_id is not continuous".

`SNodeTreeBufferManager` (taichi/runtime/llvm/snode_tree_buffer_manager.h:16-29,
.cpp:12-24): `std::map<int, DeviceAllocation> snode_tree_id_to_device_alloc_`,
keyed by `int` tree id. `allocate(std::size_t size, const int snode_tree_id,
uint64 *result_buffer)`. The "contiguous chunk backing" the brief mentions
(snode_tree.h:13) is exactly this: one device allocation per tree, size
`iroundup(root_size, taichi_page_size)`, zeroed by memset
(llvm_runtime_executor.cpp:421-434), then `import_memory`'d.

`root_size` comes from `StructCompilerLLVM::run` (struct_llvm.cpp:269-270,
`tlctx_->get_type_size(node_type)`) and is `std::size_t` throughout. So the
tree's own byte size is 64-bit clean on this path.

Destroy path: `LlvmRuntimeExecutor::destroy_snode_tree`
(llvm_runtime_executor.cpp:758-761) → `TaichiLLVMContext::delete_snode_tree(id)`
(llvm_context.cpp:968-973, erases from `struct_modules` maps) +
`SNodeTreeBufferManager::destroy` (frees the device allocation).

**Gap I want to record:** nothing in the destroy path clears
`runtime->element_lists[i]`, `node_allocators[i]` or `ambient_elements[i]` for
the SNode ids that tree owned, and nothing frees the ~1 MiB ListManagers those
slots point at. Nor does it clear `roots[tree_id]` / `root_mem_sizes[tree_id]`.
The device-side `runtime_memory_chunk` is a bump allocator
(`allocate_from_reserved_memory`, runtime.cpp:836-874, only ever advances
`preallocated_head`) with no free. I am flagging this as an observation about
the lifecycle, not proposing anything.

`SNode` ids are also not recycled (see note 6), so a destroyed-then-recreated
tree consumes fresh ids and fresh runtime slots. That interacts directly with
6.1: the ceiling is on ids ever issued, not on live SNodes.

`TaichiLLVMContext` keeps struct modules in
`std::unordered_map<int, std::unique_ptr<llvm::Module>> struct_modules` keyed
by tree id (llvm_context.cpp:682, 689, 693; llvm_context.h:163 holds a parallel
`snode_tree_funcs_` map). Unbounded — no 512 ceiling on the host side.

## 9. `taichi/struct/` — the whole layer is 41 + 15 lines

`taichi/struct/struct.h` (26 lines) is `StructCompiler`: three public members,
`std::vector<SNode *> stack`, `std::vector<SNode *> snodes`,
`std::size_t root_size` (struct.h:10-12), plus three pure virtuals.
`taichi/struct/struct.cpp:7-13` is `collect_snodes`, a plain preorder DFS
push_back. Loop counter is `int ch_id` over `snode.ch.size()`
(struct.cpp:9) — the only width in the file.

`taichi/struct/snode_tree.h` / `.cpp` is the SNodeTree class:
`kFirstID = 0` (snode_tree.h:17), `int id_` (snode_tree.h:40),
`std::unique_ptr<SNode> root_` (41). `get_snodes_to_root_id` returns
`std::unordered_map<int, int>` (snode_tree.h:52, snode_tree.cpp:88-93).
`check_tree_validity` (snode_tree.cpp:76-86) only checks that non-place,
non-root nodes have children.

The "contiguous chunk of memory" in the snode_tree.h:13 doc comment is not
implemented in this class at all. It lives in
`SNodeTreeBufferManager::allocate` on the LLVM path and in
`GfxRuntime::add_root_buffer` on the gfx path.

## 10. Contiguity assumption in `runtime_initialize_snodes`

`runtime_initialize_snodes` fills `element_lists[i]` for
`i` in `[root_id, root_id + num_snodes)` (runtime.cpp:1002-1008), where
`num_snodes = (int)snode_metas.size()` (llvm_runtime_executor.cpp:444).

But `snode_metas` is built from `struct_compiler.snodes`
(llvm_program.cpp:109-119), which is `collect_snodes`' preorder DFS, and each
entry carries its **actual** `snodes[i]->id` (llvm_program.cpp:112). The
`node_allocators` / `ambient_elements` loop at llvm_runtime_executor.cpp:446-468
uses those real ids. The element-list loop does not — it uses the arithmetic
range.

So the runtime relies on: the SNode ids of one tree being exactly
`root_id .. root_id + count - 1`, with no gaps and in DFS order. Nothing in
`taichi/struct/` or `taichi/program/` asserts that. Whether it can be violated
(interleaved SNode construction, `lazy_grad`, `lazy_dual`,
`allocate_adjoint_checkbit` — snode.h:323-327) is beyond what I was asked to
determine. Escalating rather than chasing.

## 11. gfx / SPIR-V backend side of the struct layer

`taichi/runtime/gfx/snode_tree_manager.{h,cpp}` and
`taichi/runtime/gfx/runtime.cpp`. No fixed-size SNode arrays here at all.

- `std::vector<CompiledSNodeStructs> compiled_snode_structs_`
  (snode_tree_manager.h:39), appended in `materialize_snode_tree`
  (snode_tree_manager.cpp:54-59).
- `std::vector<std::unique_ptr<DeviceAllocationGuard>> root_buffers_`
  (runtime.h:152), appended in `GfxRuntime::add_root_buffer`
  (runtime.cpp:730-750).
- `std::unordered_map<DeviceAllocation *, size_t> root_buffers_size_map_`
  (runtime.h:164).

So `taichi_max_num_snodes` and `kMaxNumSnodeTreesLlvm` do **not** constrain the
gfx path. Consistent with the constant's name (`...Llvm`) and with the grep in
note 6.

Widths on this path: `get_root_buffer(int id)` / `get_root_buffer_size(int id)`
(runtime.cpp:752-765), `get_field_in_tree_offset(int tree_id, ...)` returns
`size_t` and accumulates `mem_offset_in_parent_cell` in `size_t`
(snode_tree_manager.cpp:74-90), `get_snode_tree_device_ptr(int tree_id)`
(snode_tree_manager.cpp:92-94). `root_size` is `size_t`.
`GfxRuntime::RegisterParams::num_snode_trees` is `std::size_t`
(runtime.h:40, 94).

`SNodeTreeManager::destroy_snode_tree` (snode_tree_manager.cpp:61-72) does a
linear scan of `compiled_snode_structs_` matching on the root pointer, resets
`root_buffers_[root_id]`, and leaves the `compiled_snode_structs_` entry in
place. So the vector index is materialisation order, and Program's recycled
tree ids (program.cpp:559-566) are a different numbering. Recording; not
resolving.

## 12. How the constant gets baked in — the build/install seam for 6.1

`runtime.cpp` is not linked into the host binary. It is compiled separately to
LLVM bitcode:

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15`
```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
        -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR}
```
one per arch in `HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH` (line 29-31), and
installed to `${CMAKE_INSTALL_PREFIX}/python/taichi/_lib/runtime` (line 13).
`-I ${PROJECT_SOURCE_DIR}` is what picks up `taichi/inc/constants.h`, so the
ceiling is compiled into the bitcode.

It is loaded back at **run time from the install tree**:
`TaichiLLVMContext::module_from_file` (llvm_context.cpp:358-362) reads
`{runtime_lib_dir()}/runtime_{arch}.bc`; `get_runtime_fn`
(llvm_context.cpp:209-211) builds that name; `runtime_lib_dir()`
(taichi/util/lang_util.cpp:30-45) resolves from `compiled_lib_dir` or the
`TI_LIB_DIR` environment variable.

The host never declares `LLVMRuntime`'s layout. `context.h:11` is a bare
forward declaration, and codegen pulls the type out of the loaded bitcode by
name: `TaichiLLVMContext::get_runtime_type`
(llvm_context.cpp:1004-1011) does `llvm::StructType::getTypeByName(ctx, "struct." + name)`,
used at codegen_llvm.cpp:2697 for `"LLVMRuntime"`. So the array sizes are
single-sourced from runtime.cpp — there is no host-side mirror to keep in sync.

**But** the host binary does have its own copy of `taichi_max_num_snodes`, used
for the assertion at struct_llvm.cpp:266. Host binary and installed `.bc` are
separate artifacts. If an install-time build parameter changes the constant and
only one of the two is regenerated, the assertion and the actual array size
disagree, silently. Escalating this; it is a build-architecture question and
overlaps agent 04.

The checked-in `cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` (3744 bytes) is
a different artifact, built from `cuda_runtime.cu` by
`COMPILE_CUSTOM_CUDA_LIBRARY` (CMakeLists.txt:17-26), which is commented out at
lines 36-42. Its loader is `taichi/util/lang_util.cpp:19-28`.

## 13. Where codegen reaches into my arrays

Only one place, and it is the tree-root lookup:
`TaskCodeGenLLVM::get_root(int snode_tree_id)`
(taichi/codegen/llvm/codegen_llvm.cpp:2689-2692) emits
`call("LLVMRuntime_get_roots", get_runtime(), tlctx->get_constant(snode_tree_id))`.
`LLVMRuntime_get_roots` is generated by `STRUCT_FIELD_ARRAY(LLVMRuntime, roots)`
(runtime.cpp:618), so its signature is `Ptr LLVMRuntime_get_roots(LLVMRuntime *, int i)`
— an **i32-indexed** load out of `roots[kMaxNumSnodeTreesLlvm]`, in every kernel
that touches a field. That is the codegen/runtime seam for the tree ceiling.

`element_lists` / `node_allocators` / `ambient_elements` are reached from
device code only through `meta->snode_id` inside the node helpers
(node_pointer.h:55, 76, 96; node_dynamic.h:30, 51, 74, 112) and from the host
through the `runtime_query` wrappers in llvm_runtime_executor.cpp:263, 328, 339.
None of those are bounds-checked.

## 14. `taichi/program/` — 32-bit sweep

Verified narrowing, `taichi/program/ndarray.cpp:35-38` and `75-78`:

```
nelement_(std::accumulate(std::begin(shape_), std::end(shape_), 1,
                          std::multiplies<>())),
```

`nelement_` is `std::size_t` (ndarray.h:80), but `std::accumulate`'s accumulator
type is deduced from the init value, which is the `int` literal `1`, and
`shape_` is `std::vector<int>` (ndarray.h:55). So the product is computed in
`int` and overflows before the conversion to `size_t`. It then feeds
`allocate_memory_on_device(nelement_ * element_size_, ...)` at ndarray.cpp:61.

Contrast ndarray.cpp:53 and 96, where the same idiom is written `1LL` and is
64-bit clean. Two lines apart, in the same constructor. So the `1` at line 38
and line 78 reads as an oversight rather than a decision. Not proposing a fix,
recording the fact.

`flatten_index` (ndarray.cpp:16-23) accumulates in `size_t` — clean.

`Ndarray::shape` and `total_shape_` are `std::vector<int>` (ndarray.h:55, 82).
`nelement_` and `element_size_` are `std::size_t` (80-81).

`LaunchContextBuilder`:
- `set_arg_external_array_with_shape` (launch_context_builder.cpp:220-241) takes
  `const std::vector<int64> &shape` and then writes each entry as
  `(int32)shape[i]` at line 240. Explicit i64→i32 truncation of the shape.
- `set_arg_ndarray_impl` (306-327): `(int32)shape[i]` at line 324;
  `size_t total_size` accumulated at 325 (promoted, clean);
  `set_array_runtime_size(arg_id, total_size)`.
- `array_runtime_sizes` is `unordered_map<vector<int>, uint64, ...>`
  (launch_context_builder.h:125-126) — 64-bit, clean.
- `arg_buffer_size` / `result_buffer_size` are `size_t`
  (launch_context_builder.h:114, 116).
- All the `taichi_max_num_indices` guards: lines 230, 247, 269, 302, 322.

Cross-boundary, outside my territory but it is the canonical statement of 6.2 —
`taichi/ir/snode.cpp:88-101`:
```
int64 acc_shape = 1;
for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
  // casting to int32 in extractors.
  new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
  acc_shape *= new_node.extractors[i].shape;
}
if (acc_shape > std::numeric_limits<int>::max()) {
  ... "SNode index might be out of int32 boundary but int64 indexing is not
       supported yet. Struct fors might not work either."
```
and the matching ndarray warning at ndarray.cpp:55-60 / 98-103. Upstream knows
the ceiling is 2^31 cells per container and warns rather than supporting it.
That belongs to agent 01; I record it only because it is the same width my
runtime structures carry.

## 15. Remainder of the territory — no SNode-count or addressing exposure

Swept for fixed-size arrays across `taichi/runtime/`, `taichi/struct/`,
`taichi/program/`. Besides the five in `LLVMRuntime` and
`ListManager::chunks`, the only fixed arrays are:
`PhysicalCoordinates::val[taichi_max_num_indices]` (runtime.cpp:289),
`Element::loop_bounds[2]` (runtime.cpp:519),
`error_message_template[2048]` / `error_message_arguments[32]` (587-588),
`printf_helper::buffer[1024]` (runtime.cpp:1831),
`internal_functions.h:84` `Ptr ptrs[24]` and `:119` `Ptr ptrs[kN]`,
`taichi/runtime/cuda/jit_cuda.cpp:28-29` `options[max_num_options]` (JIT
compile options, unrelated),
`taichi/runtime/amdgpu/kernel_launcher.cpp:8` `attr_val[8]`.
None scale with SNode count.

## 16. Closing arithmetic derived for the report

Generalising the verified `sizeof(LLVMRuntime) = 35256` into the two ceilings.
With N = `taichi_max_num_snodes` and T = `kMaxNumSnodeTreesLlvm`:

```
sizeof(LLVMRuntime) = 2488 + 16*T + 24*N
```
where
```
24*N   = 3 arrays * 8 bytes    (element_lists, node_allocators, ambient_elements)
16*T   = 2 arrays * 8 bytes    (roots, root_mem_sizes)
  2048 = error_message_template
   256 = error_message_arguments
   184 = chunks, function pointers, locks, counters
  ----
  2488
```
Check at N=1024, T=512: 2488 + 8192 + 24576 = 35256. Matches the compiler.

The device-side charge is `iroundup(sizeof(LLVMRuntime), 4096)`
(runtime.cpp:897, `taichi::iroundup` at taichi/math/arithmetic.h:13-17) =
36864 bytes at the current values.

Full runtime-objects preallocation on CUDA/AMDGPU
(`runtime_get_memory_requirements`, runtime.cpp:889-905):
```
iroundup(sizeof(LLVMRuntime), 4096)                    = 36864
+ iroundup(taichi_global_tmp_buffer_size, 4096)        = 1048576
+ iroundup(sizeof(RandState) * num_rand_states, 4096)
```
`sizeof(RandState)` = 20 (verified: four u32 + one i32, runtime.cpp:527-533).
`num_rand_states` = `saturating_grid_dim * max_block_dim` on CUDA
(llvm_runtime_executor.cpp:648). A result buffer of
`sizeof(uint64) * taichi_result_buffer_entries` = 256 bytes is appended
(llvm_runtime_executor.cpp:681, 691-694).

On CPU there is no preallocation: `runtime_initialize` falls through to
`host_allocator(memory_pool, sizeof(LLVMRuntime), 128)` (runtime.cpp:931-932).

## 17. Also checked, nothing found

- `taichi/runtime/llvm/runtime_module/internal_functions.h`: mostly self-tests.
  The one live path is the sparse-matrix triplet insert,
  `insert_triplet_f32` / `_f64` (lines 30-45) via the `ATOMIC_INSERT(T)` macro
  (lines 12-21). `insert_triplet_f32` indexes `data_base_ptr[triplet_id * 3]`
  with `triplet_id` an `int32` from `atomic_add_i32` — a 32-bit index, but into
  the sparse matrix builder, not SNode addressing.
- `taichi/program/argpack.{h,cpp}`, `callable.*`, `compile_config.*`,
  `function.*`, `graph_builder.*`, `kernel.*`, `matrix.h`, `texture.*`: nothing
  sized by SNode count.
- `taichi/runtime/{cpu,cuda,amdgpu}/kernel_launcher.*` and `jit_*`: launch
  plumbing, no SNode arrays.
- `taichi/runtime/dx12/`, `taichi/runtime/gfx/aot_*`: AOT module build/load,
  agent 04's territory.
- Confirmed by grep that `snode_trees_` is never reset in
  `Program::destroy_snode_tree` (only `snode_trees_[id] = std::move(tree)` on
  reuse at program.cpp:249, and `push_back` at 252). The destroyed tree's
  `SNodeTree` and its `SNode` objects stay alive in the vector until the slot is
  reused.

## 18. Line-number corrections

Before writing the report I re-derived every runtime.cpp line number by grep
rather than by eye. Several of my earlier entries were off by one to three
lines. The findings are unchanged; the citations below supersede the earlier
ones. Corrected values:

| Thing | Earlier note said | Actual |
|---|---|---|
| upstream i32 TODO above `ListManager` | 423 | 424-425 |
| `struct ListManager` | 424-509 | 426-513 |
| `max_num_chunks` / `chunks[]` | 425-426 | 427-428 |
| `log2chunk_num_elements`/`lock`/`num_elements` | 429-431 | 431-433 |
| `reserve_new_element` | 449-454 | 451-456 |
| `touch_chunk` decl | 464 | 465 |
| `get_num_active_chunks` | 466-472 | 467-473 |
| `resize` | 478 | 479-481 |
| `get_element_ptr` | 481-484 | 483-486 |
| `touch_and_get` | 491-494 | 493-496 |
| `size()` | 496 | 498-500 |
| `ptr2index` | 500-508 | 502-512 |
| `struct PhysicalCoordinates` | 281-283 | 288-290 |
| `StructMeta::max_num_elements` | 302 | 310 |
| `StructMeta::get_num_elements` decl | 311 | 318 |
| `LLVMRuntime::create<T>` | 605-611 | 606-612 |
| `NodeManager::allocate` | 665-676 | 666-677 |
| `NodeManager::locate` | 678-680 | 679-681 |
| `NodeManager::gc_serial` | 683-704 | 688-706 |
| `allocate_from_reserved_memory` | 836-874 | 838-874 |
| `runtime_get_memory_requirements` | 889-905 | 891-906 |
| `runtime_initialize` | 908-958 | 911-958 |
| `all_dense` early return | 998-1000 | 1000-1002 |
| element-list loop | 1002-1008 | 1003-1007 |
| `clear_list` | 1269-1272 | 1270-1273 |
| `element_listgen_root` | 1281-1326 | 1282-1327 |
| `element_listgen_nonroot` | 1328-1377 | 1331-1378 |
| `cpu_block_task_helper_context` | 1381-1388 | 1387-1394 |
| `cpu_struct_for_block_helper` | 1397-1414 | 1403-1420 |
| `parallel_struct_for` | 1416-1464 | 1422-1465 |
| `range_task_helper_context` | 1471-1481 | 1471-1481 (unchanged) |
| `cpu_parallel_range_for_task` | 1483-1508 | 1483-1509 |
| `cpu_parallel_range_for` | 1510-1537 | 1511-1538 |
| `gpu_parallel_range_for` | 1539-1561 | 1541-1562 |
| `mesh_task_helper_context` | 1563-1571 | 1567-1575 |
| `cpu_parallel_mesh_for` | 1595-1622 | 1599-1623 |
| `gpu_parallel_mesh_for` | 1624-1644 | 1627-1647 |
| `linear_thread_idx` | 1646-1652 | 1650-1656 |
| `node_gc` | 1690-1692 | 1691-1693 |
| `gc_parallel_0/1/2` entry points | 1720/1737/1780 | 1721/1738/1782 |
| `ListManager::touch_chunk` def | (not cited) | 1664-1679 |
| `stack_top_primal` / `stack_push` | 1874-1897 | 1876-1897 |

The five `LLVMRuntime` array declarations (562, 563, 567, 568, 569),
`runtime_initialize_snodes` (986), `runtime_NodeAllocator_initialize` (1026),
`runtime_allocate_ambient` (1033), `struct Element` (517-521),
`struct RandState` (527-533), `struct NodeManager` (630), `list_data_type`
(641), the NodeManager constructor (643-663), and
`get_temporary_pointer` (723) were all taken from grep in the first place and
are correct as written.

## 19. Report written

Wrote `modernization/investigation/report-03-runtime-struct.md`. Before
finalising I re-verified by grep every line number cited in it, including the
ones I had only read by eye. Two more corrections found and applied to the
report at that stage:

- `LLVMRuntime::num_rand_states` is at runtime.cpp:595, not 594.
- `LLVMRuntime::total_requested_memory` is at runtime.cpp:597, not 596.
- `llvm_device()->import_memory(...)` is at
  llvm_runtime_executor.cpp:437-438 with the store to `snode_tree_allocs_` at
  440, not "435-436".

Two files touched in total, both under `modernization/investigation/`. No
source file was modified. The only compilation I ran was `g++ -fsyntax-only`
on a heredoc piped through stdin, which writes nothing.

# ===== REVISION PASS (adversarial round one) =====

## 20. Adjudicating the pool arithmetic — 03-1 vs 03-2

Both adversaries found what I missed: the ~1 MiB `ListManager`s are not cut
from free device memory, they are bump-allocated out of a fixed preallocated
chunk whose size is `config_.device_memory_GB`, defaulting to 1.

Verified myself, with exact line numbers re-derived by `awk` over the file:

- `taichi/program/compile_config.cpp:63` — `device_memory_GB = 1;  // by default, preallocate 1 GB GPU memory`
- `taichi/program/compile_config.cpp:64` — `device_memory_fraction = 0.0;`
- `taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632` — `preallocate_runtime_memory()`
- `:613-619` — the `if (device_memory_fraction == 0) total_prealloc_size = device_memory_GB * (1UL << 30); else ... fraction * total_mem;`
- `:620` — `TI_ASSERT(total_prealloc_size <= total_mem);`
- `:629-631` — `runtime_jit->call(..., "runtime_initialize_memory", ...)`
- `runtime.cpp:962-971` — `runtime_initialize_memory` sets `runtime_memory_chunk`

**Adjudication 1: the assert line is 620.** 03-2 says 620, 03-1 says 619.
`awk` output above shows 620. **03-2 correct.**

**Adjudication 2: the abort lines are 864-869 and 872.** 03-1 says 864-869 /
872; 03-2 says 860-868 / 871. `awk` shows `__assertfail(` at 864, its last
argument line at 869, `#endif` at 870, closing brace at 871,
`taichi_assert_runtime(this, success, ...)` at 872. **03-1 correct.**

**Adjudication 3: the three bump-allocator citations.** 03-1 gives
`allocate_aligned` 823, `allocate_from_reserved_memory` 838, head advance 853,
`runtime_initialize_memory` 962. 03-2 gives 822, 837, 852, 961 — each one
early. `awk` confirms 823, 838, 853, 962. **03-1 correct on all four.**

**Adjudication 4: pool capacity, 1020 or 1023.** This is the substantive one.
I worked the allocator arithmetic rather than taking either number.

`allocate_from_reserved_memory`, runtime.cpp:848-853:
```
auto alignment_bytes =
    alignment - 1 - (preallocated_head + alignment - 1) % alignment;
size += alignment_bytes;
if (preallocated_head + size <= preallocated_tail) {
  ret = (Ptr)(preallocated_head + alignment_bytes);
  memory_chunk.preallocated_head += size;
```

Two things to note. First, `alignment_bytes` is the standard
`(A - h % A) % A`: at `h % 4096 == 0` it is `4095 - 4095 = 0`; at
`h % 4096 == 40` it is `4095 - ((40 + 4095) % 4096) = 4095 - 39 = 4056`.
Second, the padding is added to `size` *before* the fit test at line 851 and
before the head advance at 853, so the caller is charged for it. The head is
**not** rounded up after the payload — it ends at `aligned_h + payload`.

`1048616 mod 4096 = 40` (1048616 = 4096*256 + 40). So the head sits 40 bytes
into a page after each `ListManager`, and every subsequent one is charged
`4056 + 1048616 = 1052672` = exactly 257 pages.

Cumulative head after k element lists, assuming a page-aligned pool base:
```
head(k) = 1048616 + (k-1) * 1052672
```
- head(1020) = 1048616 + 1019*1052672 = 1048616 + 1072672768 = 1,073,721,384  <= 2^30 = 1,073,741,824  ✅
- head(1021) = 1,074,774,056  > 2^30  ❌

**Capacity is 1020. 03-1 is correct; 03-2's 1023 drops the padding.**

And 1024 lists would need head(1024) = 1048616 + 1023*1052672 = 1,077,932,072,
overrunning the 1 GiB default by **4,190,248 bytes**, about four lists' worth.
03-2's headline "the existing constant overruns the existing default by 40,960
bytes — under one page over" is therefore wrong, and 03-1's correction of it is
right. The *qualitative* point both make — that the constant and the default
pool are matched to within about half a percent, so raising one alone buys
nothing — survives, and is the finding that matters.

**Caveat I am adding that neither adversary states:** the formula assumes the
pool base is 4096-aligned. The base comes from
`LlvmRuntimeExecutor::preallocate_memory` (`llvm_runtime_executor.cpp:586-604`)
→ `llvm_device()->allocate_memory(...)`. I did not establish the alignment
guarantee of that path; it is in `taichi/rhi/`, outside my territory. If the
base is not page-aligned the first allocation also pays padding and capacity
drops by at most one. Recording the assumption explicitly rather than hiding
it.

## 21. Which path actually sets up the pool

Verified. `preallocate_runtime_memory` has exactly two call sites:

- `llvm_runtime_executor.cpp:719-723` — in `materialize_runtime`, for
  `cuda || amdgpu`, but only `if (!use_device_memory_pool())`.
- `llvm_runtime_executor.cpp:412-414` — in `initialize_llvm_runtime_snodes`,
  `if (config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense)`.

`use_device_memory_pool_` is set at `llvm_runtime_executor.cpp:49` from
`CUDAContext::get_instance().supports_mem_pool()`, which is
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` gated on driver >= 11.2
(`taichi/rhi/cuda/cuda_context.cpp:35-53`), default `false`
(`llvm_runtime_executor.h:162`).

So on CUDA the 1 GiB pool is set up either way; only *when* differs. 03-1's
observation that `all_dense` gates whether `runtime_memory_chunk` exists at all
is correct but applies **only on the mem-pool branch**. On the non-mem-pool
branch and on AMDGPU the pool is created unconditionally at program init.
I am sharpening 03-1 here rather than repeating it.

On CPU neither call site fires, so `runtime_memory_chunk.preallocated_size`
stays 0 and `allocate_aligned` (runtime.cpp:830-834) falls through to
`host_allocator`. **The 1 GiB ceiling is a CUDA/AMDGPU fact, not a universal
one.** Neither adversary says this and it matters for brief section 2, which
makes CPU-only a first-class target.

## 22. My citation fault — diagnosed and confirmed

Both adversaries charge me with a systematic +54 / +43 offset into
`taichi/struct/snode_tree.cpp` and `taichi/runtime/gfx/snode_tree_manager.cpp`.
**They are right, and I can name the cause exactly.**

I read those two files with a single `cat -n <header> <impl>` invocation.
`cat -n` numbers the *concatenation*, not each file, so every `.cpp` line
number I recorded carries the header's length as an offset. `snode_tree.h` is
54 lines; `gfx/snode_tree_manager.h` is 43 lines. Those are exactly the two
offsets.

I did not make the same mistake on `taichi/struct/struct.{h,cpp}`, because
there I ran two separate `cat -n` calls in one shell line. Both adversaries
independently confirmed those citations are correct, which corroborates the
diagnosis.

Re-read both files with `cat -n` per file. True line numbers:

`taichi/struct/snode_tree.cpp` — 41 lines total:
| Item | True lines |
|---|---|
| `get_snodes_to_root_id_impl` (anon ns) | 6-13 |
| `SNodeTree::SNodeTree` | 17-20 |
| `SNodeTree::check_tree_validity` | 22-32 |
| `get_snodes_to_root_id` | 34-39 |

`taichi/runtime/gfx/snode_tree_manager.cpp` — 54 lines total:
| Item | True lines |
|---|---|
| `SNodeTreeManager::SNodeTreeManager` | 8-9 |
| `materialize_snode_tree` | 11-16 (`push_back` at 15) |
| `destroy_snode_tree` | 18-29 (scan 20-24, `root_buffers_[root_id].reset()` at 28) |
| `get_field_in_tree_offset` | 31-47 (`TI_ASSERT_INFO` 34-38, `size_t offset` 40-44) |
| `get_snode_tree_device_ptr` | 49-51 |

My *content* at each of those citations was right. Only the addresses were
wrong. Report corrected.

## 23. Other citation disputes, adjudicated against the source

Re-derived every one with `awk 'NR>=a && NR<=b {printf "%d\t%s\n", NR, $0}'`,
which numbers a single file and cannot repeat the concatenation fault.

| Item | I said | 03-1 said | 03-2 said | Source |
|---|---|---|---|---|
| `STRUCT_FIELD_ARRAY` macro | 68-76 | 69-77 | 69-77 | **69-77** |
| `RUNTIME_STRUCT_FIELD_ARRAY` macro | 82-87 | 85-88 | 85-88 | **85-88** |
| int multiply in `element_listgen_root` | 1316 | 1322, 1323 | 1322, 1323 | **1319, 1322, 1323** — 1316 is the `std::min`; the loop condition at 1319 also multiplies, which neither adversary counted |
| `element_listgen_root` extent | 1282-1327 | — | 1282-1328 | **1282-1329** |
| `element_listgen_nonroot` extent | 1331-1378 | — | 1331-1380 | **1331-1383** |
| `set_arg_external_array_with_shape` | 220-241 | — | — | **220-242**, guard 230, truncation 240 |
| `set_arg_ndarray_impl` | 306-327, trunc 324 | 308-330, trunc 326 | — | **308-330**, guard 322, trunc 326, `total_size *= shape[i]` 327 |
| NodeManager 128K default | — | 646-649 | 646-649 | **647 comment, 648-650 the `if`** |
| NodeManager 128 MB cap loop | 653-656 | — | 651-655 | **652-655** |
| `TI_ASSERT(total_prealloc_size <= total_mem)` | — | 619 | 620 | **620** (03-2) |
| CUDA `__assertfail` / fallback | — | 864-869 / 872 | 860-868 / 871 | **864-869 / 872** (03-1) |
| `allocate_aligned` | 823 | 823 | 822 | **823** (03-1, me) |
| `allocate_from_reserved_memory` | 838 | 838 | 837 | **838** (03-1, me) |
| bump head advance | — | 853 | 852 | **853** (03-1) |
| `runtime_initialize_memory` | — | 962 | 961 | **962** (03-1) |

So the adversaries split roughly evenly against each other, and my own
`runtime.cpp` citations held except the four rows above.

## 24. Content adjudications

**`all_dense` — both adversaries are right and I was wrong.**
`llvm_runtime_executor.cpp:402-410`:
```
bool all_dense = config_.demote_dense_struct_fors;
for (size_t i = 0; i < snode_metas.size(); i++) {
  if (snode_metas[i].type != SNodeType::dense &&
      snode_metas[i].type != SNodeType::place &&
      snode_metas[i].type != SNodeType::root) {
    all_dense = false;
    break;
  }
}
```
The config flag is the **seed**; the loop can only clear it. So
`all_dense == demote_dense_struct_fors AND (every node is dense/place/root)`.
My report said the config "also forced it on", which is exactly backwards — it
is a necessary precondition for the skip, never a force. And I cited line 401;
the assignment is at 402.

Consequence I must state, which follows from the conjunction: with
`demote_dense_struct_fors == false`, `all_dense` is false unconditionally and
**every tree pays a `ListManager` per SNode, dense trees included**. Default is
`true` (`compile_config.cpp:18`) and `CompileConfig::fit` forces it true again
for SPIR-V archs (`compile_config.cpp:72-74`).

**`is_gc_able`** is `pointer || dynamic` only,
`taichi/ir/snode_types.cpp:21-23`. Verified. My report said "pointer, dynamic"
via the call site without citing the definition; now cited.

**`NodeManager` chunk count** — 03-2 is right that the sole call site overrides
the 128K default: `runtime.cpp:1029-1030` passes `1024 * 16`, so
`chunk_num_elements` is 16384 and the 128 MB halving loop at 652-655 engages
only when `node_size > 8192`. 03-1's remark that neither report *quoted* the
128K default is also right — I listed the constructor parameters and stopped.
Both are correct about different things.

**`ptr2index` — 03-1 is right, 03-2 overstates.** `runtime.cpp:502-512`:
line 505 is `taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.")`
*inside* the loop and *before* the range test at 506. `enable_assert` is a
`constexpr bool ... = true` (runtime.cpp:329), so the loop aborts the thread at
the first untouched chunk rather than walking 131072 entries. It is
O(chunks touched). `get_num_active_chunks` (467-473) genuinely is
unconditional over 131072, and its only caller is
`runtime_ListManager_get_num_active_chunks` (runtime.cpp:743-747) from the host
debug printer `print_list_manager_info` (llvm_runtime_executor.cpp:188-209).
Neither is a kernel hot path in the sense 03-2 implies. The underlying route
`Pointer_deactivate` → `recycle` → `locate` → `ptr2index` is real.

**gfx numbering divergence — closing it, as instructed.** With the true line
numbers from note 22 I can now construct it, and 03-2's construction holds:
1. `materialize_snode_tree` (snode_tree_manager.cpp:11-16) only `push_back`s,
   so the vector index is materialisation order.
2. `destroy_snode_tree` (:18-29) resets `root_buffers_[root_id]` at :28 and
   leaves both vectors' lengths unchanged.
3. `Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
   (program.cpp:235); `allocate_snode_tree_id` pops it (program.cpp:559-567).
4. `get_snode_tree_device_ptr(int tree_id)` (:49-51) returns
   `root_buffers_[tree_id]->get_ptr()`.
Create trees 0 and 1, destroy 1, create a third. The third gets `tree_id == 1`
from the free stack; its buffer is pushed at index 2; `root_buffers_[1]` is the
`unique_ptr` reset at :28. `get_snode_tree_device_ptr(1)` dereferences null and
`get_field_in_tree_offset(1, ...)` reads the destroyed tree's descriptors and
trips its own `TI_ASSERT_INFO` at :34-38. **Diverges after one
destroy-then-add cycle.** The LLVM path does not have the equivalent problem:
`snode_tree_allocs_` is an `unordered_map` keyed by tree id
(llvm_runtime_executor.h:152), so a recycled id overwrites rather than shifts.

**Reachability of the destroy path.** Verified myself:
`grep -rn destroy_snode_tree` over the tree shows `Program::destroy_snode_tree`
(program.cpp:214-236) has exactly one caller, the pybind lambda at
`taichi/python/export_lang.cpp:569-570`, driven from
`python/taichi/_snode/snode_tree.py:21`. No C++ core caller, no test caller.
Brief section 1.2 puts the Python front end out of scope, so today the stale
tables and the gfx divergence are **latent, not live**. Both adversaries reach
this and both are right. I must state it rather than leaving the lifecycle
findings ambiguous.

**Generated setters have zero callers.** Verified:
`grep -rn "LLVMRuntime_set_element_lists\|..._set_node_allocators\|..._set_roots\|..._set_root_mem_sizes"` over the tree
returns nothing at all outside the `STRUCT_FIELD_ARRAY` expansions at
runtime.cpp:616-619. So there is no mechanism to clear a slot, not merely no
call. 03-1's sharpening accepted.

**`snode_tree_allocs_` is never erased.** Verified: three occurrences only —
declared `llvm_runtime_executor.h:152`, written `:440`, read `:387`. 03-2
found this and I did not. Accepted.

## 25. Corrected footprint arithmetic, simulated not hand-derived

I no longer trust hand arithmetic on the bump allocator, so I simulated
`allocate_from_reserved_memory` (runtime.cpp:838-874) exactly — pad to the
alignment, charge the pad to the caller, advance the head by pad+size, fail
when `head + size > tail` — against a 2^30 pool, and read the numbers off.

Chunk sizes, from `ListManager::touch_chunk` (runtime.cpp:1664-1679, allocation
at 1672-1674, `max_num_elements_per_chunk * element_size`, alignment 4096):

| List | elements/chunk | element_size | chunk bytes |
|---|---|---|---|
| element list (runtime.cpp:1005-1006) | 65536 (`1024*64`) | 64 (`sizeof(Element)`) | 4,194,304 |
| NodeManager free_list / recycled_list | 16384 (`1024*16`, runtime.cpp:1030) | 4 (`sizeof(i32)`) | 65,536 |
| NodeManager data_list | 16384 | `node_size` | 16384 * node_size |

`sizeof(Element)` = 64 by construction (Ptr 8 + int[2] 8 + PhysicalCoordinates
48); both adversaries measured 64 with the compiler, and
`sizeof(PhysicalCoordinates)` = 12 * 4 = 48.

`node_size` is set on the host at `llvm_runtime_executor.cpp:448-456`:
`cell_size_bytes` for pointer, `sizeof(void*) + cell_size_bytes * chunk_size`
for dynamic.

Simulation results against the default 1 GiB pool:

| Configuration | Steady-state bytes per SNode | Fit in 1 GiB |
|---|---|---|
| element list header only | 1,052,672 | **1020** |
| element list header + its first 4 MiB chunk | 5,246,976 | **204** |
| pointer SNode, all headers, no chunks | 4,214,784 | **254** |
| pointer SNode, headers + element chunk + a 4 MiB data_list chunk (node_size=256) | 12,603,392 | **85** |

The first row settles the 03-1 / 03-2 dispute: **1020, not 1023**. And the
1024 constant needs head(1024) = 1,077,932,072 against a 1,073,741,824-byte
pool, an overrun of **4,190,248 bytes**, not 03-2's "40,960 bytes, under one
page".

The second and third rows reproduce 03-1's 5,246,976 / 204 and 4,214,784 / 254
exactly. I had briefly mis-derived 5,251,032 by hand; the simulation gives
5,246,976 and 204 * 5,246,976 = 1,070,383,104 closes exactly, so 5,246,976 is
right. Recording the slip rather than hiding it.

Not folded into any headline, because it is data-dependent: each
garbage-collectable SNode also takes an ambient element,
`allocate_aligned(runtime_memory_chunk, size, 128, request=true)`
(runtime.cpp:1038-1039). Alignment 128, not 4096, and size is `node_size`.
Neither adversary includes it.

## 26. Findings from the adversaries I verified and am adopting

Each checked against the source by me, not taken on their word:

- `taichi_listgen_max_element_size = 1024` (`taichi/inc/constants.h:28`), used
  at runtime.cpp:1316 and 1369 and at `codegen_llvm.cpp:2291`. A third 1024 in
  the dispatch path. My "exhaustive fixed-size array" sweep missed it because I
  grepped for array declarations, not for uses of `inc/constants.h` values.
  Verified by grep.
- `taichi_max_num_mem_requests = 1024 * 64` (`constants.h:16`) has no use
  anywhere in the tree. Verified by the same grep.
- `taichi/runtime/llvm/runtime_module/CMakeLists.txt:13` reads `${arch}` while
  the target at :8 reads the function parameter `${rtm_arch}`. Verified by
  reading the file with per-file numbering. It works only because CMake lets a
  function read the caller's scope and the `foreach` variable at :29-30 happens
  to hold the same value.
- `UnifiedAllocator::allocate` does
  `allocation_size = std::max(allocation_size, default_allocator_size)`
  (`taichi/rhi/common/unified_allocator.cpp:68`), so the CPU path has no
  ceiling on `sizeof(LLVMRuntime)`. Verified.
- Host/device width mismatch on the list-manager queries:
  `RUNTIME_STRUCT_FIELD(ListManager, max_num_elements_per_chunk)` and
  `(ListManager, element_size)` (runtime.cpp:759, 760) expose fields that are
  `std::size_t` on the device (runtime.cpp:429-430), while the host reads them
  as `int32` (`llvm_runtime_executor.cpp:193-198`) through
  `runtime_query<int32>`, which union-casts the low bytes of a u64. Verified.
- `SNode::SNode(const SNode &)` is `TI_NOT_IMPLEMENTED`
  (`taichi/ir/snode.cpp:230-233`), so ids are consumed exactly once per
  constructed node. Verified; strengthens my cumulative-ceiling finding.
- `ListManager::clear()` (runtime.cpp:475-477) only sets `num_elements = 0`.
  Chunks are never returned, so the touched set is monotonic for the life of
  the process and the 4 MiB per element list is a floor. Verified.
- `is_gc_able` definition at `taichi/ir/snode_types.cpp:21-23`. Verified.

Not adopting: 03-2's claim that `ptr2index` is an unconditional 131072-entry
scan. See note 24.

## 27. Scope check on the pool finding

Before writing it into the report I checked what it is *not*.

- It is a CUDA/AMDGPU fact. On CPU neither call site of
  `preallocate_runtime_memory` fires, `runtime_memory_chunk.preallocated_size`
  stays 0, and `allocate_aligned` falls through to `host_allocator`
  (runtime.cpp:830-834). Brief section 2 makes CPU-only a first-class target,
  so the 1020 figure must not be stated as universal. Neither adversary says
  this.
- `device_memory_GB` is reachable from C++ only as the `CompileConfig` default.
  Its only external setter is the Python binding. I am not chasing that; brief
  section 1.2.
- `TI_ASSERT(total_prealloc_size <= total_mem)` at
  `llvm_runtime_executor.cpp:620` means the pool can never exceed the card, so
  on a 2 GB GTX 750 the largest pool a configuration can request is under 2 GiB
  and the root buffers, the runtime-objects buffer and the CUDA context all
  come out of the same card *outside* this pool.

What the pair `(taichi_max_num_snodes, device_memory_GB)` should be is a
decision the project owner has not made. It goes in Escalations, unresolved.

## 28. Verification of the revised report

Rewrote `report-03-runtime-struct.md` in full rather than patching it, because
section 3 (the pool) reorders the argument and the footprint numbers in
section 2 all changed.

Then verified mechanically. I extracted every `file:line` and `file:a-b`
citation from the finished report with a regex, resolved bare `runtime.cpp` to
`taichi/runtime/llvm/runtime_module/runtime.cpp` and bare `CMakeLists.txt` to
`taichi/runtime/llvm/runtime_module/CMakeLists.txt`, and bounds-checked each
against the real file length.

**219 distinct citations, all in range, none past end of file.** That is the
check that would have caught the +54 / +43 fault in the first pass, and it is
now part of the process rather than an eyeball pass.

Bounds checking does not catch a citation that is in range but points at the
wrong line, so I additionally printed the source line for 47 citations —
every one I newly introduced this pass, every one either adversary disputed,
and every one I changed. All correct. Sample:

```
runtime.cpp:69    #define STRUCT_FIELD_ARRAY(S, F)
runtime.cpp:85    #define RUNTIME_STRUCT_FIELD_ARRAY(S, F)
runtime.cpp:339   constexpr bool enable_assert = true;
runtime.cpp:505   taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.");
runtime.cpp:1030  runtime->create<NodeManager>(runtime, node_size, 1024 * 16);
runtime.cpp:1319  for (int c = c_start; c * ch_element_size < ch_num_elements; c += c_step) {
llvm_runtime_executor.cpp:402  bool all_dense = config_.demote_dense_struct_fors;
llvm_runtime_executor.cpp:620  TI_ASSERT(total_prealloc_size <= total_mem);
compile_config.cpp:63          device_memory_GB = 1;  // by default, preallocate 1 GB GPU memory
snode_tree.cpp:22              void SNodeTree::check_tree_validity(SNode &node) {
snode_tree_manager.cpp:28      runtime_->root_buffers_[root_id].reset();
```

Four citations were in range but wrong, found by this pass and fixed before
publication: `enable_assert` was 329 not 339; `get_snode_tree_device_ptr` was
386-389 not 385-388; `preallocate_memory` was 587-604 not 586-604; the
`cache_field` snode loop was 110-119 not 108-119 and the function 97-122 not
97-121.

Also re-measured `sizeof(Element)` = 64, `sizeof(PhysicalCoordinates)` = 48 and
`sizeof(StructMeta)` = 72 with my own compiler run rather than adopting the
adversaries' figures, because section 2.4's 4 MiB chunk depends on
`sizeof(Element)` and I state it as VERIFIED.

Two files touched this pass: `report-03-runtime-struct.md` (rewritten) and
this notes file (appended). No source file modified. The only compilation was
`g++ -fsyntax-only` on a heredoc piped through stdin, which writes nothing;
the pool simulation was a `python3` heredoc, which also writes nothing.

---

# Amendment pass — round-two adversaries

Entries 29-36. Input: `adversary2-03-1.md` and `adversary2-03-2.md`. The two
round-two adversaries split on severity: 03-1 called the remaining gap two
corrections wide, 03-2 found two further errors 03-1 lacked and judged the pair
neither correct nor complete. Instruction was to take the harsher list and to
verify every item against source rather than adopt it. Six claims were put to
me. I verified all six. All six hold. I changed nothing on an adversary's word.

## 29. Claim 1 — `ptr2index` is arch-dependent. UPHELD. My report was wrong.

Claim: my section 8.3 states the GPU behaviour as universal and repeats it in
E14; `taichi_assert_format` emits its thread kill only under CUDA and AMDGPU, so
the CPU build runs all 131072 iterations on the not-found path.

Read for myself, not adopted:

```
runtime.cpp:502   i32 ptr2index(Ptr ptr) {
runtime.cpp:504     for (int i = 0; i < max_num_chunks; i++) {
runtime.cpp:505       taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.");
runtime.cpp:506       if (chunks[i] <= ptr && ptr < chunks[i] + chunk_size) {
runtime.cpp:511     return -1;
```

`taichi_assert_runtime` is `runtime.cpp:817-819`, a one-line forward to
`taichi_assert_format` at `766-815`. Grepping the preprocessor lines in that
function gives the whole answer:

```
780:  if (!enable_assert || test != 0)      <- early return when the test PASSES
782:  if (!runtime->error_code) {           <- record once
798: #if ARCH_cuda
800:   asm("exit;");
801: #elif ARCH_amdgpu
802:   asm("S_ENDPGM");
814: #endif
```

No `#else`. No CPU branch. The function returns normally on x64. `enable_assert`
is `constexpr bool ... = true` at `runtime.cpp:339`, which is the fact my
revision-2 pass stopped at — I read line 505 and 339 and never opened
`taichi_assert_format`. That is exactly the diagnosis 03-2 gives of my notes
entry 24, and it is correct.

Host bitcode arch: `runtime_module/CMakeLists.txt:8` passes
`-D "ARCH_${rtm_arch}"`, driven over `HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH`
at `:29-31`. So the host build defines `ARCH_x64`, not `ARCH_cuda`.

Behaviour on CPU when the pointer is absent: line 505 flags the error and
returns; line 506 evaluates `nullptr <= ptr` (true) and `ptr < nullptr + size`
(false); no match, no break; 131072 iterations, then -1 at `:511`.

Report section 8.3 rewritten with the three-way table. E14 rewritten. This was a
wrong adjudication of mine, made under a heading that explicitly ruled against
round one, not an oversight. Recorded as such.

## 30. Claim 2 — root buffers are inside the pool when `use_device_memory_pool()` is false. UPHELD. Only 03-2 has it.

The instruction was to verify this one carefully because a single adversary
holds it. I read all eight hops rather than the summary chain.

```
llvm_runtime_executor.cpp:419-420   snode_tree_buffer_manager_->allocate(...)
snode_tree_buffer_manager.cpp:15    runtime_exec_->allocate_memory_on_device(size, result_buffer)
llvm_runtime_executor.cpp:479-485   llvm_device()->allocate_memory_runtime({..., use_device_memory_pool()})
cuda_device.cpp:56                  } else if (params.use_memory_pool) {
cuda_device.cpp:57-58                 malloc_async                      <- outside the pool
cuda_device.cpp:60-61                 DeviceMemoryPool::allocate_with_cache  <- the other branch
device_memory_pool.cpp:32             allocator_->allocate(device, params)
allocator.cpp:54-55                   device->allocate_llvm_runtime_memory_jit(params)   (cache miss)
cuda_device.cpp:82-84                 runtime_jit->call("runtime_memory_allocate_aligned", ...)
runtime.cpp:884-885                   runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)
```

`runtime_memory_chunk` is the 1 GiB pool. Same allocator, same head, same tail.
My section 3.4 said the opposite in plain words. It is wrong and is corrected.

Two scope facts I established myself, because the claim's scope is what makes
it matter and I would not take it on assertion:

- **AMDGPU does not even have the branch.** `AmdgpuDevice::allocate_memory_runtime`
  (`amdgpu_device.cpp:55-78`) calls `allocate_with_cache` unconditionally at
  `:62-63`. There is no `use_memory_pool` test in it at all. So on AMDGPU the
  root buffers always come from the pool — and separately,
  `use_device_memory_pool_` can never be true on AMDGPU anyway, because its only
  write is `llvm_runtime_executor.cpp:49` inside the `arch == Arch::cuda` branch
  at `:39`, against the `false` initialiser at `llvm_runtime_executor.h:162`.
- **CPU is not on this path at all.** `CpuDevice::allocate_memory_runtime`
  (`cpu_device.cpp:44-51`) calls plain `allocate_memory`, not the JIT hop.
  `CpuDevice::allocate_llvm_runtime_memory_jit` exists at `:53-59` but is
  reached only through `CachingAllocator`, which CPU's runtime path does not
  enter. Consistent with there being no pool on CPU at all (report 3.2).

The CUDA gate, read at source rather than taken from the claim:
`cuda_context.cpp:36-37` requires driver major > 11, or major 11 with minor >= 2,
before it will even query `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` at
`:38-40`; the `else` at `:41-50` warns and forces `device_supports_mem_pool = 0`
at `:49`. `supports_mem_pool_` is set at `:53` only if that is non-zero.

Ndarrays and argpacks: `ndarray.cpp:61` and `argpack.cpp:17-18` both call
`Program::allocate_memory_on_device` (`program.h:245-248`), which forwards to
`program_impl_->allocate_memory_on_device`, which for the LLVM path is
`llvm_program.h:124-127` → `runtime_exec_->allocate_memory_on_device`, i.e. hop
3 above. Confirmed by reading, not inferred from the class name.

The runtime-objects buffer is genuinely separate and my report was right about
it: `llvm_runtime_executor.cpp:687-690` is its own `preallocate_memory` call,
feeding `runtime_objects_chunk` at `runtime.cpp:936-941`.

**Where I decline to go as far as the claim.** 03-2 writes that this makes every
capacity row an upper bound "on exactly the baseline-tier hardware". The AMDGPU
case and the sub-11.2-driver case are established. Whether a GTX 750 on a
current driver reports the mem-pool attribute is a device-and-driver fact I
cannot read from this tree, and plan section 5.2's baseline tier is that card. I
state the two unconditional cases and stop. Report 3.4 says so explicitly.

Escalated as E15, because how much headroom a sizing rule should reserve is a
decision and not a measurement.

## 31. Claim 3 — `device_memory_GB` is reachable from C++. UPHELD, and it is the one correction that changes a decision.

I was told to establish this firmly. Four links, each read:

```
compile_config.h:8      struct CompileConfig {          <- plain struct, public fields
compile_config.h:71-72  float64 device_memory_GB;  float64 device_memory_fraction;
compile_config.h:110    extern TI_DLL_EXPORT CompileConfig default_compile_config;
lang_util.cpp:14        CompileConfig default_compile_config;
program.cpp:75          config = default_compile_config;
program.cpp:77          config.fit();
export_lang.cpp:264-267 m.def("default_compile_config", []() -> CompileConfig & { return default_compile_config; }, reference)
```

The pybind lambda at `export_lang.cpp:264-267` returns a **reference to the same
global**. So Python is one client of the field, not its owner, and
`export_lang.cpp:208-210`'s `def_readwrite` binds a public struct member. My E1's
"at C++ level it is currently a hard-coded 1" was false. `compile_config.cpp:63`
sets the constructor default; nothing prevents a C++ driver from overwriting the
global before constructing a `Program`.

`demote_dense_struct_fors` is the same shape: public field at
`compile_config.h:28` on the same global.

The consequence I was told to establish. Plan section 8.1 item 2 records the
replacement for 1024 as a pair. The two halves are not equally expensive:

- `taichi_max_num_snodes` is `constexpr int` at `constants.h:12` and is pulled
  into every per-arch `.bc` by the `-I ${PROJECT_SOURCE_DIR}` on
  `runtime_module/CMakeLists.txt:8`. Moving it is a build-time mechanism, and by
  my own section 4.7 a mismatch between the host binary's copy and the installed
  bitcode has no detection path.
- `device_memory_GB` is a runtime write to an exported global. No mechanism.

Revision 2 told the planner the opposite. E1 rewritten with the table.

Both are `float64`, so fractional values are legal. Recorded because 8.1.2 is a
sizing question.

## 32. Claim 4 — `llvm_program.cpp:112` should be 113. UPHELD.

```
110:   const auto &snodes = struct_compiler.snodes;
111:   for (size_t i = 0; i < snodes.size(); i++) {
112:     LlvmOfflineCache::FieldCacheData::SNodeCacheData snode_cache_data;
113:     snode_cache_data.id = snodes[i]->id;
```

112 is the declaration; the assignment is 113. My section 4.5 cited 112. Fixed.
The span `110-119` quoted in section 2.3 is right and does not move.

## 33. Claim 5 — two array-inventory gaps. UPHELD, and there are three.

03-1 named two, 03-2 named the same two plus a third. Taking the harsher list, I
grepped `runtime.cpp` for fixed-extent declarations myself:

```
1435:  alignas(8) char tls_buffer[1];     in parallel_struct_for, #if ARCH_cuda || ARCH_amdgpu
1552:  alignas(8) char tls_buffer[64];    in gpu_parallel_range_for, #ifdef ARCH_amdgpu
1636:  alignas(8) char tls_buffer[64];    in gpu_parallel_mesh_for,  #ifdef ARCH_amdgpu
```

The `[1]` at 1435 is documented at `:1433-1434` as a placeholder rewritten to
`tls_buffer_size` during codegen. The two `[64]`s are the AMDGPU arms of
`#ifdef`s whose `#else` arms (`:1554`, `:1638`) are variable-length. The other
`tls_buffer` declarations at `:1412`, `:1487`, `:1581` are variable-length and
are not fixed-size arrays, so they do not belong in the section 9 inventory.

None scale with SNode count, so section 9's conclusion is unaffected. The word
"exhaustive" was not earned and the three entries are now in the list.

## 34. Claim 6 — nothing in the tree aligns the pool base. UPHELD. Assumption closed.

My section 3.3 declared a page-aligned-base assumption and left it open on the
grounds that the base is set in `taichi/rhi/`, outside my territory. Both
adversaries went there. I followed:

```
llvm_runtime_executor.cpp:594-596  llvm_device()->allocate_memory(params, &alloc)
cuda_device.cpp:28-29              mem_pool.allocate(params.size, DeviceMemoryPool::page_size, managed)
device_memory_pool.cpp:35-37       void *DeviceMemoryPool::allocate(size, alignment, managed) {
device_memory_pool.cpp:40            return allocate_raw_memory(size, managed);
device_memory_pool.cpp:67            CUDADriver::get_instance().malloc(&ptr, size);
```

The `alignment` parameter is declared at `:36` and never read. Line 40 forwards
size and `managed` only. So the base is whatever `cuMemAlloc` returns and
nothing in this tree constrains it.

It does not change the answer. Slack after 1020 headers is
`1073741824 - 1073721384 = 20440` bytes and the largest possible extra first pad
is 4095. Capacity is 1020 at any base offset. Section 3.3 now states the chain
and drops the hedge, rather than carrying an open assumption the source closes.

## 35. Not on the lead's list, but on the harsher list, and I acted on it

Both round-two adversaries independently flagged the same self-contradiction in
my section 3.2: "on CUDA the pool is created either way; only *when* differs",
immediately followed by a sentence saying `all_dense` gates whether the chunk
exists at all. The second sentence is right. Reading the two guards:

```
llvm_runtime_executor.cpp:412  if (config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense)
llvm_runtime_executor.cpp:719  if (config_.arch == Arch::cuda || config_.arch == Arch::amdgpu) {
llvm_runtime_executor.cpp:720    if (!use_device_memory_pool()) {
```

With arch cuda, `use_device_memory_pool()` true and `all_dense` true, `:412`
fails on `!all_dense` and `:720` fails on `!use_device_memory_pool()`. **No pool
is created.** Replaced with a four-row table. Flagged to the lead that this was
on the harsher list but not in the enumerated claims.

## 36. What I did not disturb, and what I did not do

Left alone, as instructed, and each re-checked against the round-two files
before leaving it: pool capacity 1020 and overrun 4,190,248 bytes; the CPU
exemption on the pool in section 3.2; the gfx numbering closure in 7.2; the
revision-2 citation repair. Both adversaries re-simulated the allocator
independently, both reached 1020, and 03-2 withdrew its round-one 1023.

I did not re-derive any figure that was upheld, did not open any new line of
investigation, did not touch source, and did not touch another agent's file. Two
files written this pass: `report-03-runtime-struct.md` and this one. All source
inspection was `awk`, `sed` and `grep` reads. No compilation this pass; the
sizeof figures were not in dispute.

Unresolved judgement went to Escalations, not into the body: E15 is new, E1 and
E14 are rewritten.

## 37. One cosmetic item, verified and fixed

Not in the lead's enumerated claims; on 03-2's list as a discretionary item.
E12 cited the `int` accumulate initialiser as `ndarray.cpp:38` and `:78`. Read:

```
35:  nelement_(std::accumulate(std::begin(shape_),
36:                            std::end(shape_),
37:                            1,
38:                            std::multiplies<>())),
```

The literal is on 37, and 77 for the second constructor. The spans quoted in
section 8.1B, `35-38` and `75-78`, were already right, and the clean `1LL`
counterparts really are at `:53` and `:96`. E12 corrected. Nothing else moves.

Section 3.4's heading was renamed from "The ceiling on the pool itself" to "The
ceiling on the pool, and who else draws from it", because after entry 30 the
section carries both.
