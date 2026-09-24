# Report 03 — Runtime and struct layer

Explore agent 03. Territory: `taichi/runtime/`, `taichi/struct/`,
`taichi/program/`. All paths relative to `/opt/project/taichi`.

Working notes: `modernization/investigation/notes-03-runtime-struct.md`.
Entries 1-19 are the first pass. Entries 20-28 are the revision-2 pass. Entries
29-36 are this amendment pass and carry the adjudication of the round-two
adversaries.

**Plan version worked against: `PROJECT-PLAN.md` last updated 2026-09-09**,
re-read in full before this revision was written. Section 10 was renumbered
between revision 3 and revision 4; grading is now item 8 and the citation rule
is item 7. Revision 3 was written against the 2026-09-08 version. Nothing in
this report contradicts the 2026-09-09 plan, and where the plan now records a
finding of mine as settled I have not re-argued it.

**Revision 4**, after `adversary3-03-1.md` and `adversary3-03-2.md`. Both
round-three adversaries found this report correct and incomplete. Three gaps,
all folded, all re-verified against source first:

- Section 3.4 and E15. **The pool's consumer list was short by a class.**
  Per-launch staging buffers for external array arguments enter the same bump
  allocator by the same hops. Both files are inside this territory. Notes 38.
- Sections 1, 3.2 and E1. **The CPU consequence is now stated positively**,
  not only as a scope limit on the GPU claim. Notes 39.
- Section 5.5 and 7.2. **The dangling `snode_tree_allocs_` entry reconciled.**
  This report said it is handed out on a recycled tree id while 7.2 said a
  recycled id overwrites. Both are true, on different branches of
  `Program::add_snode_tree`. Notes 40.

Two further items, on the adversaries' remaining lists but not in the brief I
was given, verified and folded, and flagged as such:

- Section 3.2 and E16. **DX12 is a fifth LLVM arch and takes neither pool
  guard.** The positive statements here were exact; the enumeration of the
  exception said "CPU" and DX12 is not CPU. Notes 41.
- Section 5.2, one citation; section 3.4, one loose word; section 3.2, one
  stale credit. Notes 43.

**Measured, not inferred, and now recorded as such.** Both round-three
adversaries queried the CUDA driver on the development box independently and
agree. Revision 3 declined to map the memory-pool branch onto the tiers in plan
section 5.2, on the ground that the gate is a device attribute this tree cannot
read. That restraint was right about the tree and the measurement settles the
question from outside it. Folded into 3.4. Notes 42.

**Revision 3**, after `adversary2-03-1.md` and `adversary2-03-2.md`. This is an
amendment, not a re-investigation. Every claim in it was re-verified against
source before it was written in; nothing was adopted on an adversary's word.
What changed:

- Section 8.3 and escalation E14. The `ptr2index` termination claim was wrong.
  `taichi_assert_format` emits its thread kill only under `ARCH_cuda` and
  `ARCH_amdgpu`, so the CPU build runs all 131072 iterations on the not-found
  path. Both round-two adversaries carried this and both are right. Notes 29.
- Section 3.4. The claim that the SNode tree root buffers sit outside the 1 GiB
  pool was wrong on the path where `use_device_memory_pool()` is false. The full
  allocation chain is now recorded. Ndarrays and argpacks take the same route.
  Notes 30.
- Section 3.2. "On CUDA the pool is created either way" was self-contradicted by
  its own next sentence. Corrected, with the AMDGPU case now stated from
  `AmdgpuDevice::allocate_memory_runtime` rather than inferred.
- Section 3.3. The page-alignment assumption is closed rather than carried.
  Nothing in the tree aligns the pool base, and the answer is 1020 regardless.
  Notes 34.
- Escalation E1. `device_memory_GB` is **not** hard-coded at C++ level. It is a
  public field of a `TI_DLL_EXPORT` global that `Program` copies at
  construction. This inverts the relative cost of the two halves of the pair in
  plan section 8.1 item 2. Notes 31.
- Section 4.5, one off-by-one citation. Section 9, two missing array entries.
  Section 5.5, one path-conditional word. Notes 32, 33, 35.

**Revision 2**, after `adversary-03-1.md` and `adversary-03-2.md`. What changed:

- Every citation into `taichi/struct/snode_tree.cpp` and
  `taichi/runtime/gfx/snode_tree_manager.cpp` was wrong by a constant offset.
  All regenerated. Cause and diagnosis in notes entry 22.
- The preallocation pool is now section 3. It is the thing that binds, and the
  first pass did not have it.
- The footprint figures in section 2 are now simulated against the real bump
  allocator rather than hand-derived, and are between 4x and 5x the first
  pass's.
- The `all_dense` condition was inverted in the first pass. Corrected in 2.6.
- The gfx numbering question is closed rather than escalated (section 7.2).
- Reachability of the lifecycle findings is now established (section 6.4).

Everything below is marked **VERIFIED** (read in the source, or measured with
the compiler, or simulated from the source) or **INFERRED** (a conclusion drawn
from verified facts).

---

## 1. Summary

1. `taichi_max_num_snodes = 1024` costs **24576 bytes** inside `LLVMRuntime`,
   69.7% of that struct. Real, and negligible.
2. The cost that matters is per SNode, not per slot. A sparse SNode whose
   element list has been touched consumes **5,246,976 bytes**. A pointer SNode
   with its allocator and no chunks touched consumes **4,214,784 bytes**.
3. Those bytes come from a **fixed 1 GiB preallocated pool**, not from free
   device memory. `device_memory_GB` defaults to 1 at
   `taichi/program/compile_config.cpp:63`. It is a public field of an exported
   global, settable from C++ without any new mechanism (E1). Where
   `use_device_memory_pool()` is false, root buffers, ndarrays and argpacks come
   out of the same pool (3.4).
4. That pool holds **1020 element-list headers**. The constant is 1024. The two
   are already matched to within 0.4%, so **raising `taichi_max_num_snodes`
   alone is inert on CUDA and AMDGPU.** Open question 8.1.2 is a pair of
   numbers, not one.
5. Exhaustion is a hard grid abort, not degradation.
6. The 1024 bound is checked **per tree** but the arrays it sizes are indexed
   by a **program-global, never-recycled** SNode id.
7. A second ceiling, `kMaxNumSnodeTreesLlvm = 512`, sizes two more arrays in
   the same struct and is bounds-checked nowhere.

---

## 2. Footprint

### 2.1 Structure sizes — VERIFIED by compiler

I reproduced `LLVMRuntime`, `ListManager` and `NodeManager` field-for-field
from `taichi/runtime/llvm/runtime_module/runtime.cpp` into a throwaway
translation unit including the real `taichi/inc/constants.h`, and read the
sizes out of `g++ -fsyntax-only` via an incomplete-template diagnostic. Nothing
was written to disk. Both adversaries reproduced every figure independently,
one of them by compiling the real `runtime.cpp`.

| Type | `sizeof` (bytes) |
|---|---|
| `LLVMRuntime` | 35256 |
| `ListManager` | 1048616 |
| `NodeManager` | 56 |
| `Element` | 64 |
| `PhysicalCoordinates` | 48 |
| `RandState` | 20 |

Offsets: `roots` 88, `element_lists` 8296, `node_allocators` 16488,
`ambient_elements` 24680, `temporaries` 32872. The three SNode arrays occupy
bytes 8296-32871, 24576 contiguous.

### 2.2 Decomposition of `sizeof(LLVMRuntime)` — VERIFIED

```
element_lists    1024 * 8  =  8192
node_allocators  1024 * 8  =  8192
ambient_elements 1024 * 8  =  8192
                             -----
                             24576   69.71%   <- taichi_max_num_snodes

roots             512 * 8  =  4096
root_mem_sizes    512 * 8  =  4096
                             -----
                              8192   23.24%   <- kMaxNumSnodeTreesLlvm

error_message_template       2048    5.81%
error_message_arguments       256    0.73%
chunks, fn ptrs, locks,
counters, misc                184    0.52%
                             -----
                             35256  100.00%
```

With N = `taichi_max_num_snodes` and T = `kMaxNumSnodeTreesLlvm`:

```
sizeof(LLVMRuntime) = 2488 + 16*T + 24*N
```

Check at N=1024, T=512: `2488 + 8192 + 24576 = 35256`. Marginal cost is 24
bytes per SNode slot, 16 per tree slot.

| N | `sizeof` at T=512 | page-rounded |
|---|---|---|
| 1024 | 35256 | 36864 |
| 4096 | 108984 | 110592 |
| 16384 | 403896 | 405504 |
| 65536 | 1583544 | 1585152 |

**INFERRED: raising the constant is nearly free in the struct itself.** At
N=65536 the struct is 1.51 MiB. It is charged once per `Program` through
`runtime_get_memory_requirements` (`runtime.cpp:891-906`, `iroundup` of
`sizeof(LLVMRuntime)` at line 897, `taichi::iroundup` at
`taichi/math/arithmetic.h:13-17`), against a separate runtime-objects
allocation made at `taichi/runtime/llvm/llvm_runtime_executor.cpp:675-694`. On
CPU it goes through `host_allocator(memory_pool, sizeof(LLVMRuntime), 128)`
(`runtime.cpp:933`), and `UnifiedAllocator::allocate` gives an oversized
request its own chunk (`taichi/rhi/common/unified_allocator.cpp:68`), so there
is no ceiling there either.

### 2.3 The cost that is not in the struct — VERIFIED

`ListManager` (`runtime.cpp:426-513`) opens with a flat pointer table:

```
427:  static constexpr std::size_t max_num_chunks = 128 * 1024;
428:  Ptr chunks[max_num_chunks];
```

131072 pointers, 1048576 bytes, present in every instance whether or not a
chunk is touched.

