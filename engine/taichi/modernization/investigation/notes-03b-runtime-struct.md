# Notes 03B — runtime and struct layer

Contemporaneous working notes. Appended as I go. Agent 03B.

## N01 — Starting point: constants.h

Read `/opt/project/taichi/taichi/inc/constants.h` in full (76 lines).
Relevant constants for my territory:

- `taichi_max_num_indices = 12`   (line 5)
- `taichi_max_num_args = 8`       (line 7, "legacy: only used in opengl backends")
- `taichi_max_num_args_total = 64` (line 10)
- `taichi_max_num_args_extra = 32` (line 11)
- `taichi_max_num_snodes = 1024`  (line 12)
- `kMaxNumSnodeTreesLlvm = 512`   (line 13)
- `taichi_max_num_mem_requests = 1024 * 64` (line 16)
- `taichi_listgen_max_element_size = 1024` (line 27)

Surprise #1 already: the brief names `taichi_max_num_snodes` but there is a
SECOND SNode-scaling constant right next to it, `kMaxNumSnodeTreesLlvm = 512`,
which the brief does not mention. That one also sizes a runtime array. Flagging
now, will chase.

All of these are `constexpr int` / `constexpr std::size_t` in a header with no
`#ifndef` guard on the values, i.e. no existing build-time override mechanism.
There is no `#cmakedefine` in this file — it is a plain header, not generated.

## N02 — Territory listing

`taichi/struct/` is tiny: only `snode_tree.{h,cpp}` and `struct.{h,cpp}`.
Note the brief's 6.1 mentions `taichi/codegen/llvm/struct_llvm.cpp` which is
NOT in my territory (that's 02/02B's codegen). I will read it only far enough
to state the coupling, and mark it as outside my assignment.

## N03 — The LLVMRuntime struct, exact layout

`taichi/runtime/llvm/runtime_module/runtime.cpp:551-611` is `struct LLVMRuntime`.
Field-by-field, in declaration order (x86-64 / NVPTX64, all pointers 8 bytes):

| Offset | Field | Line | Bytes |
|---|---|---|---|
| 0 | `runtime_objects_chunk` (PreallocatedMemoryChunk) | 552 | 24 |
| 24 | `runtime_memory_chunk` | 553 | 24 |
| 48 | `host_allocator` | 555 | 8 |
| 56 | `assert_failed` | 556 | 8 |
| 64 | `host_printf` | 557 | 8 |
| 72 | `host_vsnprintf` | 558 | 8 |
| 80 | `memory_pool` | 559 | 8 |
| 88 | `roots[512]` | 562 | 4096 |
| 4184 | `root_mem_sizes[512]` | 563 | 4096 |
| 8280 | `thread_pool` | 565 | 8 |
| 8288 | `parallel_for` | 566 | 8 |
| 8296 | **`element_lists[1024]`** | 567 | **8192** |
| 16488 | **`node_allocators[1024]`** | 568 | **8192** |
| 24680 | **`ambient_elements[1024]`** | 569 | **8192** |
| 32872 | `temporaries` | 570 | 8 |
| 32880 | `rand_states` | 571 | 8 |
| 32888 | `profiler` | 584 | 8 |
| 32896 | `profiler_start` | 585 | 8 |
| 32904 | `profiler_stop` | 586 | 8 |
| 32912 | `error_message_template[2048]` | 588 | 2048 |
| 34960 | `error_message_arguments[32]` (u64) | 589 | 256 |
| 35216 | `error_message_lock` (i32) | 590 | 4 (+4 pad) |
| 35224 | `error_code` (i64) | 591 | 8 |
| 35232 | `result_buffer` | 593 | 8 |
| 35240 | `allocator_lock` (i32) | 594 | 4 |
| 35244 | `num_rand_states` (i32) | 596 | 4 |
| 35248 | `total_requested_memory` (i64) | 598 | 8 |

`sizeof(LLVMRuntime)` = **35256 bytes**, alignment 8. Member functions
(`allocate_aligned`, `allocate_from_reserved_memory`, `set_result`, `create`)
are non-virtual, so contribute nothing.

The three SNode-indexed arrays are 24576 bytes = **69.7%** of the struct.
`roots` + `root_mem_sizes` add another 8192 (23.2%). Together, 93% of the
LLVMRuntime struct is fixed-size tables indexed by SNode or SNode-tree id.

Rounded to `taichi_page_size` (4096) at
`runtime.cpp:929-930`, the runtime object occupies **36864 bytes = 9 pages**
out of the device preallocation.

## N04 — Where the runtime struct actually lives

`runtime_initialize` at `runtime.cpp:911-959`. On CUDA/AMDGPU
(`preallocated_size != 0`) the runtime object is placed at the head of the
device preallocated buffer, `runtime.cpp:926-931`. So the 24 KiB of SNode
tables is **device memory**, on the 2 GB GTX 750 as much as anywhere.
It is a one-off 36 KiB, not per-tree — see N05.

## N05 — Surprise #2: kMaxNumSnodeTreesLlvm has no assertion

`grep` for both constants across the whole tree (excluding build/):

- `taichi_max_num_snodes` — 4 sites: `inc/constants.h:12`,
  `codegen/llvm/struct_llvm.cpp:266` (the TI_ASSERT), and
  `runtime.cpp:567,568,569`.
- `kMaxNumSnodeTreesLlvm` — 3 sites: `inc/constants.h:13` and
  `runtime.cpp:562,563`. **No assertion anywhere.**

So `roots[snode_tree_id]` and `root_mem_sizes[snode_tree_id]` at
`runtime.cpp:996-997` are written with a tree id that nothing in my territory
bounds-checks against 512. Need to find where snode_tree_id is issued.

## N06 — SNode ids are GLOBAL and never recycled. Surprise #3.

`taichi/ir/snode.h:88-89`: `static std::atomic<int> counter; int id{0};`
`taichi/ir/snode.cpp:12`: `std::atomic<int> SNode::counter{0};`
`taichi/ir/snode.cpp:220`: `id = counter++;` in the SNode(depth, type, ...) ctor.
`taichi/ir/snode.h:348-350`: `static void reset_counter() { counter = 0; }`.

`grep` for who resets it: only `taichi/program/program.cpp:144`
(`SNode::counter = 0;`), in the `Program` constructor. `reset_counter()` itself
has **zero callers**.

So within one `Program` lifetime, SNode ids are monotonically increasing and
are **never reused**, including after `destroy_snode_tree`. The 1024 ceiling is
therefore a ceiling on *cumulative* SNodes created over the process lifetime,
not on live SNodes.

Contrast with SNode **tree** ids, which ARE recycled:
`taichi/program/program.cpp:559-567` `allocate_snode_tree_id()` pops from
`free_snode_tree_ids_` (`program/program.h:336`, a `std::stack<int>`), pushed
by `destroy_snode_tree` at `program/program.cpp:235`.

## N07 — Surprise #4: the assertion does not guard the arrays it appears to guard

`taichi/codegen/llvm/struct_llvm.cpp:266`:
`TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`

`snodes` is `StructCompiler::snodes` (`taichi/struct/struct.h:106`), filled by
`StructCompiler::collect_snodes` (`taichi/struct/struct.cpp:128-134`) from the
root passed to `run()` (`struct_llvm.cpp:247-250`). A fresh
`StructCompilerLLVM` is constructed **per tree** at
`taichi/runtime/program_impls/llvm/llvm_program.cpp:50-52`.

So the assertion bounds the **per-tree** SNode count.

But the runtime arrays are indexed by the **global** `SNode::id`:
- `runtime.cpp:1005` `runtime->element_lists[i]` for `i` in
  `[root_id, root_id + num_snodes)` where `root_id` is `tree->root()->id`
  (`llvm_program.cpp:60`, `FieldCacheData::root_id`).
- `runtime.cpp:1029` `runtime->node_allocators[snode_id]`.
- `runtime.cpp:1038` `runtime->ambient_elements[snode_id]`.

Therefore with several trees, or with repeated create/destroy, global ids pass
1024 while every individual tree stays far under the assert. The write is then
out of bounds and nothing checks it. Recording as a finding, not resolving it —
the fix is a judgement call. Escalation.

## N08 — SNodeTree lifecycle

`taichi/struct/snode_tree.h:10-44`. The class is thin: an `int id_` and a
`unique_ptr<SNode> root_`. The header comment at line 13 says "An SNodeTree
will be backed by a contiguous chunk of memory" — that chunk is NOT owned by
this class. It is allocated in the LLVM runtime executor; see N10.
`check_tree_validity` (`snode_tree.cpp:76-86`) only checks that non-place,
non-root nodes have children.

`get_snodes_to_root_id` (`snode_tree.cpp:88-93`) returns
`unordered_map<int,int>` — **snode id -> root snode id**, both `int`. Another
32-bit id map.

Program side:
- `Program::add_snode_tree` `program/program.cpp:238-255`.
- `Program::destroy_snode_tree` `program/program.cpp:214-236`.
- `Program::allocate_snode_tree_id` `program/program.cpp:559-567`.
- `Program::get_snode_root(int tree_id)` `program/program.cpp:257-259` — raw
  `snode_trees_[tree_id]`, unchecked.
- `Program::get_snode_tree_size` `program/program.cpp:269-271` returns `int`
  from a `size_t`.
- Storage: `program/program.h:335-336`.

No bound anywhere against `kMaxNumSnodeTreesLlvm = 512`.

## N09 — ListManager: the footprint that dwarfs the 24 KiB. Surprise #5.

`runtime.cpp:426-511`, `struct ListManager`.

```
runtime.cpp:427:  static constexpr std::size_t max_num_chunks = 128 * 1024;
runtime.cpp:428:  Ptr chunks[max_num_chunks];
```

`sizeof(ListManager)`:

| Field | Line | Bytes |
|---|---|---|
| `chunks[131072]` | 428 | 1048576 |
| `element_size` (size_t) | 429 | 8 |
| `max_num_elements_per_chunk` (size_t) | 430 | 8 |
| `log2chunk_num_elements` (i32) | 431 | 4 |
| `lock` (i32) | 432 | 4 |
| `num_elements` (i32) | 433 | 4 (+4 pad) |
| `runtime` (ptr) | 434 | 8 |

= **1,048,616 bytes, just over 1 MiB each.**

`runtime.cpp:1005-1006` creates one ListManager per SNode in a non-all-dense
tree. `runtime.cpp:640-646` (NodeManager ctor) creates **three** more per
gc-able SNode (`free_list`, `recycled_list`, `data_list`).

That is the real cost of the SNode ceiling, and it is roughly 1000x the 24 KiB
of pointer tables the brief points at. Arithmetic in the report.

Also note the existing upstream TODO at `runtime.cpp:424-425`:
"there are many i32 types in this class, which may be an issue if there are
>= 2 ** 31 elements." Upstream already knew. Nobody acted.

## N10 — The tree's contiguous chunk

`SNodeTreeBufferManager`, `taichi/runtime/llvm/snode_tree_buffer_manager.{h,cpp}`.
- `allocate(size, snode_tree_id, result_buffer)` `snode_tree_buffer_manager.cpp:12-18`
  — one `DeviceAllocation` per tree, keyed in
  `std::map<int, DeviceAllocation> snode_tree_id_to_device_alloc_`
  (`snode_tree_buffer_manager.h:28`). Key is `int`.
- `destroy(SNodeTree*)` `snode_tree_buffer_manager.cpp:20-24`.

Caller: `LlvmRuntimeExecutor::initialize_llvm_runtime_snodes`
`taichi/runtime/llvm/llvm_runtime_executor.cpp:391-469`.
- `rounded_size = iroundup(root_size, taichi_page_size)` line 417. `size_t`, OK.
- `snode_tree_allocs_[tree_id] = alloc;` line 440.
- The JIT call, lines 442-444:
  `runtime_jit->call<void *, std::size_t, int, int, int, std::size_t, Ptr>(
     "runtime_initialize_snodes", llvm_runtime_, root_size, root_id,
     (int)snode_metas.size(), tree_id, rounded_size, root_buffer, all_dense);`
  `root_id`, `num_snodes`, `tree_id` all crossed as **i32**.
- `runtime_NodeAllocator_initialize` line 460-462, `snode_id` as **int**.
- `runtime_allocate_ambient` line 465-466, `snode_id` as **int**.

Matching device-side signatures:
`runtime.cpp:985-992` `runtime_initialize_snodes(..., const int root_id,
const int num_snodes, const int snode_tree_id, ...)`;
`runtime.cpp:1026-1030` `runtime_NodeAllocator_initialize(..., int snode_id, ...)`;
`runtime.cpp:1034-1039` `runtime_allocate_ambient(..., int snode_id, ...)`.

These three signatures are the host/device ABI for SNode ids. Any width change
to SNode ids must change both sides together, and the JIT `call<...>` template
argument list at `llvm_runtime_executor.cpp:442` is where the host side is
spelled.

## N11 — 32-bit inventory, device runtime (runtime.cpp and node_*.h)

Read all of `runtime.cpp` in blocks plus the five `node_*.h`. Collected:

**Identity / index carriers**
- `runtime.cpp:308` `StructMeta::snode_id` is `i32`. This is how a kernel's
  metadata names the SNode it indexes into `element_lists` / `node_allocators`
  / `ambient_elements`.
- `runtime.cpp:289` `PhysicalCoordinates::val[taichi_max_num_indices]` is
  `i32[12]`. The physical coordinate itself is 32-bit per axis.
- `runtime.cpp:521` `Element::loop_bounds[2]` is `int[2]`.
- `runtime.cpp:310` `StructMeta::max_num_elements` is `i64` — but see below.

**Truncating returns (i64 source, i32 return)** — `max_num_elements` is `i64`
yet every `get_num_elements` returns `i32`:
- `runtime.cpp:318` the function-pointer type `i32 (*get_num_elements)(Ptr, Ptr)`
- `node_dense.h:10-12` `Dense_get_num_elements` returns `((StructMeta*)meta)->max_num_elements` as i32
- `node_pointer.h:10-12` `Pointer_get_num_elements` same
- `node_bitmasked.h:10-12` `Bitmasked_get_num_elements` same
- `node_root.h:21-23` `Root_get_num_elements` returns literal 1
- `node_dynamic.h:116-119` `Dynamic_get_num_elements` returns `node->n` (i32)

**Pointer arithmetic done in `int`, not `size_t`** — `node_pointer.h`:
- `:44` `volatile Ptr lock = node + 8 * i;`
- `:45` `volatile Ptr *data_ptr = (Ptr *)(node + 8 * (num_elements + i));`
- `:69`, `:70` same in `Pointer_deactivate`
- `:86` same in `Pointer_is_active`
- `:92` same in `Pointer_lookup_element`
`8 * i` is `int * int`. Overflows at i >= 2^28 (268435456). Both `i` and
`num_elements` are 32-bit here, so `num_elements + i` can also overflow before
the multiply. Flagging; not resolving.

**ListManager**, `runtime.cpp:426-511`, with upstream's own TODO at 424:
- `:431` `log2chunk_num_elements` i32, `:432` `lock` i32, `:433` `num_elements` i32
- `:451` `reserve_new_element()` returns `i32`, `atomic_add_i32(&num_elements, 1)`
- `:463` `get_num_active_chunks()` i32
- `:472` `resize(i32 n)`
- `:475` `get_element_ptr(i32 i)`; the offset expression
  `chunks[i >> log2chunk_num_elements] + element_size * (i & ((1 << log2chunk_num_elements) - 1))`
  — `element_size` is `std::size_t` so that multiply is 64-bit, but the index
  `i` and the shift are 32-bit
- `:481` `T &get(i32 i)`, `:485` `touch_and_get(i32 i)`, `:490` `size()` i32
- `:493` `ptr2index(Ptr ptr)` returns `i32`, and the composed index
  `(i << log2chunk_num_elements) + i32((ptr - chunks[i]) / element_size)`
  at `:497-498` is built in 32 bits
- `runtime.cpp:1662` `ListManager::touch_chunk(int chunk_id)`

Capacity implied: `max_num_chunks` 131072 x `num_elements_per_chunk`. For
element lists that is 131072 x 65536 = 2^33, which already exceeds what the
i32 `num_elements` can count. The chunk array is sized for more elements than
the counter can address.

**NodeManager**, `runtime.cpp:630-...`:
- `:632-636` `lock`, `element_size`, `chunk_num_elements`, `free_list_used` all `i32`
- `:639` `recycle_list_size_backup` i32
- `:641` `using list_data_type = i32;` — the free/recycled lists store i32 element indices
- `:643-645` ctor takes `i32 element_size`, `i32 chunk_num_elements`
- `:669` `allocate()` uses `int old_cursor = atomic_add_i32(&free_list_used, 1)`
- `:681` `i32 locate(Ptr ptr)`
- host passes `std::size_t node_size` (`runtime.cpp:1028`) into the `i32
  element_size` parameter — narrowing at the `create<NodeManager>` call
  `runtime.cpp:1029-1030`.

**Listgen and struct-for**, all loop indices `int`:
- `runtime.cpp:1281-1327` `element_listgen_root`: `c_start`, `c_step`, `c`,
  `ch_num_elements`, `ch_element_size`, and `c * ch_element_size` at `:1317` and
  `(c + 1) * ch_element_size` at `:1321` — 32-bit multiplies
- `runtime.cpp:1329-1379` `element_listgen_nonroot`: `num_parent_elements`,
  `i_start/i_step/j_start/j_step`, `i`, `j`, `j_lower`, `j_higher`, `ch_lower`
- `runtime.cpp:1384` `using BlockTask = void(RuntimeContext *, char *, Element *, int, int);`
  — the lower/upper bounds handed to every generated struct-for body are `int`
- `runtime.cpp:1386-1393` `cpu_block_task_helper_context`: `element_size`,
  `element_split` int
- `runtime.cpp:1403-1421` `cpu_struct_for_block_helper`: `element_id`,
  `part_size`, `part_id`, `lower`, `upper` int; `part_id * part_size` 32-bit
- `runtime.cpp:1423-1430+` `parallel_struct_for(RuntimeContext*, int snode_id,
  int element_size, int element_split, ...)` — **snode_id crosses as `int`**
- `runtime.cpp:1692` `node_gc(LLVMRuntime *runtime, int snode_id)`
- `runtime.cpp:1723`, `:1740`, `:1784` gc_parallel_impl_{0,1,2} paths index
  `node_allocators[snode_id]`

**Accessor macros** — `runtime.cpp:67-77` `STRUCT_FIELD_ARRAY(S, F)` generates
`S##_get_##F(S *s, int i)` and `S##_set_##F(S *s, int i, ...)`. Applied to
`element_lists`, `node_allocators`, `roots`, `root_mem_sizes`
(`runtime.cpp:616-619`) and to `PhysicalCoordinates::val` (`:292`) and
`Element::loop_bounds` (`:524`). `runtime.cpp:85-88`
`RUNTIME_STRUCT_FIELD_ARRAY` likewise takes `int i`; applied at `:749-750`.
So the accessor ABI for the SNode-indexed tables is `int` on both sides.

**64-bit clean in the runtime** (verified, listing so it is not re-derived):
- `runtime.cpp:823-835` `allocate_aligned` — `std::size_t size, alignment`
- `runtime.cpp:838-874` `allocate_from_reserved_memory` — `std::size_t`
  throughout, head/tail cast to `std::size_t`
- `runtime.cpp:552-554` `PreallocatedMemoryChunk::preallocated_size` `std::size_t`
- `runtime.cpp:563` `root_mem_sizes` is `size_t[]`
- `runtime.cpp:597` `total_requested_memory` `i64`, `atomic_add_i64` at `:828`

## N12 — 32-bit inventory, host side of my territory