One is allocated **per SNode** in a non-`all_dense` tree, `place` nodes
included, at `runtime.cpp:1003-1007`:

```
for (int i = root_id; i < root_id + num_snodes; i++) {
  // TODO: some SNodes do not actually need an element list.
  runtime->element_lists[i] =
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

`LLVMRuntime::create<T>` (`runtime.cpp:606-612`) calls
`allocate_aligned(runtime_memory_chunk, sizeof(T), 4096, request=true)`, so the
full 1048616 bytes is committed at materialisation, not on demand.
`num_snodes` is `(int)snode_metas.size()`
(`llvm_runtime_executor.cpp:443-444`), built from `struct_compiler.snodes`
with no filtering (`taichi/runtime/program_impls/llvm/llvm_program.cpp:110-119`),
which `collect_snodes` (`taichi/struct/struct.cpp:7-13`) fills by full preorder
traversal.

Every garbage-collectable SNode additionally gets a `NodeManager`
(`runtime.cpp:1026-1031`), whose constructor (`runtime.cpp:643-664`) creates
**three more** `ListManager`s at lines 658-659, 660-661 and 662-663.
`is_gc_able` is `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`),
tested at `llvm_runtime_executor.cpp:447`.

### 2.4 Chunk sizes — VERIFIED

`ListManager::touch_chunk` (`runtime.cpp:1664-1679`) allocates
`max_num_elements_per_chunk * element_size` at 4096 alignment (lines 1672-1674).

| List | elements/chunk | element size | chunk bytes |
|---|---|---|---|
| element list (`runtime.cpp:1005-1006`) | 65536 (`1024*64`) | 64 (`sizeof(Element)`) | **4,194,304** |
| `NodeManager::free_list`, `recycled_list` | 16384 | 4 (`sizeof(i32)`) | 65,536 |
| `NodeManager::data_list` | 16384 | `node_size` | 16384 × `node_size` |

The `NodeManager` constructor's documented default is 128K elements per chunk
(`runtime.cpp:647-650`), but the **only** call site overrides it with
`1024 * 16` at `runtime.cpp:1030`, so `chunk_num_elements` is 16384 and the
128 MB halving loop at `runtime.cpp:652-655` engages only when
`node_size > 8192`. `node_size` is set on the host at
`llvm_runtime_executor.cpp:448-456`: `cell_size_bytes` for pointer,
`sizeof(void*) + cell_size_bytes * chunk_size` for dynamic.

**The element list's first chunk is not deferred.**
`runtime_initialize_snodes` appends the root element at `runtime.cpp:1016`,
which runs `append` → `allocate` → `reserve_new_element` → `touch_chunk(0)`
(`runtime.cpp:1681-1689`, `451-456`). So 4 MiB is committed for the root's list
at materialisation of every sparse tree, before any kernel runs. Every other
element list takes its 4 MiB on its first listgen append
(`element_listgen_root` `runtime.cpp:1282-1329`, `element_listgen_nonroot`
`:1331-1383`).

**And it is never returned.** `clear_list` (`runtime.cpp:1270-1273`) calls
`ListManager::clear()` (`:475-477`), which only sets `num_elements = 0`. The
touched set is monotonic for the life of the process.

### 2.5 Per-SNode cost — VERIFIED by simulation

I simulated `allocate_from_reserved_memory` (`runtime.cpp:838-874`) exactly —
pad to the requested alignment, charge the pad to the caller at line 850,
advance the head by pad+size at line 853, fail when it would pass the tail at
line 851 — rather than deriving by hand.

| Configuration | Steady-state bytes per SNode |
|---|---|
| element list header only | 1,052,672 |
| element list header + its first 4 MiB chunk | **5,246,976** |
| pointer SNode, all four headers + NodeManager, no chunks | **4,214,784** |
| pointer SNode, headers + element chunk + a `data_list` chunk at `node_size`=256 | 12,603,392 |

The header-only figure is 1,052,672 rather than 1,048,616 because
`1048616 mod 4096 = 40`, so every allocation after the first is charged 4056
bytes of alignment padding. That is 257 pages exactly.

Not folded into any row because it is data-dependent: each
garbage-collectable SNode also takes an ambient element of `node_size` bytes at
128-byte alignment (`runtime.cpp:1033-1040`).

### 2.6 The whole cost is conditional on `all_dense` — VERIFIED

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

The config flag is the **seed value** and the loop can only clear it. So

```
all_dense == demote_dense_struct_fors AND (every node is dense, place or root)
```

It is a necessary precondition for skipping the element lists
(`runtime.cpp:1000-1002`), never something that forces the skip on. *The first
revision of this report stated the inverse. Both adversaries caught it.*

Two consequences:

- Default `demote_dense_struct_fors = true`
  (`taichi/program/compile_config.cpp:18`), forced true again for SPIR-V archs
  by `CompileConfig::fit` (`compile_config.cpp:72-74`). So under the default,
  the cost above is paid only by trees containing a non-dense node, which is
  exactly what brief section 4.1 requires.
- With the flag **false**, `all_dense` is false unconditionally and every tree
  pays a `ListManager` per SNode, dense trees included. That makes
  `demote_dense_struct_fors` a third dial on the footprint.

---

## 3. The preallocation pool — what actually binds

**This section did not exist in the first revision. It is the answer to open
question 8.1.2 and the first pass walked past it.**

### 3.1 The pool — VERIFIED

`ListManager`s are not cut from free device memory.
`LLVMRuntime::allocate_aligned` (`runtime.cpp:823-835`) dispatches to
`allocate_from_reserved_memory` whenever
`memory_chunk.preallocated_size > 0` (line 830). That chunk is set by
`runtime_initialize_memory` (`runtime.cpp:962-971`), called from
`LlvmRuntimeExecutor::preallocate_runtime_memory`
(`llvm_runtime_executor.cpp:607-632`):

```
613:  if (config_.device_memory_fraction == 0) {
614:    TI_ASSERT(config_.device_memory_GB > 0);
615:    total_prealloc_size = std::size_t(config_.device_memory_GB * (1UL << 30));
616:  } else {
617:    total_prealloc_size =
618:        std::size_t(config_.device_memory_fraction * total_mem);
619:  }
620:  TI_ASSERT(total_prealloc_size <= total_mem);
```

and the defaults, `taichi/program/compile_config.cpp:63-64`:

```
63:  device_memory_GB = 1;  // by default, preallocate 1 GB GPU memory
64:  device_memory_fraction = 0.0;
```

**So the default pool is 1 GiB on every CUDA card, from a GTX 750 to a 24 GB
Ampere part.** It does not scale with the hardware.

### 3.2 Which path sets it up — VERIFIED

`preallocate_runtime_memory` has exactly two call sites:

- `llvm_runtime_executor.cpp:719-723` — in `materialize_runtime`, for CUDA or
  AMDGPU, only `if (!use_device_memory_pool())`.
- `llvm_runtime_executor.cpp:412-414` — in `initialize_llvm_runtime_snodes`,
  `if (config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense)`.

`use_device_memory_pool_` is initialised `false`
(`llvm_runtime_executor.h:162`) and set at `llvm_runtime_executor.cpp:49` from
`CUDAContext::get_instance().supports_mem_pool()`, which reads
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` and is gated on driver >= 11.2
(`taichi/rhi/cuda/cuda_context.cpp:35-53`).

**VERIFIED, corrected in revision 3.** An earlier wording here said "on CUDA the
pool is created either way; only *when* differs." That is false and its own next
sentence contradicted it. Both round-two adversaries caught it. The three cases,
read off the two guards:

| `arch` | `use_device_memory_pool()` | `all_dense` | Pool created |
|---|---|---|---|
| cuda | false | either | yes, at `:720-721` |
| cuda | true | false | yes, at `:412-413` |
| cuda | true | true | **no. Neither guard passes.** |
| amdgpu | always false | either | yes, at `:720-721` |

`use_device_memory_pool_` is written at exactly one place,
`llvm_runtime_executor.cpp:49`, inside the `config.arch == Arch::cuda` branch at
`:39`, and is otherwise the `false` initialiser at `llvm_runtime_executor.h:162`.
So on AMDGPU `use_device_memory_pool()` is unconditionally false and the first
call site always fires. On the CUDA mem-pool branch, `all_dense` gates whether
`runtime_memory_chunk` exists at all, not merely how much it holds.

`preallocate_runtime_memory` is idempotent: `llvm_runtime_executor.cpp:608-609`
returns early if `preallocated_runtime_memory_allocs_` is already set, so the
two call sites cannot build two pools. Recorded so the table above is not
misread.

**Scope limit.** Neither call site fires on any arch but CUDA and AMDGPU,
`preallocated_size` stays 0, and `allocate_aligned` falls through to
`host_allocator` (`runtime.cpp:830-834`). **The 1 GiB ceiling is a CUDA and
AMDGPU fact, not a universal one.** Plan section 2 makes CPU-only a first-class
target, so this must not be read as a global limit.

**Which archs the exception covers, enumerated rather than named — REVISION 4.**
Revisions 2 and 3 wrote this as "the CPU path". That is short. `arch_uses_llvm`
(`taichi/rhi/arch.cpp:54-57`) admits **five** archs — `x64`, `arm64`, `cuda`,
`dx12`, `amdgpu` — and the two pool guards admit two of them.

| Arch | On the LLVM spine | Takes a pool guard |
|---|---|---|
| `x64` | yes | no |
| `arm64` | yes | no |
| `cuda` | yes | yes |
| `amdgpu` | yes | yes |
| `dx12` | yes | **no** |

**DX12 is not a CPU arch.** `arch_is_cpu` (`taichi/rhi/arch.cpp:42-48`) admits
only `x64`, `arm64` and `js`. `LlvmRuntimeExecutor`'s constructor nevertheless
hands DX12 a `cpu::CpuDevice` at `llvm_runtime_executor.cpp:158-164`, under a
`FIXME: add dx12 device`, and `DX12_ARCH` sits in the bitcode arch loop at
`runtime_module/CMakeLists.txt:29`, so a `runtime_dx12.bc` is built with its own
baked copy of the constant. Found by `adversary3-03-1.md`; the five-arch list,
the CPU-arch list and the device assignment were each read here before folding.
Escalated as E16.

**THE POSITIVE CONSEQUENCE, stated rather than left to inference — REVISION 4.**
Earlier revisions gave only the negative scoping and never joined it to the
facts already in section 2.2. Both round-three adversaries asked for the
positive form, and plan section 6.1 already records it. Stating it here:

Where no pool exists — `x64`, `arm64` and `dx12` — `allocate_aligned` takes the
`host_allocator` branch at `runtime.cpp:834`, which reaches
`UnifiedAllocator::allocate` (`taichi/rhi/common/unified_allocator.cpp:64-85`).
That gives an oversized request its own fresh chunk at `:68` and has no ceiling.
Nothing then bounds the `ListManager`s except host RAM and the array itself:
`element_lists` is 1024 entries (`runtime.cpp:567`) and the write at
`runtime.cpp:1005` is unchecked. **So on those archs `taichi_max_num_snodes` is
the binding limit and raising it alone is effective**, at the roughly 5.2 MiB
per populated sparse SNode computed in 2.5. That is the opposite of the CUDA and
AMDGPU conclusion, and it is the answer for the target plan section 2 puts
first.

### 3.3 Capacity — VERIFIED by simulation

Against the default 1 GiB pool, with the allocator simulated exactly:

| Configuration | Fits in 1 GiB |
|---|---|
| element list headers only | **1020** |
| element list header + its first 4 MiB chunk | **204** |
| pointer SNode, headers only | **254** |
| pointer SNode, headers + element chunk + a `data_list` chunk at `node_size`=256 | **85** |

**These rows assume the whole pool is available to `ListManager`s. Where
`use_device_memory_pool()` is false they are upper bounds, because the root
buffers, ndarrays and argpacks share the same pool. See 3.4.**

Closed form for the header-only case:

```
head(k) = 1048616 + (k-1) * 1052672
head(1020) = 1,073,721,384  <=  2^30 = 1,073,741,824   fits
head(1021) = 1,074,774,056  >   2^30                    does not
head(1024) = 1,077,932,072  -> overruns the pool by 4,190,248 bytes
```

**INFERRED, and this is the finding:** the existing constant of 1024 and the
existing default pool of 1 GiB are matched to within 0.4%. The pool runs out
four element lists before the constant does. **Raising
`taichi_max_num_snodes` without raising `device_memory_GB` changes nothing on
any CUDA tier** — not just the baseline, because the pool default does not
scale with the card.

**The page-alignment assumption is now closed, and the answer does not move.**
Revision 2 declared it as an assumption because the base is set in `taichi/rhi/`,
outside my territory. Both round-two adversaries went there, so I followed and
verified the chain rather than adopt it:

- `LlvmRuntimeExecutor::preallocate_memory` (`llvm_runtime_executor.cpp:587-604`)
  calls `llvm_device()->allocate_memory` at `:594-596`.
- `CudaDevice::allocate_memory` (`taichi/rhi/cuda/cuda_device.cpp:21-48`) calls
  `mem_pool.allocate(params.size, DeviceMemoryPool::page_size, managed)` at
  `:28-29`.
- `DeviceMemoryPool::allocate` (`taichi/rhi/llvm/device_memory_pool.cpp:35-41`)
  takes an `alignment` parameter at `:36` and **never reads it**. Line 40
  forwards `allocate_raw_memory(size, managed)` only.
- `allocate_raw_memory` (`:53-92`) is a bare `CUDADriver::malloc` at `:67`.

**So nothing in this tree aligns the pool base.** It is whatever the driver
returns. The capacity is 1020 either way: the slack after 1020 headers is
`1,073,741,824 - 1,073,721,384 = 20,440` bytes, and the largest possible extra
first-request pad is 4095. The closed form above is stated from an aligned base
for legibility; no row in the table depends on it.

### 3.4 The ceiling on the pool, and who else draws from it — VERIFIED

`TI_ASSERT(total_prealloc_size <= total_mem)` at
`llvm_runtime_executor.cpp:620` means a configuration can never request more
pool than the card holds. On the baseline tier's 2 GB GTX 750,
`device_memory_fraction = 0.9` caps the pool near 1.8 GiB.

**CORRECTION, revision 3.** Revision 2 said here that the root buffers, the
runtime-objects buffer and the CUDA context "all come out of the same card
outside this pool." That is right for the runtime-objects buffer and the CUDA
context and **wrong for the root buffers** whenever `use_device_memory_pool()`
is false. Adversary `adversary2-03-2.md` found it; I read every hop before
accepting it.

The runtime-objects buffer really is separate: it is its own
`preallocate_memory` call at `llvm_runtime_executor.cpp:687-690`, feeding
`runtime_objects_chunk` at `runtime.cpp:936-941`, sized by
`runtime_get_memory_requirements` and therefore not exhaustible by SNode count.

The root buffer is not. The chain, every hop read:

| # | Site | What it does |
|---|---|---|
| 1 | `llvm_runtime_executor.cpp:419-420` | `snode_tree_buffer_manager_->allocate(rounded_size, tree_id, result_buffer)` |
| 2 | `snode_tree_buffer_manager.cpp:15` | `runtime_exec_->allocate_memory_on_device(size, result_buffer)` |
| 3 | `llvm_runtime_executor.cpp:476-491` | `llvm_device()->allocate_memory_runtime(...)`, passing `use_device_memory_pool()` as the `use_memory_pool` field at `:485` |
| 4 | `taichi/rhi/cuda/cuda_device.cpp:50-78` | branches at `:56`. `use_memory_pool` **true** takes `malloc_async` at `:57-58`, genuinely outside the pool. **False** takes `DeviceMemoryPool::allocate_with_cache` at `:60-61` |
| 5 | `taichi/rhi/llvm/device_memory_pool.cpp:27-33` | `allocator_->allocate(device, params)` at `:32` |
| 6 | `taichi/rhi/llvm/allocator.cpp:33-58` | on a cache miss, `device->allocate_llvm_runtime_memory_jit(params)` at `:54-55` |
| 7 | `cuda_device.cpp:80-90` | JIT-calls `runtime_memory_allocate_aligned` at `:82-84` |
| 8 | `runtime.cpp:879-886` | `runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)` at `:884-885` |

Step 8 is the same 1 GiB bump allocator the `ListManager`s come from.

Scope of the correction, stated exactly:

- **AMDGPU: always.** `AmdgpuDevice::allocate_memory_runtime`
  (`taichi/rhi/amdgpu/amdgpu_device.cpp:55-78`) does not branch on
  `use_memory_pool` at all. Its only branch is `host_read || host_write` at
  `:59`, whose true arm is `TI_NOT_IMPLEMENTED` at `:60` and which the sole
  caller enters with both false (`llvm_runtime_executor.cpp:480-481`); the false
  arm calls `allocate_with_cache` at `:62-63`. `allocate_llvm_runtime_memory_jit`
  (`amdgpu_device.cpp:80-90`) makes the same JIT call at `:82-84`.
  *Revision 4: revision 3 wrote "unconditionally", which is right about
  `use_memory_pool` and loose about the function. Corrected.*
- **CUDA: whenever `use_device_memory_pool()` is false.** That flag is set at
  `llvm_runtime_executor.cpp:49` from `CUDAContext::supports_mem_pool()`, which
  `taichi/rhi/cuda/cuda_context.cpp:35-50` sets only when the driver is 11.2 or
  newer (`:36-37`) **and** the device reports
  `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` (`:38-40`). Below that driver
  version the attribute is not even queried and the flag is forced 0 at `:49`.
- **CPU: not applicable.** `CpuDevice::allocate_memory_runtime`
  (`taichi/rhi/cpu/cpu_device.cpp:44-51`) routes to plain `allocate_memory`, not
  to the JIT path, and there is no pool on CPU in the first place (3.2).

**The same route carries ndarrays and argpacks.** `Ndarray::Ndarray`
(`taichi/program/ndarray.cpp:61`) and `ArgPack::ArgPack`
(`taichi/program/argpack.cpp:17-18`) both call
`Program::allocate_memory_on_device` (`taichi/program/program.h:245-248`), which
forwards to `LlvmProgramImpl::allocate_memory_on_device`
(`llvm_program.h:124-127`) and thence to step 3 above.

**INFERRED, and this is what it costs section 3.3.** Every capacity row in 3.3
assumes the whole 1 GiB is available to `ListManager`s. Where
`use_device_memory_pool()` is false, it is not: the root buffers, sized
`iroundup(root_size, taichi_page_size)` at `llvm_runtime_executor.cpp:417`, and
every ndarray and argpack, come out of the same denominator. **On that path the
1020 / 204 / 254 / 85 figures are upper bounds, not budgets.** How much they
overstate is workload-dependent, so I give no number. Escalated as E15.

I do not extend this to a claim about which of the three tiers in plan section
5.2 lands on that path. The gate is a driver version and a device attribute,
neither of which I can read here. What is established is the AMDGPU case, which
is unconditional, and the sub-11.2-driver case, which is unconditional.

### 3.5 The failure mode is a grid abort — VERIFIED

`allocate_from_reserved_memory` is a pure bump allocator. On failure,
`runtime.cpp:864-869`:

```
    __assertfail(
        "Out of CUDA pre-allocated memory.\n"
        "Consider using ti.init(device_memory_fraction=0.9) or "
        "ti.init(device_memory_GB=4) to allocate more"
        " GPU memory",
        "Taichi JIT", 0, "allocate_from_reserved_memory", 1);
```