**`taichi/runtime/llvm/llvm_runtime_executor.{h,cpp}`**
- `llvm_runtime_executor.h:97` `DevicePtr get_snode_tree_device_ptr(int tree_id);`
- `llvm_runtime_executor.h:152` `std::unordered_map<int, DeviceAllocation> snode_tree_allocs_;`
- `llvm_runtime_executor.h:87` `T fetch_result(int i, uint64 *result_buffer)`
- `llvm_runtime_executor.h:133` `uint64 fetch_result_uint64(int i, ...)`
- `llvm_runtime_executor.cpp:399-400` `const int tree_id`, `const int root_id`
- `llvm_runtime_executor.cpp:442-444` the JIT call, already noted in N10
- `llvm_runtime_executor.cpp:460-466` `snode_id` passed as int twice
- `llvm_runtime_executor.cpp:188-209` `print_list_manager_info` queries
  `ListManager_get_num_elements`, `_get_element_size`,
  `_get_max_num_elements_per_chunk`, `_get_num_active_chunks` all as `int32`.
  Note `_get_element_size` and `_get_max_num_elements_per_chunk` are
  `std::size_t` on the device (`runtime.cpp:429-430`) but are fetched as
  `int32` here — a host/device width mismatch that already exists.
- `llvm_runtime_executor.cpp:257-269` `get_snode_num_dynamically_allocated`
  returns `std::size_t` but the underlying query is `int32`
- `llvm_runtime_executor.cpp:322-324`, `:337-339` memory profiler passes
  `snode->id` to `LLVMRuntime_get_element_lists` / `_get_node_allocators`
- `llvm_runtime_executor.cpp:642-653` `int num_rand_states`;
  `num_rand_states = config_.saturating_grid_dim * config_.max_block_dim` at
  `:648` — a 32-bit multiply feeding
  `sizeof(RandState) * num_rand_states` at `runtime.cpp:902-903`
- `llvm_runtime_executor.cpp:676-677` `call<void *, int32_t, int32_t>(
  "runtime_get_memory_requirements", ...)`
- `llvm_runtime_executor.cpp:637` `int starting_rand_state = config_.random_seed * 1048391;` (32-bit multiply)

**`taichi/runtime/llvm/llvm_offline_cache.h`** — and these are SERIALIZED:
- `:68` `SNodeCacheData::id` is `int`; `:73` `TI_IO_DEF(id, type, cell_size_bytes, chunk_size)`
- `:76-77` `FieldCacheData::tree_id`, `root_id` are `int`; `:81` `TI_IO_DEF(...)`
- `:119` `std::unordered_map<int, FieldCacheData> fields; // key = snode_tree_id`
Changing the id width changes the on-disk offline cache format. Escalation
candidate — the cache version bump is not mine to decide.

**`taichi/runtime/llvm/llvm_context.{h,cpp}`** — struct modules keyed by tree id:
- `llvm_context.h:57` `add_struct_module(..., int tree_id)`
- `llvm_context.h:114` `get_struct_function(const std::string &name, int tree_id)`
- `llvm_context.cpp:671-693`, `:999-1001`, `:1036-1058` (`used_tree_ids` is
  `std::unordered_set<int>`)

**`taichi/runtime/llvm/snode_tree_buffer_manager.h:28`**
`std::map<int, DeviceAllocation> snode_tree_id_to_device_alloc_;`

**`taichi/runtime/program_impls/llvm/llvm_program.{h,cpp}`**
- `llvm_program.cpp:58-64` `compile_snode_tree_types` — `int snode_tree_id`, `int root_id`
- `llvm_program.cpp:66-75` `materialize_snode_tree` — `int snode_tree_id`
- `llvm_program.cpp:93-119` `cache_field(int snode_tree_id, int root_id, ...)`,
  `snode_cache_data.id = snodes[i]->id;` at `:110`
- `llvm_program.cpp:53` `++num_snode_trees_processed_;`

**`taichi/program/program.{h,cpp}`**
- `program.h:115` `int get_snode_tree_size();`
- `program.h:189` `destroy_snode_tree(SNodeTree *)`
- `program.h:204` `add_snode_tree(...)`
- `program.h:214` `int allocate_snode_tree_id();`
- `program.h:232-234` `get_snode_tree_device_ptr(int tree_id)`
- `program.h:335-336` `snode_trees_`, `free_snode_tree_ids_` (`std::stack<int>`)
- `program.cpp:279-280`, `:291` — the SNode **reader** kernel hard-codes
  `PrimitiveType::i32` for every index argument, and
  `ker.insert_scalar_param(PrimitiveType::i32)`
- `program.cpp:305-306`, `:315`, `:322` — same in the SNode **writer**
- `program.cpp:330` `uint64 Program::fetch_result_uint64(int i)`
- `program/snode_rw_accessors_bank.cpp:8-14` `set_kernel_args(const
  std::vector<int> &I, ...)` and `launch_ctx->set_arg_int({i}, I[i])` — the host
  read/write path for a field carries `int` per-axis coordinates
- `program/snode_rw_accessors_bank.cpp:38,49,61,73,84,95` all take
  `const std::vector<int> &I`
- `program/snode_expr_utils.{h,cpp}` `place_child(..., const std::vector<int>
  &offset, int id_in_bit_struct, ...)` — `snode_expr_utils.cpp:52-56`

**`taichi/struct/snode_tree.{h,cpp}`**
- `snode_tree.h:17` `constexpr static int kFirstID = 0;`
- `snode_tree.h:25` `explicit SNodeTree(int id, ...)`; `:27` `int id() const`;
  `:40` `int id_{0}`
- `snode_tree.h:52` / `snode_tree.cpp:60-67,88-93`
  `std::unordered_map<int,int> get_snodes_to_root_id(...)`

**`taichi/struct/struct.h:107`** `std::size_t root_size{0};` — 64-bit, clean.

## N13 — Tree destruction does not clear the device tables. Surprise #6.

`LlvmRuntimeExecutor::destroy_snode_tree`
`taichi/runtime/llvm/llvm_runtime_executor.cpp:758-761`:

```
get_llvm_context()->delete_snode_tree(snode_tree->id());
snode_tree_buffer_manager_->destroy(snode_tree);
```

That is the whole of it. It does NOT:
- null `runtime->element_lists[i]` for the tree's SNode ids
- null `runtime->node_allocators[i]` or `runtime->ambient_elements[i]`
- null `runtime->roots[tree_id]` / `root_mem_sizes[tree_id]`
- free the `ListManager` / `NodeManager` objects created for those SNodes.
  They came from `allocate_aligned(runtime_memory_chunk, ...)`
  (`runtime.cpp:606-612` `LLVMRuntime::create<T>`), which on GPU is the bump
  allocator `allocate_from_reserved_memory` (`runtime.cpp:838-874`). There is
  no free path in it at all.
- erase `snode_tree_allocs_[tree_id]` (`llvm_runtime_executor.cpp:440`, only
  ever assigned; read at `:387`)

Combined with N06 (SNode ids never recycled) and N05 (no bound on
`kMaxNumSnodeTreesLlvm`), the create/destroy cycle walks the SNode id counter
upward, leaves stale pointers behind it, and never reclaims the 1 MiB
ListManagers. This matters directly to 4.6 in the brief: an SNode configuration
"composed from the larger data stores for working scope" implies recomposition,
which means repeated tree creation.

Escalation. I am not proposing a fix.

## N14 — gfx / SPIR-V side has no 1024 array

`taichi/runtime/gfx/snode_tree_manager.{h,cpp}` and
`taichi/runtime/gfx/runtime.cpp:730-761`. Root buffers are a
`std::vector<std::unique_ptr<DeviceAllocationGuard>> root_buffers_`
(`gfx/runtime.h:152`), grown by `add_root_buffer` (`gfx/runtime.cpp:730-750`).
No fixed-size table, so `taichi_max_num_snodes` does not bind here.

But `gfx::SNodeTreeManager::destroy_snode_tree`
(`snode_tree_manager.cpp:18-29`) only calls `root_buffers_[root_id].reset()`.
It does not erase from `compiled_snode_structs_`, which is only ever
`push_back`-ed (`snode_tree_manager.cpp:15`). Meanwhile `Program` recycles tree
ids (`program/program.cpp:559-567`). So `compiled_snode_structs_[tree_id]` at
`snode_tree_manager.cpp:33` and `root_buffers_[tree_id]` at `:50` index by
push-back order while the id they are given is a recycled Program id. These
diverge after the first destroy. Recording; not resolving. Escalation.

`gfx/runtime.h:40` and `:94` have `std::size_t num_snode_trees{0};` — 64-bit
there, unlike the LLVM path.

## N15 — Verified 64-bit-clean, so it is not re-derived

- `runtime.cpp:823-874` allocation path, `std::size_t` throughout
- `runtime.cpp:891-906` `runtime_get_memory_requirements` uses `i64 size` and
  `taichi::iroundup(i64(sizeof(LLVMRuntime)), taichi_page_size)` at `:897`
- `struct/struct.h:107` `std::size_t root_size`
- `llvm_offline_cache.h:70-71` `cell_size_bytes`, `chunk_size` are `size_t`
- `gfx/runtime.cpp:730` `add_root_buffer(size_t root_buffer_size)`
- `ir/snode.h:100` `int64 num_cells_per_container{1}`,
  `ir/snode.h:102-103` `std::size_t cell_size_bytes`,
  `offset_bytes_in_parent_cell` — note `ir/snode.h:101` `int chunk_size{0}` is
  32-bit, and `ir/snode.h:306` `int64 max_num_elements()`.

## N16 — Build mechanics for making 1024 an install-time parameter

`taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15`:

```
COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp
        -o "runtime_${rtm_arch}.bc" -fno-exceptions -emit-llvm -std=c++17
        -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
```

Points that bear on 6.1:

1. `runtime.cpp` is compiled to bitcode by a **separate clang invocation**
   under `add_custom_target`, not by the normal C++ target. It receives exactly
   two flags of interest: `-D ARCH_<arch>` and `-I ${PROJECT_SOURCE_DIR}`. It
   inherits **none** of the project's `target_compile_definitions`. Any
   build-parameter mechanism for `taichi_max_num_snodes` must be added here
   explicitly, or must live in a generated header on the include path.
2. Output goes into the **source** directory (`WORKING_DIRECTORY` is
   `CMAKE_CURRENT_SOURCE_DIR`), acknowledged by the TODO at line 9. Line 13
   installs it to `python/taichi/_lib/runtime`.
3. One bitcode per arch: `foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH
   AMDGPU_ARCH)` at line 29-31. Loaded by name at `llvm_context.cpp:209-211`
   `get_runtime_fn(Arch)` -> `runtime_<arch>.bc`, read from `runtime_lib_dir()`
   at `llvm_context.cpp:362`.
4. `cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` is a **checked-in
   prebuilt** artifact (`CMakeLists.txt:17-26` is the generator and it is
   commented out at `:36-42`). Linked at `llvm_context.cpp:580-588`. It does
   not reference the SNode arrays, but it is pinned to sm_60, which is Pascal.
   The GTX 750 is sm_50. Flagging for 04/backend-build, not resolving.

**Layout is confined to the bitcode.** `LLVMRuntime` is only forward-declared
on the host (`taichi/program/context.h:11`, `taichi/rhi/llvm/llvm_device.h:8`).
The host never takes `sizeof(LLVMRuntime)` itself; it asks the bitcode via
`runtime_get_memory_requirements` (`llvm_runtime_executor.cpp:676-679` calling
`runtime.cpp:891-906`), and reads fields only through the generated
`LLVMRuntime_get_*` accessors. Codegen obtains the type by name at
`codegen/llvm/codegen_llvm.cpp:2697` `get_runtime_type("LLVMRuntime")`.

Consequence: changing the constant is layout-safe **provided the .bc is
rebuilt with the same value the host C++ was built with**. The two places the
host C++ itself uses the constant are `struct_llvm.cpp:266` (the assert) and
nothing else. A stale .bc paired with a fresh host binary would silently
disagree only through that assert, which as N07 shows is not checking the thing
it looks like it checks.

## N17 — Range-for and struct-for loop indices are i32 end to end

- `runtime.cpp:42` `using RangeForTaskFunc = void(RuntimeContext *, const char *tls, int i);`
- `runtime.cpp:43` `using MeshForTaskFunc = void(RuntimeContext *, const char *tls, uint32_t i);`
- `runtime.cpp:44-48` `parallel_for_type` — `int splits`, `int num_desired_threads`,
  and the callback `void (*)(void *, int thread_id, int i)`
- `runtime.cpp:1384` `using BlockTask = void(RuntimeContext *, char *, Element *, int, int);`
- `runtime.cpp:1471-1479` `range_task_helper_context`: `int begin, end, block_size, step`
- `runtime.cpp:1483-1508` `cpu_parallel_range_for_task`: `block_start`,
  `block_end` int; `ctx.begin + task_id * ctx.block_size` at `:1494` is a
  32-bit multiply
- `runtime.cpp:1510-1535` `cpu_parallel_range_for(..., int begin, int end, int step, int block_dim, ...)`
- `runtime.cpp:1537-1560` `gpu_parallel_range_for(..., int begin, int end, ...)`;
  `int idx = thread_idx() + block_dim() * block_idx() + begin;` at `:1541`
- `runtime.cpp:1563-1570` `mesh_task_helper_context`: `int num_patches, block_size`
- `runtime.cpp:1596-1624` `cpu_parallel_mesh_for(..., int num_patches, int block_dim, ...)`
- `runtime.cpp:1626-1646` `gpu_parallel_mesh_for(..., int num_patches, ...)`
- `runtime.cpp:1648-1654` `i32 linear_thread_idx(RuntimeContext *)` — used to
  index `rand_states` at `runtime.cpp:1786-1788`

Every generated kernel body therefore receives its loop variable as a 32-bit
`int` from the runtime, on both CPU and GPU. This is the runtime end of item
6.2 and it is not optional: raising SNode counts without raising this caps the
element count a single for-loop can address at 2^31.

## N18 — The IR-side statement of the same limit (seam with agent 01)

`taichi/ir/snode.cpp:89-100`:

```
int64 acc_shape = 1;
for (int i = taichi_max_num_indices - 1; i >= 0; i--) {
  // casting to int32 in extractors.
  new_node.extractors[i].acc_shape = static_cast<int>(acc_shape);
  acc_shape *= new_node.extractors[i].shape;
}
if (acc_shape > std::numeric_limits<int>::max()) {
  ErrorEmitter(TaichiIndexWarning(), &dbg_info,
    "SNode index might be out of int32 boundary but int64 indexing is not "
    "supported yet. Struct fors might not work either.");
}
```

It is a **warning**, not an error. `AxisExtractor::acc_shape`,
`::shape`, `::num_elements_from_root` are all `int` (`ir/snode.h:41,45,49`),
while `SNode::num_cells_per_container` is `int64` (`ir/snode.h:97`). Not my
territory to fix; recording because `PhysicalCoordinates::val` (`runtime.cpp:289`)
is the runtime side of exactly this.

## N19 — VERIFIED the sizes by compiling, not by hand

I did not create any file. I piped `runtime.cpp` plus appended `static_assert`s
into clang via stdin with `-fsyntax-only`:

```
(cat taichi/runtime/llvm/runtime_module/runtime.cpp; printf '...static_asserts...') \
  | clang++ -x c++ - -std=c++17 -fno-exceptions \
    -I /opt/project/taichi \
    -I /opt/project/taichi/taichi/runtime/llvm/runtime_module -fsyntax-only -w
```

All of the following **compiled clean**, i.e. are verified facts on x86-64 with
the tree's real declarations:

- `sizeof(LLVMRuntime) == 35256`
- `offsetof(LLVMRuntime, element_lists) == 8296`
- `offsetof(LLVMRuntime, node_allocators) == 16488`
- `offsetof(LLVMRuntime, ambient_elements) == 24680`
- `sizeof(ListManager) == 1048616`
- `sizeof(NodeManager) == 56`
- `sizeof(Element) == 64`
- `sizeof(PhysicalCoordinates) == 48`
- `sizeof(StructMeta) == 72`
- `sizeof(RandState) == 20`

(An `iroundup` static_assert failed only because `taichi::iroundup` at
`taichi/math/arithmetic.h:13` is not `constexpr`. 35256 / 4096 = 8.607, so
9 pages = 36864 by hand.)

Derived closed form for the runtime struct, with N = `taichi_max_num_snodes`
and T = `kMaxNumSnodeTreesLlvm`:

`sizeof(LLVMRuntime) = 2488 + 24*N + 16*T`

Check at N=1024, T=512: 2488 + 24576 + 8192 = 35256. Matches the verified value.

## N20 — Where the real cost is: element lists are per-SNode, including `place`

`runtime.cpp:1002-1016`:

```
if (all_dense) { return; }
for (int i = root_id; i < root_id + num_snodes; i++) {
  // TODO: some SNodes do not actually need an element list.
  runtime->element_lists[i] =
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

`num_snodes` is `(int)snode_metas.size()` (`llvm_runtime_executor.cpp:444`),
i.e. **every** SNode in the tree, `place` nodes included — upstream's own TODO
at `:1004` says so. `all_dense` is computed at
`llvm_runtime_executor.cpp:402-410`: false as soon as any SNode is not dense,
place or root. Given the brief's 4.1 "Sparsity is required", `all_dense` is
false for the intended workload, so every SNode in the tree pays a full
`ListManager`.

`create<T>` (`runtime.cpp:606-612`) calls
`allocate_aligned(runtime_memory_chunk, sizeof(T), 4096, request=true)`, so the
full 1048616 bytes is committed eagerly at materialization, page-aligned. Only
the *chunks* inside it are lazy (`ListManager::touch_chunk`,
`runtime.cpp:1662-1677`).

`node_allocators` and `ambient_elements` are created only for gc-able SNodes:
`llvm_runtime_executor.cpp:447` `is_gc_able(snode_metas[i].type)`, defined at
`taichi/ir/snode_types.cpp:21-23` as `pointer || dynamic`. Each such SNode gets
a `NodeManager` (56 B) which itself creates **three** ListManagers in its ctor
(`runtime.cpp:641-646`), so ~3 MiB more per pointer/dynamic SNode, plus the
ambient element (`runtime.cpp:1034-1039`, size = the cell size).

This is the arithmetic that actually decides whether the GTX 750 can hold a
configuration, and it is 1000x the 24 KiB the brief points at. Escalation:
whether the ListManager chunk-array size is in scope for 6.1 is a judgement
call I am not making.

## N21 — Correction to N03 line numbers

Re-checked `sed -n '552,600p'` with an offset print. `struct LLVMRuntime {` is
on line **552**, not 551. Two rows in the N03 table were off by one:
`runtime_objects_chunk` is line **553** (I wrote 552) and `runtime_memory_chunk`
is line **554** (I wrote 553). Every other line number in N03 checks out
against the grep in N05 and the offset print: roots 562, root_mem_sizes 563,
thread_pool 565, parallel_for 566, element_lists 567, node_allocators 568,
ambient_elements 569, temporaries 570, rand_states 571, profiler 583,
profiler_start 584, profiler_stop 585, error_message_template 587,
error_message_arguments 588, error_message_lock 589, error_code 590,
result_buffer 592, allocator_lock 593, num_rand_states 595,
total_requested_memory 597. Struct closes at 613.

The byte offsets and the verified `sizeof` in N19 are unaffected.

## N22 — Full inventory of fixed-size arrays sized by an `inc/constants.h` value

`grep -rn "\[taichi_max_num\|\[kMaxNum"` over `taichi/`, build excluded:

- `ir/snode.h:78` `AxisExtractor extractors[taichi_max_num_indices]`
- `ir/snode.h:81` `int physical_index_position[taichi_max_num_indices]`
- `runtime.cpp:289` `i32 val[taichi_max_num_indices]` (PhysicalCoordinates)
- `runtime.cpp:562-563` `roots`, `root_mem_sizes` at `kMaxNumSnodeTreesLlvm`
- `runtime.cpp:567-569` the three at `taichi_max_num_snodes`

That is all of them. `taichi_max_num_snodes` sizes exactly three arrays, and
they are all in my territory. Related loop bounds using
`taichi_max_num_indices`: `ir/snode.cpp:21,90,105`,
`runtime.cpp:1012`, `codegen/llvm/struct_llvm.cpp:171`,
`analysis/offline_cache_util.cpp:110`, `python/export_lang.cpp:559`,
`transforms/demote_dense_struct_fors.cpp:19,23`,
`transforms/scalar_pointer_lowerer.cpp:33,36,40`.
`python/export_lang.cpp:1222` exports `get_max_num_indices` to Python — out of
scope per brief 1.2, but it is a C++ binding that reads the constant.

## N23 — Codegen seam: how SNode ids reach the runtime tables

Not my territory to change, but these are the call sites that must move in step
with any width change to SNode ids:
- `codegen/llvm/codegen_llvm.cpp:1151-1152` `emit_gc` — `call("node_gc",
  get_runtime(), tlctx->get_constant(snode))` where `snode` is `int`
- `codegen/llvm/codegen_llvm.cpp:2295-2308` `parallel_struct_for` call with
  `tlctx->get_constant(leaf_block->id)`
- `codegen/llvm/codegen_llvm.cpp:2689-2691` `get_root(int snode_tree_id)` ->
  `call("LLVMRuntime_get_roots", get_runtime(), tlctx->get_constant(snode_tree_id))`
- `codegen/cuda/codegen_cuda.cpp:575,584,593` and
  `codegen/amdgpu/codegen_amdgpu.cpp:282,291,300` — `gc_parallel_{0,1,2}` with
  `snode_id`
- `codegen/llvm/codegen_llvm.cpp:2289-2290`
  `int list_element_size = std::min(leaf_block->max_num_elements(),
  (int64)taichi_listgen_max_element_size);` — an `int64` narrowed to `int`,
  though bounded by 1024 so harmless today

`tlctx->get_constant(int)` produces an i32 LLVM constant, matching the `int`
parameters on the runtime side. Consistent today. Would have to change in
lockstep.

## N24 — Done exploring. Writing the report.

## N25 — Line-number audit of the finished report

After drafting the report I re-extracted every `file:line` citation from it and
printed the corresponding source line, to catch transcription drift. Twenty-odd
citations were off by between one and nine lines and were corrected in place.
The audit found no factual error, only citation drift. Corrections applied:

Off-by-a-few in `runtime.cpp`, from my earlier `grep -A`/`sed` reads where I
recorded the anchor line rather than the definition line:
`RangeForTaskFunc` 42->43, `MeshForTaskFunc` 43->44, `parallel_for_type`
44-48->45-49, `STRUCT_FIELD_ARRAY` 67-77->69-77, `get_num_active_chunks`
463->467, `resize` 472->479, `get_element_ptr` 475->483, `get` 480->488,
`touch_and_get` 485->493, `size` 490->498, `ptr2index` 493->502 (composed index
497-498 -> 507-508), ListManager span 426-511 -> 426-513, NodeManager
`allocate` cursor 669->667, `locate` 681->679, `gc_serial` loops 689,698 ->
690,699, NodeManager's three ListManager creates 641-646 -> **658-663**,
`runtime_initialize_snodes` 985->986, `runtime_allocate_ambient` 1034->1033,
element list block 1002-1016 -> **1000-1016**, `element_listgen_root`
1281->1282 (`c * ch_element_size` 1317->1322, `(c+1)*` 1321->1323),
`element_listgen_nonroot` 1329->1331, `BlockTask` 1384->1385,
`cpu_block_task_helper_context` 1386-1392 -> 1387-1394, `parallel_struct_for`
1423->1422, `range_task_helper_context` end 1479->1481,
`cpu_parallel_range_for_task` multiply 1494->1495, `cpu_parallel_range_for`
1510->1511, `gpu_parallel_range_for` 1537->1541 (idx line 1541->1548),
`mesh_task_helper_context` 1563-1570 -> 1567-1575, `cpu_parallel_mesh_for`
1596->1599, `gpu_parallel_mesh_for` 1626->1627, `linear_thread_idx` 1648->1650,
`ListManager::touch_chunk` 1662->1664, `node_gc` 1692->1691, `gc_parallel_*`
1720,1736,1779 -> **1721,1738,1782**, `rand_states` index 1786-1788 ->
1790-1792, stack ops 1875-1893 -> 1876-1898.

Larger errors, caused by my having read `struct/` via a **concatenated**
`cat` in N08 and recording the concatenated numbering rather than per-file
numbering:
- `struct/struct.h:106,107` -> **`struct/struct.h:11,12`**
- `struct/struct.cpp:128-134` -> **`struct/struct.cpp:7-13`**
- `snode_tree.cpp:60-67` -> **`snode_tree.cpp:6-12`**
- `snode_tree.cpp:76-86` -> **`snode_tree.cpp:22-32`**
- `snode_tree.cpp:88-93` -> **`snode_tree.cpp:34-39`**
`snode_tree.h` numbering was unaffected because it was first in the
concatenation. **Any citation in N08 to `struct/struct.{h,cpp}` or
`snode_tree.cpp` is wrong by this offset; the report is the corrected version.**

Also corrected: `ir/snode.h:101` -> `ir/snode.h:98` for `int chunk_size{0}`
(101 is `DataType dt;`); `llvm_program.cpp:44-56` -> `:45-56`;
`snode_tree_buffer_manager.h:22-24` -> `:20-22`.

Nothing in section 2 (the footprint arithmetic) depended on any of these; those
numbers came from the compiler in N19.

# --- REVISION PASS, after adversarial review ---

## N26 — What came back

Both adversaries compiled the runtime themselves. My ten `sizeof`/`offsetof`
figures and the closed form reproduced in both. What I got wrong:

1. The pool. I followed `allocate_aligned` into
   `allocate_from_reserved_memory` and stopped there. I never asked where
   `runtime_memory_chunk` comes from. It comes from
   `LlvmRuntimeExecutor::preallocate_runtime_memory`
   (`llvm_runtime_executor.cpp:607-632`), sized by `config_.device_memory_GB`,
   default **1**. My "the card is the binding limit" sentence is wrong.
2. I dropped the bump allocator's alignment charge, ~0.4% low throughout.
3. I called the chunks "lazy" and never priced one. They are 4 MiB.
4. I under-stated the leak instead of establishing reachability.
5. Residual citation drift in `llvm_program.{h,cpp}` and two lines in
   `llvm_runtime_executor.cpp`.

The two adversaries disagree with each other on the pool capacity figure (1020
vs 1023), on the `__assertfail` line span, on `TI_ASSERT(total_prealloc_size
<= total_mem)`'s line, on whether `ptr2index` is a full linear scan, and on the
2 GB-card-at-0.9 figures. I adjudicate each below from source. I read
adversary-03-1 and adversary-03-2 only; I did not open report-03 or notes-03.

## N27 — ADJUDICATED: the alignment charge. Adversary 03-1 is right.

`runtime.cpp:848-853`, read directly:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

Let A be the alignment and r = head mod A. For r = 0:
`(h + A - 1) mod A = A - 1`, so `alignment_bytes = A - 1 - (A-1) = 0`.
For r > 0: `(h + A - 1) mod A = r - 1`, so `alignment_bytes = A - 1 - (r-1)
= A - r`. So `alignment_bytes = (A - r) mod A`, ordinary round-up, and line
850 charges it **to the caller** before the fit test on 851 and before the head
advance on 853.

Consequence: from an aligned head, each request advances the head by
`roundup(S, A)`. `1048616 mod 4096 = 40`, so `roundup(1048616, 4096) = 257
pages = 1,052,672`. Cumulative for k identical ListManagers from an aligned
base is `1048616 + (k-1)*1052672` — the last one is not charged its trailing
slack until the next request arrives.

Adversary 03-2 (its section 7.1) omits this and gets 1023 lists per 1 GiB, and
headlines "the existing constant overruns the existing default by 40,960 bytes,
under one page over". That figure is `1024*1048616 - 2^30 = 40960`, which is
the unpadded arithmetic. **With the charge applied the overrun is 4,190,248
bytes, about four lists' worth, and capacity is 1020.** Adversary 03-1 is
correct on all three numbers. Worked below in N29.

The shared qualitative conclusion is unaffected either way: 1020 or 1023
against a constant of 1024 is a match to within 0.4%.

## N28 — ADJUDICATED: the pool, verified myself

`taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632`, read in full:

```
607: void LlvmRuntimeExecutor::preallocate_runtime_memory() {
611:   std::size_t total_prealloc_size = 0;
612:   const auto total_mem = llvm_device()->get_total_memory();
613:   if (config_.device_memory_fraction == 0) {
614:     TI_ASSERT(config_.device_memory_GB > 0);
615:     total_prealloc_size = std::size_t(config_.device_memory_GB * (1UL << 30));
616:   } else {
617:     total_prealloc_size =
618:         std::size_t(config_.device_memory_fraction * total_mem);
619:   }
620:   TI_ASSERT(total_prealloc_size <= total_mem);
629:   runtime_jit->call<void *, std::size_t, void *>(
630:       "runtime_initialize_memory", llvm_runtime_, total_prealloc_size,
631:       runtime_memory_prealloc_buffer);
```

`TI_ASSERT(total_prealloc_size <= total_mem)` is on **620**. Adversary 03-2 is
right, adversary 03-1 wrote 619 and later conceded it.

Defaults, `taichi/program/compile_config.cpp`, verified by offset print:
line 18 `demote_dense_struct_fors = true;`, line 63 `device_memory_GB = 1;`,
line 64 `device_memory_fraction = 0.0;`, lines 72-73 the SPIR-V force inside
`CompileConfig::fit()` (which begins at 67).

`get_total_memory` on CUDA is `CUDAContext::get_total_memory`
(`taichi/rhi/cuda/cuda_context.cpp:88-92`), which returns the **total** from
`mem_get_info`, not the free figure.

Call sites of `preallocate_runtime_memory`: `llvm_runtime_executor.cpp:413`
(inside `initialize_llvm_runtime_snodes`, guarded by
`config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense` at
`:412`) and `:721` (in `materialize_runtime`, guarded by
`!use_device_memory_pool()`). So on CUDA with a device memory pool, the runtime
memory chunk is **not created at all** unless a non-all-dense tree is
materialised. `all_dense` gates allocator existence, not merely footprint.

## N29 — The capacity arithmetic, worked

Pool at the default: 1 x 2^30 = **1,073,741,824** bytes.

Charges, from an aligned head (each `roundup(S, 4096)` except a trailing one):

| Allocation | Size | Charge |
|---|---|---|
| `ListManager` header | 1,048,616 | 1,052,672 (257 pages) |
| `NodeManager` | 56 | 4,096 |
| element-list first chunk | 4,194,304 | 4,194,304 (exact) |
| `free_list` / `recycled_list` first chunk | 65,536 | 65,536 (exact) |

Element-list chunk size: `touch_chunk` (`runtime.cpp:1664-1679`) allocates
`max_num_elements_per_chunk * element_size`. `runtime.cpp:1006` constructs with
`sizeof(Element)` and `1024 * 64`, and `sizeof(Element) == 64` (measured, N19),
so `65536 * 64 = 4,194,304`. Alignment 4096.

**(a) element-list headers only**
`cum(k) = 1048616 + (k-1)*1052672`
- k=1020: 1019*1052672 = 1,072,672,768; +1,048,616 = **1,073,721,384** <= pool (slack 20,440)
- k=1021: 1020*1052672 = 1,073,725,440; +1,048,616 = **1,074,774,056** > pool
- **capacity 1020**
- k=1024 would cost 1023*1052672 + 1048616 = **1,077,932,072**, overrunning the
  default pool by **4,190,248** bytes.

The 20,440 slack at k=1020 also means the answer is 1020 whether or not the
pool base happens to be 4096-aligned; a misaligned base costs at most 4055
extra bytes.

**(b) plain sparse SNode, header + its first chunk** = 1,052,672 + 4,194,304 =
**5,246,976**
- 204 x 5,246,976 = 1,070,383,104 <= pool; 205 x = 1,075,630,080 > pool
- **capacity 204**

**(c) pointer/dynamic SNode, headers only** = element list 1,052,672 +
`NodeManager` 4,096 + 3 x 1,052,672 = **4,214,784**
- 254 x 4,214,784 = 1,070,555,136 <= pool; 255 x = 1,074,769,920 > pool
- **capacity 254**. This is the corrected form of my withdrawn "512".

**(d) pointer SNode, headers + element chunk + the two i32 chunks**
(excluding the `data_list` chunk, which is workload-sized) =
4,214,784 + 4,194,304 + 65,536 + 65,536 = **8,540,160**
- 125 x 8,540,160 = 1,067,520,000 <= pool; 126 x = 1,076,060,160 > pool
- **capacity 125**

**(e) 2 GB GTX 750 at `device_memory_fraction = 0.9`.** Pool = 0.9 x
`get_total_memory()`. At a nominal 2 GiB total that is 1,932,735,283 bytes.
- headers only: /1,052,672 = **1836**
- plain SNode populated: /5,246,976 = **368**
- pointer SNode headers only: /4,214,784 = **458**

Adversary 03-1 gives ~458 for the pointer row and I reproduce it. Adversary
03-2 gives 351 plain and 211 pointer; **neither reproduces for me**, and the
two are not consistent with each other: 351 implies a pool near 1.84e9 while
211 implies one near 0.89e9. I am recording my own arithmetic and flagging
theirs as unreproduced rather than adopting either.

## N30 — `NodeManager` chunk counts: I omitted the default and the override

`runtime.cpp:643-664`. The ctor's default is `128 * 1024` at **line 649**
(comment 647, `if` 648, assignment 649), and the 128 MB halving loop is
**652-655**. Adversary 03-1 cites 647-650, adversary 03-2 cites 646-649; the
value assignment is on 649 and the enclosing `if` block is 648-650.

The only call site overrides it: `runtime.cpp:1030`
`runtime->create<NodeManager>(runtime, node_size, 1024 * 16)`, so
`chunk_num_elements` is **16384**, and the halving loop engages only when
`node_size > 8192` (since 16384 x 8192 = 128 MiB). My report listed the ctor
parameters and never stated either number. Both adversaries are right that this
is a gap; adversary 03-1 is right that it is a gap rather than an error, since
I never quoted the 131072 figure for it.

Chunk sizes that follow, from `runtime.cpp:658-663`:
- `free_list`, `recycled_list`: `sizeof(i32)` x 16384 = 65,536 each
- `data_list`: `node_size` x 16384. `node_size` is `cell_size_bytes` for
  pointer, or `sizeof(void*) + cell_size * chunk_size` for dynamic
  (`llvm_runtime_executor.cpp:448-457`). A 1 KiB cell gives a single 16 MiB
  allocation; a 4 KiB cell gives 64 MiB.

## N31 — ADJUDICATED: the abort site

`runtime.cpp`, offset-printed: `#if ARCH_cuda` 860, comment 861-863,
`__assertfail(` **864** through `869`, `#endif` 870, closing `}` 871,
`taichi_assert_runtime(this, success, ...)` **872**.

Adversary 03-1's 864-869 and 872 is correct. Adversary 03-2's 860-868 and 871
is wrong on both. Adversary 03-1 already recorded 03-2's error and its own
earlier 864-871 slip; I confirm 864-869 / 872.

## N32 — ADJUDICATED: `ptr2index` is a full scan on CPU. Both adversaries are partly wrong.

Adversary 03-2 calls `ptr2index` (`runtime.cpp:502-512`) an O(131072) scan on
the hot path. Adversary 03-1 rebuts: the `taichi_assert_runtime` on every
iteration "terminates at the first untouched chunk".

I read `taichi_assert_format`, `runtime.cpp:766-815`:

```
780:  if (!enable_assert || test != 0)
781:    return;
...
798: #if ARCH_cuda
800:   asm("exit;");
801: #elif ARCH_amdgpu
802:   asm("S_ENDPGM");
814: #endif
815: }
```

The early return on 780-781 fires when the test **passes**. When it fails, the
error is recorded and then the thread is killed **only under `ARCH_cuda` or
`ARCH_amdgpu`**. On the CPU build there is no `exit` — the function returns
normally and the caller's loop continues.

So:
- On CUDA/AMDGPU, adversary 03-1 is right: the thread dies at the first null
  chunk.
- On CPU, adversary 03-2 is closer: `ptr2index` sets the error flag once
  (guarded by `if (!runtime->error_code)` at `:782`) and then runs the full
  131072 iterations, because `chunks[i] == nullptr` satisfies
  `chunks[i] <= ptr` and fails `ptr < chunks[i] + chunk_size`, so no iteration
  matches and none breaks.
- In the **success** case, on every arch, the loop exits at the matching chunk,
  so it is O(index of the chunk holding `ptr`), bounded by the touched set, not
  by 131072.

Brief section 2 makes CPU-only a first-class target, so the CPU behaviour is
not a corner. Reached from `Pointer_deactivate` (`node_pointer.h:67-82`) and
`Dynamic_deactivate` (`node_dynamic.h:43-59`) via `NodeManager::recycle`
(`runtime.cpp:683-686`) and `locate` (`:679-681`).

`get_num_active_chunks` (`runtime.cpp:467-473`) is genuinely unconditional over
131072, and its only caller is the host debug printer
(`runtime.cpp:743-747` -> `llvm_runtime_executor.cpp:200-201`). Adversary 03-1
is right about that one.

## N33 — VERIFIED independently: the setters have zero call sites

Adversary 03-1's claim. My own grep, whole tree excluding `build/`, over
`.cpp .h .py`:

```
grep -rn "LLVMRuntime_set_element_lists\|LLVMRuntime_set_node_allocators\
\|LLVMRuntime_set_roots\|LLVMRuntime_set_root_mem_sizes\|_set_ambient_elements"
```

returns **nothing**. `STRUCT_FIELD_ARRAY` at `runtime.cpp:616-619` emits these
setters; nothing anywhere calls them. So my section 3.2 statement that
`destroy_snode_tree` "does not null" the slots understates it: there is no
mechanism in the codebase by which a slot could be nulled.

Note `ambient_elements` does not even get a `STRUCT_FIELD_ARRAY` — the four at
`runtime.cpp:616-619` are `element_lists`, `node_allocators`, `roots`,
`root_mem_sizes`. `ambient_elements` has no accessor at all.

## N34 — VERIFIED independently: reachability of the leak

`grep -rn "destroy_snode_tree"` over the tree, build excluded:

- `taichi/program/program.cpp:214` the definition
- `taichi/program/program.cpp:234` calls `program_impl_->destroy_snode_tree`, a
  different function (`program_impl.h:55`)
- `taichi/python/export_lang.cpp:569-570` the pybind lambda — **the only caller
  of `Program::destroy_snode_tree` in the repository**
- `python/taichi/_snode/snode_tree.py:21` drives it from Python
- the rest are the impl-side overrides and their declarations

So today the path is reachable only through the Python front end, which brief
1.2 puts out of scope. It is latent from the C++ core, not live. Brief 4.6
makes it likely this fork adds a core-owned recompose path, at which point it
becomes live. Both adversaries reached this independently and I confirm it.

## N35 — VERIFIED: three items I missed that are in my territory

1. **`taichi_listgen_max_element_size = 1024`** (`taichi/inc/constants.h:28`),
   used at `runtime.cpp:1316` and `:1369` (both in listgen, capping the
   per-element loop-bound split) and mirrored at
   `codegen/llvm/codegen_llvm.cpp:2291`. My section 4.10 "exhaustive" inventory
   grepped for array **declarations** sized by a constant, so it could not have
   caught a constant used as a `std::min` bound. The claim of exhaustiveness
   was true for what it said and misleading for what a reader would take it to
   mean. Adversary 03-1 found this; adversary 03-2 did not.
2. **`taichi_max_num_mem_requests = 1024 * 64`** (`constants.h:16`) has zero
   uses anywhere in the tree. Confirmed by my own grep.
3. **`runtime_module/CMakeLists.txt:13`** installs `runtime_${arch}.bc` while
   line 8 builds `runtime_${rtm_arch}.bc`. `rtm_arch` is the function
   parameter (line 3); `arch` is the caller's `foreach` variable (line 29)
   visible inside the function by CMake's parent-scope read. It works today
   because they hold the same value at call time. I named this file as the
   parameterisation seam and did not read line 13 against line 8.

## N36 — VERIFIED: my sm_60 flag cannot fire, downgrading it

I flagged `cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` as Pascal-pinned
against a Maxwell baseline. It is double-gated:

- `llvm_context.cpp:504-506`: `int cap = CUDAContext::get_instance()
  .get_compute_capability(); if (cap >= 60) link_module_with_custom_cuda_library(module);`
  A GTX 750 is sm_50, so the call is not made at all.
- `llvm_context.cpp:574-577`: the function body is inside
  `if (!cuda_library_path.empty())`, and
  `get_custom_cuda_library_path` (`taichi/util/lang_util.cpp:18-28`) returns
  `""` unless the file is present in `runtime_lib_dir()`. Its only `install()`
  rule is `runtime_module/CMakeLists.txt:24`, inside
  `COMPILE_CUSTOM_CUDA_LIBRARY`, whose only call site is commented out at
  `:36-42`.

Adversary 03-1 is right. Downgrading to a note.

## N37 — The `all_dense` conjunction I half-stated

`llvm_runtime_executor.cpp:402-410`:

```
402:  bool all_dense = config_.demote_dense_struct_fors;
403:  for (size_t i = 0; i < snode_metas.size(); i++) {
404:    if (snode_metas[i].type != SNodeType::dense &&
405:        snode_metas[i].type != SNodeType::place &&
406:        snode_metas[i].type != SNodeType::root) {
407:      all_dense = false;
408:      break;
409:    }
410:  }
```

The config flag is the seed and the loop can only clear it. So
`all_dense == demote_dense_struct_fors AND (every SNode is dense/place/root)`.
My report gave only the second conjunct. The consequence I missed: with
`demote_dense_struct_fors = false`, `all_dense` is false unconditionally and
**every** tree pays a `ListManager` per SNode, dense trees included. Default is
true (`compile_config.cpp:18`), forced true for SPIR-V archs
(`compile_config.cpp:72-73`), and settable from Python
(`export_lang.cpp:201-202`, out of scope per brief 1.2, so hard-coded true at
C++ level today).

## N38 — CLOSING the gfx divergence question I escalated as E7

Adversary 03-2 constructs the case and adversary 03-1 checks it. I re-derived
it from the four functions:

1. `gfx::SNodeTreeManager::materialize_snode_tree`
   (`gfx/snode_tree_manager.cpp:11-16`) only `push_back`s onto
   `compiled_snode_structs_` (`:15`) and calls `add_root_buffer`
   (`:14`), which `push_back`s onto `root_buffers_` (`gfx/runtime.cpp:747`).
2. `gfx::SNodeTreeManager::destroy_snode_tree` (`:18-29`) finds `root_id` by
   linear scan on the root pointer and resets `root_buffers_[root_id]` (`:28`).
   Neither vector shrinks and `compiled_snode_structs_` is untouched.
3. `Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
   (`program/program.cpp:235`); `allocate_snode_tree_id` (`:559-567`) pops it.
4. `get_snode_tree_device_ptr(int tree_id)` (`snode_tree_manager.cpp:49-51`)
   returns `root_buffers_[tree_id]->get_ptr()`.

Create trees 0 and 1; destroy 1; create a third. The third gets `tree_id == 1`
from the free stack, its buffer lands at `root_buffers_[2]`, and
`root_buffers_[1]` is the `unique_ptr` reset at step 2. `->get_ptr()` on it
dereferences null. `get_field_in_tree_offset(1, ...)`
(`snode_tree_manager.cpp:31-47`) reads the destroyed tree's descriptors and
trips its own `TI_ASSERT_INFO` at `:34-38`.

It diverges after one destroy-then-add cycle. It is determinable, so it stops
being an escalation and becomes a finding. The remedy remains a design decision
and stays escalated.

The LLVM path does not have the analogue: `snode_tree_allocs_` is an
`unordered_map` keyed by tree id (`llvm_runtime_executor.h:152`), so a recycled
id overwrites rather than shifts. The LLVM defect is the different one in N13.

## N39 — Citation drift corrections for this pass, verified by offset print

- `llvm_runtime_executor.cpp:637` -> **639** (`int starting_rand_state = ...`)
- `llvm_runtime_executor.cpp:257-269` -> **257-270**
- `llvm_program.h:190` (enclosing `get_field_in_tree_offset`) -> **188**;
  the `FIXME` `:191` -> **189**. My `llvm_program.h:199` for the
  `offset +=` line was already correct.
- `llvm_program.cpp:58-64` -> **58-65**
- `llvm_program.cpp:66-75` -> **67-76**
- `llvm_program.cpp:93-119` -> **97-122**
- `llvm_program.cpp:110` (`snode_cache_data.id = snodes[i]->id;`) -> **113**
- `llvm_program.cpp:53` (`++num_snode_trees_processed_`) -> **54**
- `llvm_program.cpp:60` cited for `FieldCacheData::root_id` -> **61**
  (`int root_id = tree->root()->id;`; 60 is `int snode_tree_id = tree->id();`)
- `llvm_program.cpp:130-148` -> **130-149**
- `llvm_runtime_executor.cpp:642-653` verified **correct as written**
  (declaration on 642, the two assignments on 648 and 653); adversary 03-2's
  641 includes the preceding comment, which is a matter of taste, not an error.

## N40 — What I am NOT changing, and why

- The `sizeof` table, the closed form and the scaling table in section 2 stand
  unaltered. Both adversaries reproduced every figure.
- Section 3.1 (per-tree assertion versus global array index) stands. Both
  adversaries verified it independently and both call it the sharpest finding
  in the territory.
- Section 4 stands except for the additions in N35 and the drift in N39.
- I am not proposing values for `taichi_max_num_snodes`, `device_memory_GB`,
  `ListManager::max_num_chunks` or the `1024 * 64` element-list chunk count.
  Per standing instruction 3 I am not calling any of them surplus either.

## N41 — Revised report checked, all revised arithmetic recomputed

I recomputed every figure in the revised section 2 rather than transcribing my
hand working. Charges, capacities and slack all reproduce:

```
roundup(1048616, 4096) = 1052672          ListManager charge
roundup(56, 4096)      = 4096             NodeManager charge
65536 * 64             = 4194304          element-list chunk (page-exact)
16384 * 4              = 65536            free/recycled chunk (page-exact)

cum(1020) = 1048616 + 1019*1052672 = 1,073,721,384   fits, slack 20,440
cum(1021) = 1048616 + 1020*1052672 = 1,074,774,056   over by 1,032,232
cum(1024) = 1048616 + 1023*1052672 = 1,077,932,072   over by 4,190,248

plain populated  = 1052672 + 4194304          = 5,246,976  -> 204 fit
   204 x = 1,070,383,104   205 x = 1,075,630,080
pointer headers  = 1052672 + 4096 + 3*1052672 = 4,214,784  -> 254 fit
   254 x = 1,070,555,136   255 x = 1,074,769,920
pointer + chunks = 4214784 + 4194304 + 2*65536 = 8,540,160 -> 125 fit
   125 x = 1,067,520,000   126 x = 1,076,060,160

pool at 0.9 x 2 GiB = 1,932,735,283 -> 1836 / 368 / 458
```

Citation audit of the revised report, by extracting every `file:line` and
printing the source line. New citations verified: `compile_config.cpp:18,63,72`,
`export_lang.cpp:201,208,569`, `cuda_context.cpp:88`, `lang_util.cpp:18`,
`llvm_context.cpp:504-506,574-577`, `llvm_runtime_executor.cpp:412,413,448,587,
620,629,721`, `runtime.cpp:649,683,743,766,848,859,864,962,1016,1030,1316`,
`runtime_module/CMakeLists.txt:3,8,13,24,29`,
`python/taichi/_snode/snode_tree.py:21`. All land.

One further drift caught by the audit and fixed:
`llvm_runtime_executor.cpp:385-388` for `get_snode_tree_device_ptr` should be
**386-389**. (`:387` for the `snode_tree_allocs_` read was already right.)

Also added while checking: `runtime_allocate_ambient` requests alignment
**128**, not 4096 (`runtime.cpp:1038-1039`), so the ambient element is the one
runtime-memory allocation that is not charged a page. Its span is 1033-1040,
not 1033-1038.

## N42 — Revision pass closed

The report is revision 2. Section 6 of it carries the withdrawal, the
corrections, the closures and the adjudication table. I did not touch any
source file, and I did not open report-03 or notes-03.

---

# Revision 3 — amendment pass, round-two adversarial review

Four claims handed to me for adjudication. I read `adversary2-03-1.md` and
`adversary2-03-2.md` and verified every claim against source before touching
the report. I read no other agent's report and no source file was modified.
Where the two adversaries split on severity I took the harsher list, which is
`adversary2-03-2.md`'s: 03-1 says two sentences remain, 03-2 finds two further
errors 03-1 lacks. Both of 03-2's extra findings survive verification, so the
harsher verdict is the correct one.

## N43 — CLAIM 1 UPHELD: E5 outruns its evidence on CPU

Claim: E5's "raising either alone is inert" must be a CUDA and AMDGPU
statement, not a universal one. Both adversaries raise it; 03-1 localises it
precisely to that one sentence and notes my body text was already scoped
("on every CUDA card", "every tier", and every tier in brief 5.2 is a GPU).
That localisation is right and I adopt it.

Verified by grep and offset print:

```
$ grep -rn "preallocate_runtime_memory" --include=*.cpp --include=*.h taichi/
llvm_runtime_executor.cpp:413    call site 1
llvm_runtime_executor.cpp:607    definition
llvm_runtime_executor.cpp:721    call site 2
llvm_runtime_executor.h:105      declaration
```

- `:412` guards `:413` on `config_.arch == Arch::cuda && use_device_memory_pool()
  && !all_dense`.
- `:719` guards `:721` on `config_.arch == Arch::cuda || config_.arch ==
  Arch::amdgpu`, with `:720` adding `!use_device_memory_pool()`.
- `use_device_memory_pool_` is assigned at `:49`, inside the
  `if (config.arch == Arch::cuda)` branch of the constructor at `:39`, from
  `CUDAContext::get_instance().supports_mem_pool()`. Nothing else assigns it,
  so it is false on every non-CUDA arch.
- `runtime.cpp:830-834`: `allocate_aligned` uses the reserved chunk only when
  `memory_chunk.preallocated_size > 0`, otherwise
  `return (Ptr)host_allocator(memory_pool, size, alignment);` at `:834`.
- `host_allocator` is `host_allocate_aligned` (`llvm_runtime_executor.cpp:28-32`,
  bound at `:710`), which is `HostMemoryPool::allocate`, reaching
  `UnifiedAllocator::allocate`. `unified_allocator.cpp:64-69` sizes a fresh
  chunk `std::max(size, default_allocator_size)` when the current chunk cannot
  serve. No ceiling.

So on CPU there is no pool and the constant binds alone. Report amended at 2.6
(scope plus the positive CPU consequence) and at E5 (the qualifier, and the
statement that item 8.1.2 does not have one answer across the target set).
I am not proposing a value or a per-arch rule; that is E5's escalation and it
stays the planner's.

## N44 — CLAIM 2 UPHELD, with one qualifier of my own

Claim: the root buffers do not sit outside the 1 GiB pool when
`use_device_memory_pool()` is false, because the chain runs tree buffer manager
-> RHI caching allocator -> the same bump allocator; ndarrays and argpacks take
the same route. Only `adversary2-03-2.md` has this. 03-1's section 7 says it
looked for "a third pool … and found none", which is true and is a different
question. I read every hop rather than accepting the chain:

1. `llvm_runtime_executor.cpp:419-420` — `snode_tree_buffer_manager_->allocate`.
2. `snode_tree_buffer_manager.cpp:12-18` — `:15` calls
   `runtime_exec_->allocate_memory_on_device`.
3. `llvm_runtime_executor.cpp:476-491` — `:479` calls
   `llvm_device()->allocate_memory_runtime`, and `:485` passes
   `use_device_memory_pool()` as the params' `use_memory_pool`.
4. `cuda_device.cpp:50-78` — `:56` branches. True: `malloc_async` at `:57-58`.
   False: `DeviceMemoryPool::get_instance().allocate_with_cache` at `:60-61`.
5. `device_memory_pool.cpp:27-33` -> `allocator.cpp:33-58`; on a cache miss
   `:54-55` calls `device->allocate_llvm_runtime_memory_jit`.
6. `cuda_device.cpp:80-90` — `:82-84` JIT-calls
   `"runtime_memory_allocate_aligned"`.
7. `runtime.cpp:879-886` — `:883-885` is
   `runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)`.

The chain holds exactly. `amdgpu_device.cpp:55-78` has no `use_memory_pool`
branch at all and always takes `allocate_with_cache` at `:63`, with the same
JIT hop at `:80-90`, so on AMDGPU the root buffers are always inside the pool.
`ndarray.cpp:61` and `argpack.cpp:17-18` both route through
`Program::allocate_memory_on_device` (`program.h:245-248`), the same function
as hop 3, so the claim about ndarrays and argpacks holds too.

The runtime-objects buffer is genuinely outside: its own `preallocate_memory`
at `llvm_runtime_executor.cpp:687-690`, reaching `llvm_device()->allocate_memory`
at `:594-596`. So my revision-2 sentence was right about one of the three things
it named and wrong about the other.

**My qualifier.** The adversary goes on to say this makes every capacity row an
upper bound "on exactly the baseline-tier hardware". That does not follow from
this tree. `cuda_context.cpp:35-53` shows the branch is decided at runtime by a
driver-version test (11.2 or newer) and the device attribute
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`. Nothing in the repository records
what the GTX 750, the GTX 1070 or the RTX 3060 report, and I have not run
against any of them. The mechanism is verified; the mapping onto brief 5.2's
tiers is not. I recorded the mechanism in 2.7(c) and escalated the mapping as
E15 rather than asserting it. This is the one place I did not take the harsher
list at face value, and the reason is standing instruction 2, not disagreement
about the code.

Also checked and recorded: the CPU path is not on this chain either.
`cpu_device.cpp:44-51` calls `allocate_memory` directly and never reaches
`allocate_with_cache`, so the CPU root buffer is an ordinary host allocation.

## N45 — CLAIM 3 UPHELD: my [V] was wrong, and here is how

Claim: `device_memory_GB` is a public field of a DLL-exported global that
`Program` copies at construction, so it is reachable from C++, and my report
marked the opposite VERIFIED.

Verified, every line printed:

- `taichi/program/compile_config.h:8` — `struct CompileConfig {`, a plain
  struct, all fields public.
- `:71-72` — `float64 device_memory_GB;` and `float64 device_memory_fraction;`.
- `:28` — `bool demote_dense_struct_fors;`, the same fault.
- `:110` — `extern TI_DLL_EXPORT CompileConfig default_compile_config;`
- `taichi/util/lang_util.cpp:14` — `CompileConfig default_compile_config;`
- `taichi/program/program.cpp:74-77` — `auto &config = compile_config_;`,
  `config = default_compile_config;` at `:75`, `config.arch = desired_arch;` at
  `:76`, `config.fit();` at `:77`.
- `compile_config.cpp:67-76` — `fit()` touches `check_out_of_bound`,
  `demote_dense_struct_fors` for SPIR-V archs, and calls
  `disable_offline_cache_if_needed`. It does not touch `device_memory_GB` or
  `device_memory_fraction`, so a value written to the global survives into the
  `Program`'s config.
- `export_lang.cpp:264-267` — the binding hands Python a **reference** to that
  same global. Python is a client of it, not its owner.

The claim holds in full.

**Consequence, established rather than assumed.** The two halves of the pair in
plan 8.1 item 2 are not equally expensive to move.
`taichi_max_num_snodes` is `constexpr int` at `taichi/inc/constants.h:12`, and
the `.bc` that consumes it is produced by the standalone clang command at
`runtime_module/CMakeLists.txt:8`, which receives only `-D ARCH_<arch>` and
`-I ${PROJECT_SOURCE_DIR}` and inherits no project compile definitions; the
arch loop is at `:29-31`. Moving it needs a build-time mechanism that does not
exist today, which is my own escalation E9. `device_memory_GB` needs no
mechanism: one assignment to `default_compile_config` before `Program`
construction. My revision-2 text told the planner the reverse, in E5, which is
the escalation the planner reads as the answer to 8.1.2. Corrected in 2.6 and
E5. `demote_dense_struct_fors` carried the identical fault in 2.4 and E5c and
is corrected in both places, since it is the same false sentence about the same
global.

**How a [V] marking came to be wrong.** N41's citation audit is the record. The
verification I actually ran was an offset print: I resolved
`export_lang.cpp:208` and `:201` and confirmed the lines say what I quoted them
as saying, and the audit entry reads "All land." That certifies the citation.
The sentence I attached the marking to was not the citation but a quantifier
over the whole tree — "reachable **only** from Python" — and a printed line
cannot establish a negative existential. Worse, the search that would have
tested it is not a search for the field name at all: reachability runs through
`default_compile_config`, an ordinary exported object that any C++ caller
touches by struct assignment, so no grep for `device_memory_GB` external
setters could have found it. The failure is a category slip: **[V]** earned by
a citation was allowed to cover an inference the citation did not support, and
the word "only" is exactly the shape of claim that needs the opposite of a
citation check. For the rest of this file, a **[V]** on any sentence containing
"only", "never", "no other" or "nowhere" means I ran the exhaustive search and
can name it; otherwise the sentence is **[I]**.

## N46 — CLAIM 4 UPHELD: both listgen spans end early

Claim from `adversary2-03-1.md` §7.3, seconded by `adversary2-03-2.md` §8.3.
Printed with line numbers:

```
1282: void element_listgen_root(LLVMRuntime *runtime,
...
1328:   }
1329: }
1331: void element_listgen_nonroot(LLVMRuntime *runtime,
...
1382:   }
1383: }
```

`element_listgen_root` is 1282-1329; I wrote 1282-1328. `element_listgen_nonroot`
is 1331-1383; I wrote 1331-1380, three short. Both appear twice in the report,
in 2.5 and in 4.6, and all four instances are corrected. No downstream figure
depends on either span.

## N47 — Amendment pass closed

The report is revision 3. Section 6.1 carries the adjudication table, the two
withdrawals of **[V]**-marked statements, the qualifier I placed on claim 2,
and the corrections. Not disturbed, per both round-two adversaries: `ptr2index`,
the capacity of 1020 with the 4,190,248-byte overrun, and the 2 GB card figures
1836 / 368 / 458. New escalation: E15 only. I opened no source file for editing,
touched no other agent's file, decided nothing to be unnecessary, added no
abstraction, and opened no line of investigation beyond the four claims.