under `#if ARCH_cuda`, with `taichi_assert_runtime(this, success, "Out of pre-allocated memory")`
at `runtime.cpp:872` as the fallback. The comment at `runtime.cpp:861-863`
explains that a `taichi_assert_runtime` alone would not halt the grid fast
enough.

**INFERRED:** exceeding the pool does not degrade performance. It aborts the
grid, at materialisation or at first listgen.

---

## 4. What section 6.1 touches, exhaustively

### 4.1 Every occurrence of the two constants — VERIFIED

Grep across the whole tree excluding `build/`. Exactly eight, and no others
anywhere, including tests, `c_api/` and CMake:

| File:line | Occurrence |
|---|---|
| `taichi/inc/constants.h:12` | `constexpr int taichi_max_num_snodes = 1024;` |
| `taichi/inc/constants.h:13` | `constexpr int kMaxNumSnodeTreesLlvm = 512;` |
| `taichi/codegen/llvm/struct_llvm.cpp:266` | `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:562` | `Ptr roots[kMaxNumSnodeTreesLlvm];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:563` | `size_t root_mem_sizes[kMaxNumSnodeTreesLlvm];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:567` | `ListManager *element_lists[taichi_max_num_snodes];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:568` | `NodeManager *node_allocators[taichi_max_num_snodes];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:569` | `Ptr ambient_elements[taichi_max_num_snodes];` |

Section 6.1 of the brief names three of the five arrays. The two
`kMaxNumSnodeTreesLlvm` arrays at 562-563 are a second SNode-scaled ceiling in
the same struct that the brief does not record.

### 4.2 Every read and write of the SNode-indexed arrays — VERIFIED

Writes, all on the create path:

- `runtime.cpp:996` `root_mem_sizes[snode_tree_id]`
- `runtime.cpp:997` `roots[snode_tree_id]`
- `runtime.cpp:1005-1006` `element_lists[i]`, `i` in `[root_id, root_id + num_snodes)`
- `runtime.cpp:1029-1030` `node_allocators[snode_id]`
- `runtime.cpp:1038-1039` `ambient_elements[snode_id]`

Reads in device code:

- `runtime.cpp:1011` `roots[snode_tree_id]`; `:1016` `element_lists[root_id]`
- `runtime.cpp:1271` `clear_list`
- `runtime.cpp:1287, 1288` `element_listgen_root`
- `runtime.cpp:1334, 1336` `element_listgen_nonroot`
- `runtime.cpp:1429` `parallel_struct_for`
- `runtime.cpp:1692` `node_gc`; `:1723, 1740, 1784` `gc_parallel_0/1/2`
- `node_pointer.h:55, 76` `node_allocators`; `:96` `ambient_elements`
- `node_dynamic.h:30, 51, 74` `node_allocators`; `:112` `ambient_elements`

Generated host-callable accessors:

- `runtime.cpp:616-619` `STRUCT_FIELD_ARRAY` for `element_lists`,
  `node_allocators`, `roots`, `root_mem_sizes`
- `runtime.cpp:749-750` `RUNTIME_STRUCT_FIELD_ARRAY` for `node_allocators`,
  `element_lists`

Both macros take the index as `int i`: `STRUCT_FIELD_ARRAY` at
`runtime.cpp:69-77`, `RUNTIME_STRUCT_FIELD_ARRAY` at `:85-88`.

**VERIFIED, and it is stronger than "no caller":** the setters those macros
generate — `LLVMRuntime_set_element_lists`, `_set_node_allocators`,
`_set_roots`, `_set_root_mem_sizes` — have **zero occurrences anywhere in the
tree** outside the macro expansion. There is no mechanism by which a slot can
be cleared, not merely no call.

Host callers:

- `llvm_runtime_executor.cpp:442-444` `runtime_initialize_snodes`
- `llvm_runtime_executor.cpp:460-462` `runtime_NodeAllocator_initialize`
- `llvm_runtime_executor.cpp:465-466` `runtime_allocate_ambient`
- `llvm_runtime_executor.cpp:262-264`, `:327-329`, `:339-341` — runtime queries
  indexed by `snode->id`

Codegen caller, the seam with agent 02:

- `taichi/codegen/llvm/codegen_llvm.cpp:2689-2692` —
  `TaskCodeGenLLVM::get_root(int snode_tree_id)` emits
  `call("LLVMRuntime_get_roots", get_runtime(), tlctx->get_constant(snode_tree_id))`,
  an i32-indexed load out of `roots[512]` in every kernel that touches a field.

None of the above is bounds-checked at the point of use.

### 4.3 The bound checks a different quantity from the index — VERIFIED

- `SNode::id` comes from one program-wide counter: `taichi/ir/snode.cpp:12`
  (`std::atomic<int> SNode::counter{0}`) and `:220` (`id = counter++`).
- Reset exactly once, in the `Program` constructor,
  `taichi/program/program.cpp:144`.
- `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`) has **no caller**. The
  only `reset_counter` call in the tree is `Stmt::reset_counter()` at
  `program.cpp:347`, a different class.
- Copying is forbidden — `SNode::SNode(const SNode &)` is `TI_NOT_IMPLEMENTED`
  (`taichi/ir/snode.cpp:230-233`) — so ids are consumed exactly once per
  constructed node and never returned.
- `Program::destroy_snode_tree` (`program.cpp:214-236`) recycles only the tree
  id, at line 235. `Program::allocate_snode_tree_id` (`:559-567`) reuses those.
- The only bound check, `taichi/codegen/llvm/struct_llvm.cpp:266`, is
  `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes)` where `snodes` is
  `StructCompiler::snodes` (`taichi/struct/struct.h:11`), filled by
  `collect_snodes` (`taichi/struct/struct.cpp:7-13`) from the single root passed
  to `StructCompilerLLVM::run` (`struct_llvm.cpp:247`), with a fresh compiler
  per tree (`llvm_program.cpp:45-56`).

**INFERRED:** 1024 is a ceiling on the *cumulative* number of SNodes ever
constructed in a `Program`'s lifetime, while the assertion bounds SNodes per
tree. Two trees of 600 SNodes each pass the assertion independently and then
index `element_lists[600..1199]`, past the end of a 1024-element array. I did
not construct a failing case.

### 4.4 `kMaxNumSnodeTreesLlvm` has no bound check at all — VERIFIED

`Program::allocate_snode_tree_id` (`program.cpp:559-567`) returns
`snode_trees_.size()` or a recycled id with no comparison to any ceiling. The
grep in 4.1 shows the constant appears only in `constants.h:13` and
`runtime.cpp:562-563`. `SNodeTreeBufferManager`
(`taichi/runtime/llvm/snode_tree_buffer_manager.cpp:12-18`) does not check it
either.

### 4.5 Contiguity assumption in the element-list loop — VERIFIED

`runtime.cpp:1003-1007` fills `element_lists[i]` over the arithmetic range
`[root_id, root_id + num_snodes)`. `num_snodes` is `(int)snode_metas.size()`
(`llvm_runtime_executor.cpp:444`) and `root_id` is `field_cache_data.root_id`
(`:400`), i.e. `tree->root()->id` (`llvm_program.cpp:61`).

The node-allocator and ambient loops (`llvm_runtime_executor.cpp:446-468`)
instead use the *actual* ids, `snode_metas[i].id`, populated from
`snodes[i]->id` at `llvm_program.cpp:113`. *Revision 2 cited 112; that line is
the `SNodeCacheData snode_cache_data;` declaration and the assignment is at 113.
Both round-two adversaries caught it. Nothing else moves.*

**INFERRED:** the element-list path requires a tree's SNode ids to be exactly
`root_id .. root_id + count - 1` with no gaps, in DFS order. Nothing in
`taichi/struct/` or `taichi/program/` asserts it.

### 4.6 A third 1024 in the dispatch path — VERIFIED

`constexpr int taichi_listgen_max_element_size = 1024;`
(`taichi/inc/constants.h:28`), used at `runtime.cpp:1316` and `:1369` to cap the
per-element loop-bound split during listgen, and mirrored in codegen at
`codegen_llvm.cpp:2291`. It is not SNode-count-scaled so it is not a 6.1 item,
but it is a hard 1024 in the struct-for dispatch path and brief section 6.4
makes dispatch throughput the sole criterion. *The first revision's "exhaustive"
array sweep missed this because it grepped for array declarations rather than
for uses of `inc/constants.h` values. Adversary 03-1 found it.*

Also **VERIFIED**: `taichi_max_num_mem_requests = 1024 * 64`
(`constants.h:16`) has no use anywhere in the tree.

### 4.7 The build/install seam — VERIFIED

`runtime.cpp` is not linked into the host binary. It is compiled to bitcode,
once per architecture, `taichi/runtime/llvm/runtime_module/CMakeLists.txt:6-11`:

```
 8      COMMAND ${CLANG_EXECUTABLE} ${CLANG_OSX_FLAGS} -c runtime.cpp -o "runtime_${rtm_arch}.bc"
        -fno-exceptions -emit-llvm -std=c++17 -D "ARCH_${rtm_arch}" -I ${PROJECT_SOURCE_DIR};
```

driven over `HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH` (`:29-31`) and
installed to `${CMAKE_INSTALL_PREFIX}/python/taichi/_lib/runtime` (`:13`). The
`-I ${PROJECT_SOURCE_DIR}` pulls in `taichi/inc/constants.h`, so the array
sizes are baked into the bitcode. Line 10 sets the working directory to the
source tree, with an upstream TODO at line 9 noting the pollution.

**A latent fault in that install rule, VERIFIED.** Line 8 uses the function
parameter `${rtm_arch}`; line 13 uses `${arch}`, which is not a parameter but
the caller's `foreach` variable at lines 29-30, read through CMake's
parent-scope visibility. It holds the same value at call time, so it works
today by coincidence. Anyone parameterising this function for item 6.1 or 6.3
will trip on it. *Found by adversary 03-1; verified here.*

The bitcode is loaded back at run time from the install tree:
`TaichiLLVMContext::module_from_file` (`llvm_context.cpp:358-362`) reads
`{runtime_lib_dir()}/{get_runtime_fn(arch)}`; `get_runtime_fn`
(`:209-211`) formats `runtime_{arch}.bc`; `runtime_lib_dir()`
(`taichi/util/lang_util.cpp:30-45`) resolves from `compiled_lib_dir` or
`TI_LIB_DIR`.

The host has **no mirror declaration** of `LLVMRuntime` —
`taichi/program/context.h:11` is a bare forward declaration — and codegen
fetches the type from the loaded bitcode by name:
`TaichiLLVMContext::get_runtime_type` (`llvm_context.cpp:1004-1011`), called
for `"LLVMRuntime"` at `codegen_llvm.cpp:2697`.

**INFERRED:** layout is single-sourced, so raising the constant needs no
host-side struct edit. But the host binary compiles its own copy of
`taichi_max_num_snodes` for the assertion at `struct_llvm.cpp:266`, and by 4.3
that assertion is not checking the array bound anyway. So a value mismatch
between the installed `.bc` and the host binary has **no detection path at
all**.

`taichi/runtime/llvm/runtime_module/cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc`
(3744 bytes) is a different artifact, built from `cuda_runtime.cu` by
`COMPILE_CUSTOM_CUDA_LIBRARY` (`CMakeLists.txt:17-26`), whose only call site is
commented out at lines 36-42. It does not carry the runtime struct.

---

## 5. SNodeTree lifecycle and the contiguous chunk backing

**Every citation in this section was regenerated. The first revision read
`snode_tree.h` and `snode_tree.cpp` through a single `cat -n` and recorded the
concatenated numbering, so every `.cpp` line was 54 too high. Diagnosis in
notes entry 22.**

### 5.1 The classes — VERIFIED

`taichi/struct/snode_tree.h` is 54 lines: `kFirstID = 0` at line 17,
`int id_` at 40, `std::unique_ptr<SNode> root_` at 41,
`get_snodes_to_root_id` declared at 52.

`taichi/struct/snode_tree.cpp` is **41 lines**:

| Item | Lines |
|---|---|
| `get_snodes_to_root_id_impl` (anonymous namespace) | 6-13 |
| `SNodeTree::SNodeTree` | 17-20 |
| `SNodeTree::check_tree_validity` | 22-32 |
| `get_snodes_to_root_id` | 34-39 |

`check_tree_validity` (22-32) only enforces that non-`place`, non-`root` nodes
have at least one child.

`taichi/struct/struct.h` is 26 lines: `std::vector<SNode *> stack` at 10,
`snodes` at 11, `std::size_t root_size{0}` at 12.
`taichi/struct/struct.cpp` is 15 lines; `collect_snodes` at 7-13 is a preorder
DFS `push_back`, with `int ch_id` at line 9 the only width in the file.

The doc comment at `snode_tree.h:13` — "backed by a contiguous chunk of
memory" — is not implemented in this class.

### 5.2 Where the chunk actually is — VERIFIED

**LLVM path.** `LlvmRuntimeExecutor::initialize_llvm_runtime_snodes`
(`llvm_runtime_executor.cpp:391-468`):

- `:417` `std::size_t rounded_size = taichi::iroundup(root_size, taichi_page_size);`
- `:419-420` `snode_tree_buffer_manager_->allocate(rounded_size, tree_id, result_buffer)`
- `:421-435` zero-fill via `CUDADriver::memset`, `AMDGPUDriver::memset` or `std::memset`
- `:437-438` `llvm_device()->import_memory(root_buffer, rounded_size)`, stored
  into `snode_tree_allocs_[tree_id]` at `:440`

`SNodeTreeBufferManager::allocate`
(`taichi/runtime/llvm/snode_tree_buffer_manager.cpp:12-18`) wraps
`allocate_memory_on_device` and keeps
`std::map<int, DeviceAllocation> snode_tree_id_to_device_alloc_`
(`snode_tree_buffer_manager.h:28`). `root_size` originates at
`struct_llvm.cpp:269-270` and is `std::size_t` throughout.

**gfx path.** `SNodeTreeManager::materialize_snode_tree`
(`taichi/runtime/gfx/snode_tree_manager.cpp:11-16`) calls
`GfxRuntime::add_root_buffer(compiled_structs.root_size)` at line 14 and
`compiled_snode_structs_.push_back(...)` at line 15.
`GfxRuntime::add_root_buffer` (`taichi/runtime/gfx/runtime.cpp:730-750`)
allocates one storage buffer per tree, zero-fills it, and pushes onto
`root_buffers_` at line 747. Empty roots get 4 bytes (`:731-733`).

### 5.3 Add path — VERIFIED

`Program::add_snode_tree` (`program.cpp:238-255`): allocate id (240), construct
(241), `set_snode_tree_id` (242), compile or materialise (243-247), store
(248-253).

`LlvmProgramImpl::materialize_snode_tree` (`llvm_program.cpp:67-76`) →
`compile_snode_tree_types` (`:58-65`) → `compile_snode_tree_types_impl`
(`:45-56`) and `cache_field` (`:97-122`) → `initialize_llvm_runtime_snodes`.

`GfxProgramImpl::materialize_snode_tree`
(`taichi/runtime/program_impls/gfx/gfx_program.cpp:25-36`).

### 5.4 Destroy path — VERIFIED

`Program::destroy_snode_tree` (`program.cpp:214-236`): arch assertion 215-218,
`remove_rw_accessor_cache` called at 232, `program_impl_->destroy_snode_tree` 234,
`free_snode_tree_ids_.push(snode_tree->id())` 235.

`LlvmProgramImpl::destroy_snode_tree` (`llvm_program.h:97-103`) erases the
offline cache entry and calls `LlvmRuntimeExecutor::destroy_snode_tree`
(`llvm_runtime_executor.cpp:758-761`), which is two lines:
`TaichiLLVMContext::delete_snode_tree(id)` (`llvm_context.cpp:968-973`) and
`SNodeTreeBufferManager::destroy` (`snode_tree_buffer_manager.cpp:20-24`).

`GfxProgramImpl::destroy_snode_tree` (`gfx_program.h:39-42`) →
`SNodeTreeManager::destroy_snode_tree` (`snode_tree_manager.cpp:18-29`), which
scans `compiled_snode_structs_` for the matching root pointer (20-24), errors
if not found (25-27), and resets `root_buffers_[root_id]` at line 28. Neither
vector shrinks.

### 5.5 What destroy leaves behind — VERIFIED

- Nothing clears `element_lists[i]`, `node_allocators[i]` or
  `ambient_elements[i]` for the destroyed tree's ids, and there is no setter
  with a caller anywhere (section 4.2).
- Nothing clears `roots[tree_id]` or `root_mem_sizes[tree_id]`.
- No free path: `allocate_from_reserved_memory` (`runtime.cpp:838-874`) only
  advances `preallocated_head` (line 853). The pool as a whole is a
  `DeviceAllocationGuard` (`llvm_runtime_executor.cpp:587-604`), so it is
  released at executor teardown. The leak is unbounded within a `Program`'s
  life and reclaimed when the `Program` dies.
- `snode_tree_allocs_` is never erased. Three occurrences only: declared
  `llvm_runtime_executor.h:152`, written `:440`, read `:387`. Meanwhile
  `SNodeTreeBufferManager::destroy` really does free the underlying allocation
  and erase its own map entry, so the entry left in `snode_tree_allocs_` is a
  dangling `DeviceAllocation` that `get_snode_tree_device_ptr` (`:386-389`)
  will hand out on a recycled tree id. *Found by adversary 03-2; verified here.*
- `Program::destroy_snode_tree` does not reset `snode_trees_[id]`. The only
  writes are `program.cpp:249` (overwrite on id reuse) and `:252`
  (`push_back`), so the `SNodeTree` and its `SNode` objects survive until the
  slot is reused.
- The tree's own root buffer **is** released, through
  `SNodeTreeBufferManager::destroy` →
  `LlvmRuntimeExecutor::deallocate_memory_on_device`
  (`llvm_runtime_executor.cpp:493-498`) → `llvm_device()->dealloc_memory`.
  *Revision 3: "freed" is path-conditional, on the same split as 3.4.*
  `CudaDevice::dealloc_memory` (`cuda_device.cpp:92-117`) takes
  `mem_free_async` at `:107-108` only when `info.use_memory_pool` is set. On the
  non-mem-pool path `info.use_cached` is true — it is assigned unconditionally
  at `cuda_device.cpp:68` — so `:109-111` returns the block to
  `DeviceMemoryPool::release` (`device_memory_pool.cpp:43-51`) and thence to
  `CachingAllocator::release` (`allocator.cpp:60-68`), which puts it on a free
  list. It is reusable by later runtime-memory allocations. It is never returned
  to the bump head at `runtime.cpp:853`. That is consistent with the "no free
  path" statement above; only the word "freed" was doing too much work.

Per standing instruction 3 I record these as facts and do not judge them.

---

## 6. Reachability of the lifecycle findings

**VERIFIED, and it was not established in the first revision.**
`grep -rn destroy_snode_tree` over the whole tree: `Program::destroy_snode_tree`
(`program.cpp:214-236`) has exactly one caller, the pybind lambda at
`taichi/python/export_lang.cpp:569-570`, driven from
`python/taichi/_snode/snode_tree.py:21`. There is no C++ core caller and no
test caller.

**INFERRED:** brief section 1.2 puts the Python front end out of scope, so the
stale runtime tables in 5.5 and the gfx numbering divergence in 7.2 are
**latent today, not live**. They become reachable the moment this fork drives
the core from C++ and calls `Program::destroy_snode_tree`. Brief section 4.6
describes the engine SNode configuration as "a materialised working set, not
the store itself", which implies recomposition. Whether that path is built is
in Escalations.

---

## 7. gfx / SPIR-V side of the territory

### 7.1 No fixed-size SNode tables — VERIFIED

Neither constant appears anywhere under `taichi/runtime/gfx/` or
`taichi/runtime/program_impls/gfx/` — confirmed by the grep in 4.1. That path
uses growable containers only:
`std::vector<CompiledSNodeStructs> compiled_snode_structs_`
(`taichi/runtime/gfx/snode_tree_manager.h:39`),
`std::vector<std::unique_ptr<DeviceAllocationGuard>> root_buffers_`
(`taichi/runtime/gfx/runtime.h:152`), and
`std::unordered_map<DeviceAllocation *, size_t> root_buffers_size_map_`
(`runtime.h:164`).

**INFERRED:** the two ceilings, and the 1 GiB pool of section 3, are
LLVM-path-only. Raising them changes nothing on Vulkan, OpenGL, DX11 or Metal.

Widths: `get_field_in_tree_offset(int tree_id, const SNode *child)` returns
`size_t` and accumulates `mem_offset_in_parent_cell` in `size_t`
(`snode_tree_manager.cpp:31-47`); `get_snode_tree_device_ptr(int tree_id)`
(`:49-51`); `get_root_buffer(int id)` / `get_root_buffer_size(int id)`
(`gfx/runtime.cpp:752-765`); `RegisterParams::num_snode_trees` is `std::size_t`
(`gfx/runtime.h:40, 94`).

### 7.2 The two numberings do diverge — VERIFIED, closed

*The first revision escalated this unresolved. With the corrected line numbers
it is determinable from four functions, and it diverges. Adversary 03-2
constructed it; I re-derived it against the source.*

1. `materialize_snode_tree` (`snode_tree_manager.cpp:11-16`) only `push_back`s,
   so the vector index is materialisation order.
2. `destroy_snode_tree` (`:18-29`) resets `root_buffers_[root_id]` at line 28
   and leaves both vectors' lengths unchanged.
3. `Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
   (`program.cpp:235`); `allocate_snode_tree_id` pops it (`:559-567`).
4. `get_snode_tree_device_ptr(int tree_id)` (`snode_tree_manager.cpp:49-51`)
   returns `runtime_->root_buffers_[tree_id]->get_ptr()`.

Create trees 0 and 1; destroy 1; create a third. The third receives
`tree_id == 1` from the free stack, but its buffer is pushed at index 2, and
`root_buffers_[1]` is the `unique_ptr` reset in step 2.
`get_snode_tree_device_ptr(1)` then dereferences null, and
`get_field_in_tree_offset(1, ...)` (`:31-47`) reads the destroyed tree's
descriptors and trips its own `TI_ASSERT_INFO` at `:34-38`.

**Divergence after one destroy-then-add cycle.** Latent today, per section 6.

The LLVM path does not have the equivalent problem: `snode_tree_allocs_` is an
`unordered_map` keyed by tree id (`llvm_runtime_executor.h:152`), so a recycled
id overwrites rather than shifts.

---

## 8. 32-bit width inventory

### 8.1 Verified narrowing conversions already present

**A. `StructMeta::max_num_elements` is `i64`; every accessor returns `i32`.**

- `runtime.cpp:310` `i64 max_num_elements;`
- `runtime.cpp:318` `i32 (*get_num_elements)(Ptr, Ptr);`
- `node_dense.h:10-12`, `node_pointer.h:10-12`, `node_bitmasked.h:10-12` each
  `return ((StructMeta *)meta)->max_num_elements;` from an `i32` function — an
  implicit i64 → i32 truncation of a field that is already 64-bit.
- `node_dynamic.h:116-119` returns `node->n`, an `i32` in `DynamicNode`
  (`node_dynamic.h:3-7`). `node_root.h:21-23` returns literal 1.

**B. `Ndarray::nelement_` is computed in `int`.**
`taichi/program/ndarray.cpp:35-38` and `:75-78`:
```
nelement_(std::accumulate(std::begin(shape_), std::end(shape_), 1,
                          std::multiplies<>())),
```
`nelement_` is `std::size_t` (`ndarray.h:80`) and `shape_` is
`std::vector<int>` (`ndarray.h:55`), but `std::accumulate` deduces its
accumulator from the init value, the `int` literal `1`. The product overflows
in `int` before the conversion, then feeds
`allocate_memory_on_device(nelement_ * element_size_, ...)` at
`ndarray.cpp:61`. The same idiom at `:53` and `:96` uses `1LL` and is clean.

**C. Explicit i64 → i32 shape truncation.**
`set_arg_external_array_with_shape`
(`taichi/program/launch_context_builder.cpp:220-242`) takes
`const std::vector<int64> &shape` and writes each entry as `(int32)shape[i]` at
line 240. `set_arg_ndarray_impl` (`:308-330`) does the same at line 326.

**D. Host reads device `size_t` fields as `int32`.**
`RUNTIME_STRUCT_FIELD(ListManager, max_num_elements_per_chunk)` and
`(ListManager, element_size)` (`runtime.cpp:759, 760`) expose fields declared
`std::size_t` (`runtime.cpp:429-430`), while the host reads them through
`runtime_query<int32>` (`llvm_runtime_executor.cpp:193-198`), which union-casts
the low bytes of a `u64`.

### 8.2 Integer byte-offset arithmetic in the node accessors — VERIFIED

`node_pointer.h` computes offsets in `int` before adding to the pointer:
line 44 `node + 8 * i`, line 45 `node + 8 * (num_elements + i)`, and the same
shape at 69, 70, 86, 92. `8` is an `int` literal, `i` is `int`, `num_elements`
is `i32`.

**INFERRED:** this overflows at `i >= 2^28`, eight times tighter than the i32
index itself.

`node_dense.h:22-23` and `node_bitmasked.h:41-43` use
`node + ((StructMeta *)meta)->element_size * i` with `element_size` a
`std::size_t`, so those promote to 64-bit and are bounded only by `int i`.

`node_dynamic.h`: `chunk_start` `int` (22, 67, 99), `DynamicMeta::chunk_size`
`int` (11), `DynamicNode::n` `i32` (5);
`(i - chunk_start) * meta->element_size` (81, 105) promotes;
`atomic_max_i32` (21), `atomic_add_i32` (65).

`node_bitmasked.h:18-20, 27-29`: `mask_begin[i / 32]`, `1UL << (i % 32)`.

### 8.3 `ListManager` — `runtime.cpp:426-513`

Upstream's own comment sits at `runtime.cpp:424-425`:
"TODO: there are many i32 types in this class, which may be an issue if there
are >= 2 ** 31 elements."

| Line | Declaration |
|---|---|
| 427-428 | `max_num_chunks = 128 * 1024;` / `Ptr chunks[max_num_chunks];` |
| 429-430 | `std::size_t element_size`, `max_num_elements_per_chunk` (already 64-bit) |
| 431-433 | `i32 log2chunk_num_elements`, `i32 lock`, `i32 num_elements` |
| 451-456 | `i32 reserve_new_element()` |
| 465 | `void touch_chunk(int chunk_id);` (defined 1664-1679) |
| 467-473 | `i32 get_num_active_chunks()` — `int i` loop over all 131072 |
| 475-477 | `void clear()` — sets `num_elements = 0` only |
| 479-481 | `void resize(i32 n)` |
| 483-486 | `Ptr get_element_ptr(i32 i)` — the linearised offset |
| 489-491 | `T &get(i32 i)` |
| 493-496 | `Ptr touch_and_get(i32 i)` |
| 498-500 | `i32 size()` |
| 502-512 | `i32 ptr2index(Ptr ptr)` |

The linearised offset at 483-486 is
`chunks[i >> log2chunk_num_elements] + element_size * (i & ((1 << log2chunk_num_elements) - 1))`.
The `element_size` multiply promotes; the shift, mask and index are i32.

**`ptr2index` is arch-dependent. CORRECTED IN REVISION 3.**

Revision 2 said "it terminates at the first untouched chunk and is O(chunks
touched)." **That is the GPU behaviour stated as universal, and it is wrong on
the CPU build.** Both round-two adversaries found it, one of them withdrawing
its own round-one position that revision 2 had adopted. I went to the assert
implementation myself rather than accept the ruling.

Line 505 is
`taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.")`, inside
the loop and before the range test at 506, and `enable_assert` is
`constexpr bool ... = true` (`runtime.cpp:339`). `taichi_assert_runtime`
(`runtime.cpp:817-819`) forwards to `taichi_assert_format`
(`runtime.cpp:766-815`), which:

- returns immediately at `:780-781` when the test **passes**;
- on failure records the error once under `if (!runtime->error_code)` at `:782`;
- then kills the thread **only** under `#if ARCH_cuda` (`asm("exit;")` at
  `:798-800`) or `#elif ARCH_amdgpu` (`asm("S_ENDPGM")` at `:801-802`). The
  `#endif` is at `:814` and the function then returns normally.

**There is no CPU branch.** The host-arch bitcode is built with `-D ARCH_x64`
(`runtime_module/CMakeLists.txt:8`, driven over `HOST_ARCH` at `:29`), so
neither kill is compiled in. Control returns to the loop, line 506 tests
`chunks[i] <= ptr` against `nullptr` — which holds — and
`ptr < chunks[i] + chunk_size`, which does not, so no iteration matches and none
breaks.

| Case | Behaviour |
|---|---|
| pointer found, any arch | exits at that chunk, O(index of the containing chunk) |
| pointer not found, CUDA / AMDGPU | thread dies at the first untouched chunk |
| pointer not found, **CPU** | error flagged once, then **all 131072 iterations**, returning -1 at `:511` |

It is reached from `Pointer_deactivate` (`node_pointer.h:67-82`) and
`Dynamic_deactivate` (`node_dynamic.h:43-59`) via `NodeManager::recycle`
(`runtime.cpp:683-686`) → `locate` (`:679-681`). The not-found case is an error
path, reached only if `recycle` is handed a pointer the `data_list` never
issued. Plan section 2 makes CPU-only a first-class target, so the arch on which
this is unbounded is the one the plan puts first. Recorded as a fact, not
judged.

`get_num_active_chunks` (`:467-473`) genuinely is unconditional over 131072,
but its only caller is `runtime_ListManager_get_num_active_chunks`
(`runtime.cpp:743-747`) from the host debug printer `print_list_manager_info`
(`llvm_runtime_executor.cpp:188-209`).

**INFERRED capacity inconsistency:** `max_num_chunks * max_num_elements_per_chunk`
for element lists is `131072 * 65536 = 2^33` slots, four times what the `i32
num_elements` counter at line 433 can index.

### 8.4 `NodeManager` — `runtime.cpp:630-707`

`i32 lock` (632), `i32 element_size` (634), `i32 chunk_num_elements` (635),
`i32 free_list_used` (636), `i32 recycle_list_size_backup` (639),
`using list_data_type = i32` (641) — so free and recycled lists store element
indices as 32-bit values. `allocate()` 666-677, `locate()` 679-681,
`recycle()` 683-686, `gc_serial()` 688-706, all `int`/`i32`.

The one 64-bit-safe spot is the 128 MB cap comparison at `:652-655`,
`(uint64)chunk_num_elements * element_size`.

### 8.5 Struct-for, listgen and dispatch, all `int` — VERIFIED

| Line(s) | Item |
|---|---|
| `runtime.cpp:288-290` | `PhysicalCoordinates { i32 val[taichi_max_num_indices]; }` |
| `runtime.cpp:308` | `StructMeta::snode_id` is `i32` |
| `runtime.cpp:312, 316, 322` | `lookup_element(..., int i)`, `is_active(..., int i)`, `refine_coordinates(..., int index)` |
| `runtime.cpp:517-521` | `Element { Ptr element; int loop_bounds[2]; PhysicalCoordinates pcoord; }` |
| `runtime.cpp:1270-1273` | `clear_list` |
| `runtime.cpp:1282-1329` | `element_listgen_root` — `int c_start`, `int c_step`, `int c`; **int multiplies at 1319 (loop condition), 1322 and 1323** |
| `runtime.cpp:1331-1383` | `element_listgen_nonroot` — `int num_parent_elements`, `int i`, `int j`, `int j_lower`, `int j_higher`; `taichi_listgen_max_element_size` at 1369 |
| `runtime.cpp:1387-1394` | `cpu_block_task_helper_context` — `int element_size`, `int element_split` |
| `runtime.cpp:1403-1420` | `cpu_struct_for_block_helper` — `int element_id`, `int part_size`, `int part_id`, `int lower`, `int upper` |
| `runtime.cpp:1422-1465` | `parallel_struct_for` — `int snode_id`, `int element_size`, `int element_split`; `list_tail` is `i32`; `list_tail * element_split` at 1461-1462 |
| `runtime.cpp:1471-1481` | `range_task_helper_context` — `int begin`, `end`, `block_size`, `step` |
| `runtime.cpp:1483-1509` | `cpu_parallel_range_for_task` — `ctx.begin + task_id * ctx.block_size` |
| `runtime.cpp:1511-1538` | `cpu_parallel_range_for` — grid count in int |
| `runtime.cpp:1541-1562` | `gpu_parallel_range_for` |
| `runtime.cpp:1567-1575` | `mesh_task_helper_context` — `int num_patches`, `int block_size` |
| `runtime.cpp:1577-1597`, `1599-1623`, `1627-1647` | the mesh-for trio |
| `runtime.cpp:1650-1656` | `i32 linear_thread_idx(RuntimeContext *)` |
| `runtime.cpp:986-994` | `runtime_initialize_snodes(..., const int root_id, const int num_snodes, const int snode_tree_id, ...)` |
| `runtime.cpp:1026-1031`, `1033-1040` | `int snode_id` |
| `runtime.cpp:1691-1693`, `1721`, `1738`, `1782` | `node_gc` and `gc_parallel_0/1/2`, `int snode_id` |
| `runtime.cpp:1694-1719`, `1725-1736`, `1742-1780` | the three `gc_parallel_impl_*` |
| `runtime.cpp:589`, `593`, `595` | `error_message_lock`, `allocator_lock`, `num_rand_states`, all `i32` |
| `runtime.cpp:891-893` | `runtime_get_memory_requirements(Ptr, i32, i32)` |
| `runtime.cpp:69-77`, `85-88` | `STRUCT_FIELD_ARRAY` / `RUNTIME_STRUCT_FIELD_ARRAY` take `int i` |
| `taichi/program/context.h:20` | `RuntimeContext::cpu_thread_id` is `int32_t` |

### 8.6 Host-side `int` on SNode and tree identity — VERIFIED

| Line | Item |
|---|---|
| `taichi/struct/snode_tree.h:40, 52` | `int id_`; `get_snodes_to_root_id` returns `unordered_map<int,int>` |
| `taichi/struct/struct.cpp:9` | `int ch_id` loop |
| `taichi/program/program.h:115, 189, 204, 214, 335-336` | tree signatures, `snode_trees_`, `free_snode_tree_ids_` |
| `taichi/program/program_impl.h:94` | `virtual DevicePtr get_snode_tree_device_ptr(int tree_id)` |
| `taichi/runtime/llvm/llvm_offline_cache.h:68, 76-77, 119` | `SNodeCacheData::id`, `tree_id`, `root_id`, `unordered_map<int, FieldCacheData> fields` |
| `taichi/runtime/llvm/snode_tree_buffer_manager.h:20-22, 28` | `allocate(std::size_t, const int, uint64 *)`, `std::map<int, DeviceAllocation>` |
| `taichi/runtime/llvm/llvm_runtime_executor.h:97, 133, 152` | `get_snode_tree_device_ptr(int)`, `fetch_result_uint64(int, ...)`, `snode_tree_allocs_` |
| `taichi/runtime/llvm/llvm_context.h:57, 114, 121, 163` | `add_struct_module(..., int)`, `get_struct_function(..., int)`, `delete_snode_tree(int)`, `snode_tree_funcs_` |
| `taichi/runtime/gfx/snode_tree_manager.h:33, 35` | the two `int tree_id` accessors |
| `taichi/program/ndarray.h:55, 82` | `std::vector<int> shape`, `total_shape_` |
| `taichi/runtime/llvm/llvm_runtime_executor.cpp:190-201, 268, 348-354` | `runtime_query<int32>` for list lengths and counters |

**INFERRED constraint on item 6.2 that the first revision listed the fields for
but did not draw:** `SNodeCacheData` and `FieldCacheData` are serialised —
`TI_IO_DEF(id, type, cell_size_bytes, chunk_size)` at
`llvm_offline_cache.h:73` and `TI_IO_DEF(tree_id, root_id, root_size, snode_metas)`
at `:81`. Widening those ids changes the on-disk offline-cache format.

### 8.7 Already 64-bit — VERIFIED

`ListManager::element_size`, `max_num_elements_per_chunk` (`runtime.cpp:429-430`);
`StructMeta::element_size` (`:309`), `max_num_elements` (`:310`);
`root_mem_sizes` (`:563`); `error_code` (`:590`), `total_requested_memory`
(`:597`); `PreallocatedMemoryChunk` (`:546-550`);
`allocate_aligned` / `allocate_from_reserved_memory` (`:823-835`, `:838-874`);
`runtime_get_memory_requirements` accumulates in `i64` (`:894-905`);
`get_temporary_pointer(LLVMRuntime *, u64)` (`:723-725`);
the local-stack ops (`:1876-1897`);
`SNodeTreeBufferManager::allocate` size (`snode_tree_buffer_manager.h:20`);
`allocate_memory_on_device(std::size_t, ...)` (`llvm_runtime_executor.h:51`);
`FieldCacheData::root_size`, `cell_size_bytes`, `chunk_size`
(`llvm_offline_cache.h:70-71, 78`);
`StructCompiler::root_size` (`struct.h:12`);
`array_runtime_sizes` values `uint64` (`launch_context_builder.h:125-126`),
`arg_buffer_size`, `result_buffer_size` (`:114, 116`);
`Ndarray::nelement_`, `element_size_` (`ndarray.h:80-81`), `flatten_index`
(`ndarray.cpp:16-23`);
gfx `get_field_in_tree_offset` (`snode_tree_manager.cpp:31-47`),
`add_root_buffer(size_t)` (`gfx/runtime.cpp:730`),
`num_snode_trees` (`gfx/runtime.h:40, 94`).

### 8.8 Cross-boundary corroboration (agent 01's territory)

`taichi/ir/snode.cpp:88-101`:

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

with the matching ndarray warnings at `ndarray.cpp:55-60` and `:98-103` and
`TaichiIndexWarning` at `taichi/common/exceptions.h:114-117`. Upstream
diagnoses the exact condition section 6.2 targets and warns rather than
supporting it.

---

## 9. Checked and found nothing

**VERIFIED** by grep and by reading. No SNode-count-scaled storage and no
addressing-relevant 32-bit width in:
`taichi/program/argpack.*`, `callable.*`, `compile_config.*` (beyond the two
memory knobs in section 3), `conjugate_gradient.*`, `extension.*`,
`field_info.*`, `function.*`, `function_key.*`, `graph_builder.*`, `kernel.*`,
`kernel_launcher.h`, `kernel_profiler.*`, `matrix.h`, `parallel_executor.*`,
`py_print_buffer.*`, `snode_expr_utils.*`, `snode_rw_accessors_bank.*`,
`sparse_matrix.*`, `sparse_solver.*`, `texture.*`;
`taichi/runtime/{cpu,cuda,amdgpu}/`; `taichi/runtime/dx12/`,
`taichi/runtime/gfx/aot_*`, `taichi/runtime/llvm/llvm_aot_*` (agent 04).

Fixed-size arrays in the three directories, other than the five in
`LLVMRuntime` and `ListManager::chunks`:
`PhysicalCoordinates::val[taichi_max_num_indices]` (`runtime.cpp:289`),
`Element::loop_bounds[2]` (`:519`), `error_message_template[2048]` and
`error_message_arguments[32]` (`:587-588`), `printf_helper::buffer[1024]`
(`:1831`), `Ptr ptrs[24]` and `Ptr ptrs[kN]` in the self-tests
(`internal_functions.h:84, 119`), `options[max_num_options]` and
`option_values[max_num_options]` (`taichi/runtime/cuda/jit_cuda.cpp:28-29`),
`attr_val[8]` (`taichi/runtime/amdgpu/kernel_launcher.cpp:8`);
and — *added in revision 3, missed by the revision-2 sweep and found by both
round-two adversaries* — three thread-local scratch buffers in `runtime.cpp`:
`alignas(8) char tls_buffer[64]` at `:1552` in `gpu_parallel_range_for` and at
`:1636` in `gpu_parallel_mesh_for`, both inside `#ifdef ARCH_amdgpu` because
AMDGPU does not support a dynamic array (`:1550-1551`, `:1634-1635`), and
`alignas(8) char tls_buffer[1]` at `:1435` in `parallel_struct_for`, inside
`#if ARCH_cuda || ARCH_amdgpu` and rewritten to `tls_buffer_size` during codegen
(`:1433-1434`). The sibling declarations at `:1412`, `:1487`, `:1554`, `:1581`
and `:1638` are variable-length and are not fixed-size arrays.

None of the above scale with SNode count, so the conclusion of this section
stands unchanged; only the word "exhaustive" did not. **This inventory covers
array declarations only; section 4.6 records the `inc/constants.h` value that a
declaration-shaped grep misses.**

One live 32-bit index in `internal_functions.h`: the sparse-matrix triplet
insert `insert_triplet_f32` / `_f64` (lines 30-45) via `ATOMIC_INSERT(T)`
(lines 12-21). For the f32 variant `triplet_id` is an `int32` from
`atomic_add_i32` indexing `data_base_ptr[triplet_id * 3]`. Sparse matrix
builder, not SNode addressing.

---

## 10. Escalations

Unresolved. Each needs a decision the project owner has not made.

**E1 — The replacement for 1024 is a pair, not a number, and the two halves do
not cost the same to move.** Section 3. `taichi_max_num_snodes`
(`constants.h:12`) and `device_memory_GB` (`compile_config.cpp:63`) bind within
0.4% of each other today, so raising one alone is inert on CUDA and AMDGPU.

**CORRECTION, revision 3.** Revision 2 ended this escalation with "at C++ level
it is currently a hard-coded 1." **That is false**, and it is the one place
where the error changes what the planner would decide. Adversary
`adversary2-03-2.md` raised it; I verified every link:

- `CompileConfig` is a plain struct with public fields
  (`taichi/program/compile_config.h:8`). `device_memory_GB` and
  `device_memory_fraction` are public `float64` at `:71-72`.
- `extern TI_DLL_EXPORT CompileConfig default_compile_config;` at
  `compile_config.h:110`, defined at `taichi/util/lang_util.cpp:14`.
- `Program::Program(Arch)` copies that global wholesale:
  `config = default_compile_config;` at `taichi/program/program.cpp:75`, then
  `config.fit()` at `:77`.
- The pybind binding at `taichi/python/export_lang.cpp:264-267` hands out a
  reference to the **same** global. Python is a client of it, not its owner.
  The `def_readwrite` at `:208-210` binds the struct field, not a private one.

So a C++ driver writes `taichi::lang::default_compile_config.device_memory_GB`
before constructing a `Program`, with no new mechanism and no Python. Note also
that both knobs are `float64`, so fractional values are legal.

**The consequence, which is why this matters.** The two halves of the pair in
plan section 8.1 item 2 are not symmetric:

| Half | What moving it costs |
|---|---|
| `taichi_max_num_snodes` | `constexpr` at `constants.h:12`, pulled into the per-arch bitcode by `-I ${PROJECT_SOURCE_DIR}` at `runtime_module/CMakeLists.txt:8` and baked there. Needs a **build-time** mechanism, and per section 4.7 the host binary's own copy has no detection path against the installed `.bc`. |
| `device_memory_GB` | A public field on an exported global, read at `Program` construction. Needs **no new mechanism at all**. |

Revision 2, and both explore reports, told the planner the opposite. Whether
`device_memory_GB` joins the install-time parameterisation of item 6.1 or is
separate work remains the planner's call; what has changed is that it is the
cheap half, not the blocked one. Plan section 5 makes configuration an
install-time decision, and both knobs qualify.

The same correction applies to `demote_dense_struct_fors` in E2: it is a public
field at `compile_config.h:28` on the same global, not Python-only.

**E2 — `demote_dense_struct_fors` is a third dial.** Section 2.6. Default true
(`compile_config.cpp:18`), forced true for SPIR-V (`:72-74`). With it false, a
purely dense configuration pays the full per-SNode `ListManager` cost. Whether
the install-time loader may touch it is undecided.

**E3 — `ListManager::max_num_chunks` and the element-list chunk size.**
`runtime.cpp:427` sets the 1 MiB header; the `1024 * 64` at `runtime.cpp:1006`
sets the 4 MiB first-touch chunk. Together they are the per-SNode cost in
section 2.5. Whether either is in scope for 6.1 is undecided. I propose no
change to either and call neither surplus.

**E4 — The per-tree bound versus the global index.** Section 4.3. What the
correct invariant should be is a design decision that changes what "raise 1024
to X" means.

**E5 — `kMaxNumSnodeTreesLlvm = 512` is a second ceiling the plan does not
record, bounds-checked nowhere.** Sections 4.1 and 4.4. Whether it is
parameterised alongside 1024 is the planner's call.

**E6 — Will the C++ core call `Program::destroy_snode_tree`?** Section 6.
Today the only caller is the Python binding, out of scope by brief 1.2. If this
fork builds a C++ recompose path, sections 5.5 and 7.2 become live. Brief
section 4.6 suggests it will.

**E7 — Host binary and installed `.bc` carry independent copies of the
constant, with no detection path.** Section 4.7. Overlaps agent 04's build
territory.

**E8 — The install-rule variable fault at `runtime_module/CMakeLists.txt:13`.**
Section 4.7. Latent today, surfaces under parameterisation. Agent 04's
territory; flagged, not chased.

**E9 — `taichi_listgen_max_element_size = 1024`** (`constants.h:28`). Section
4.6. A third 1024 in the dispatch path. Whether it is in scope is undecided.

**E10 — Widening SNode and tree ids changes the offline-cache on-disk format.**
Section 8.6, `llvm_offline_cache.h:73, 81`.

**E11 — `runtime_initialize_snodes` assumes contiguous ids from `root_id`.**
Section 4.5. Whether `lazy_grad`, `lazy_dual` or `allocate_adjoint_checkbit`
(`taichi/ir/snode.h:323-327`) can interleave SNode construction is an IR-layer
question for agent 01.

**E12 — `Ndarray::nelement_` overflows in `int`** at `ndarray.cpp:37` and `:77`
while the same idiom at `:53` and `:96` uses `1LL`. *Revision 3: the `int`
literal is on 37 and 77; 38 and 78 are the `std::multiplies<>()` line. The spans
in section 8.1B, `35-38` and `75-78`, were already right.* Section 8.1B. In my
territory and a 6.2 item, but fixing it is not investigation.

**E13 — `StructMeta::max_num_elements` is `i64` and all accessors truncate it
to `i32`.** Section 8.1A. A partial 64-bit conversion someone started, relevant
to how much of 6.2 is already done.

**E14 — The `NodeManager` deactivate path reaches a chunk scan, and its bound is
arch-dependent.** Section 8.3. `Pointer_deactivate` → `recycle` → `locate` →
`ptr2index`. *Revision 2 stated this as "bounded by chunks touched rather than
by live data", unqualified. That was wrong and is corrected.* On a found pointer
it is O(index of the containing chunk) on every arch. On a not-found pointer it
kills the thread under `ARCH_cuda` and `ARCH_amdgpu`, and on the CPU build runs
all 131072 iterations, because `taichi_assert_format` (`runtime.cpp:766-815`)
emits its kill only at `:800` and `:802`. Against plan section 6.4 this is a
throughput fact, and against plan section 2 the unbounded case is the
first-class target. Recorded, not judged.

**E15 — The 1 GiB pool's denominator is shared, so the capacity table is an
upper bound on part of the target set.** Sections 3.3 and 3.4. Where
`use_device_memory_pool()` is false — always on AMDGPU, and on CUDA below driver
11.2 or where the device lacks
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` — SNode tree root buffers, ndarrays
and argpacks are cut from the same bump allocator as the `ListManager`s. How
much headroom a sizing rule for item 6.1 should reserve for them is
workload-dependent and is a design decision. I propose no figure and call
nothing surplus.
