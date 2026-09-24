# Report 03B — Runtime and struct layer

Agent 03B. Territory: `taichi/runtime/`, `taichi/struct/`, `taichi/program/`.
Focus: the 1024 SNode ceiling and runtime addressing.

Contemporaneous notes: `modernization/investigation/notes-03b-runtime-struct.md`.

**Revision 3**, an amendment on top of revision 2. Revision 2's changes are in
section 6. The round-two adversarial claims I adjudicated in this amendment,
including the two I upheld against my own **[V]** markings, are in section 6.1.

Everything below is marked **[V]** verified by reading the code (or by
compiling it, see section 2.1) or **[I]** inferred by reasoning from verified
facts. Nothing is unmarked.

---

## 1. Where section 6 touches my territory

### 1.1 Item 6.1 — parameterise the SNode ceiling

`taichi_max_num_snodes` has exactly **five** use sites in the whole tree.
Three are mine. **[V]** (`grep -rn` over `taichi/`, build excluded)

| Site | What it does |
|---|---|
| `taichi/inc/constants.h:12` | the definition |
| `taichi/codegen/llvm/struct_llvm.cpp:266` | `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` — outside my territory, agent 02's |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:567` | `ListManager *element_lists[taichi_max_num_snodes];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:568` | `NodeManager *node_allocators[taichi_max_num_snodes];` |
| `taichi/runtime/llvm/runtime_module/runtime.cpp:569` | `Ptr ambient_elements[taichi_max_num_snodes];` |

A second constant the brief does not name scales the same way and also sizes
runtime arrays: `kMaxNumSnodeTreesLlvm = 512` at `taichi/inc/constants.h:13`,
used only at `runtime.cpp:562-563` for `roots[]` and `root_mem_sizes[]`. **[V]**
It has **no assertion anywhere in the tree**. **[V]**

Everything that must change for 6.1:

1. **`taichi/inc/constants.h:12`** — the value itself. The header is a plain
   `#pragma once` header with no `#ifndef` guards on any value and no
   `#cmakedefine`; there is no existing build-time override mechanism to hang a
   parameter on. **[V]**
2. **`taichi/runtime/llvm/runtime_module/CMakeLists.txt:3-15`** — `runtime.cpp`
   is compiled to bitcode by a standalone `clang` invocation under
   `add_custom_target`, receiving only `-D ARCH_<arch>` and
   `-I ${PROJECT_SOURCE_DIR}`. It inherits none of the project's
   `target_compile_definitions`. **[V]** Any `-D` mechanism must be added to
   this command explicitly; a generated header on the include path would work
   without touching it. **[I]**
3. **The host/bitcode agreement.** `LLVMRuntime` is only forward-declared on
   the host (`taichi/program/context.h:11`, `taichi/rhi/llvm/llvm_device.h:8`).
   **[V]** The host never takes `sizeof(LLVMRuntime)` itself: it asks the
   bitcode through `runtime_get_memory_requirements`
   (`llvm_runtime_executor.cpp:676-679` calling `runtime.cpp:891-906`) **[V]**
   and reads fields only through the generated `LLVMRuntime_get_*` accessors
   (`runtime.cpp:616-626`, `:749-751`). **[V]** Codegen fetches the type by
   name at `codegen/llvm/codegen_llvm.cpp:2697` `get_runtime_type("LLVMRuntime")`.
   **[V]** So the layout lives entirely in the `.bc`, and a value change is
   layout-safe provided the `.bc` is rebuilt with the same value the host C++
   was built with. **[I]**
4. **Per-arch bitcode.** One `runtime_<arch>.bc` per arch
   (`runtime_module/CMakeLists.txt:29-31`), loaded by name at
   `llvm_context.cpp:209-211` and read from `runtime_lib_dir()` at
   `llvm_context.cpp:362`. **[V]** An install-time parameter means all of these
   are regenerated per install. **[I]**
5. **The `.bc` output goes into the source directory**
   (`WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"`,
   `runtime_module/CMakeLists.txt:10`, with upstream's own TODO at line 9), and
   is installed from there (line 13). **[V]** An install-time parameter that
   varies per machine writes into the source tree as things stand. **[I]**
6. **A latent variable fault in that same install rule.** Line 8 builds
   `runtime_${rtm_arch}.bc` from the function parameter declared at line 3;
   line 13 installs `runtime_${arch}.bc`, and `arch` is not a parameter but the
   caller's `foreach` variable at line 29, visible inside the function through
   CMake's parent-scope read. **[V]** The two hold the same value at call time,
   so it works today by coincidence. **[I]** Anyone parameterising this function
   for 6.1 or 6.3 will trip on it. Escalation E13.
7. **The assertion at `struct_llvm.cpp:266` does not guard what it appears to
   guard.** See section 3.1. It is agent 02's file but the consequence lands on
   my three arrays.

### 1.2 Item 6.2 — 64 bit addressing

Full inventory in section 4. The runtime-side summary: SNode identity is `i32`
end to end (`StructMeta::snode_id`, `runtime.cpp:308`), physical coordinates
are `i32` per axis (`PhysicalCoordinates::val`, `runtime.cpp:289`), every
loop-index the runtime hands a generated kernel is `int`
(`RangeForTaskFunc`, `runtime.cpp:43`; `BlockTask`, `runtime.cpp:1385`), and
both list containers count elements in `i32` with an upstream TODO already on
the record at `runtime.cpp:424-425`. **[V]**

The memory allocation path is the one part that is already 64-bit clean:
`LLVMRuntime::allocate_aligned` and `allocate_from_reserved_memory`
(`runtime.cpp:823-874`) are `std::size_t` throughout. **[V]**

### 1.3 Item 6.3 — adaptive module loading

Touches my territory in four places only. Detail belongs to agent 04.

- `taichi/runtime/llvm/runtime_module/CMakeLists.txt:29-31` — the per-arch
  bitcode loop is the existing module-per-arch mechanism. **[V]**
- `taichi/runtime/llvm/llvm_context.cpp:209-211`, `:362` — module selection by
  arch name at load. **[V]**
- `taichi/runtime/llvm/llvm_runtime_executor.cpp:36-72` — arch selection is
  compile-time `#if defined(TI_WITH_CUDA)` / `TI_WITH_AMDGPU` plus runtime
  detection with silent fallback to `host_arch()`. **[V]**
- `taichi/runtime/program_impls/llvm/llvm_program.cpp:130-149`
  `make_kernel_launcher` is the same compile-time-plus-arch-switch shape. **[V]**

A note, downgraded from the flag my earlier report raised.
`cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` is a **checked-in prebuilt**
artifact pinned to sm_60, which is Pascal, against a baseline-tier GTX 750 that
is sm_50. **[V]** I raised that as a concern. **It cannot fire.** The link is
double-gated: **[V]**

- `llvm_context.cpp:504-506` guards the call on
  `CUDAContext::get_instance().get_compute_capability() >= 60`, so on sm_50 it
  is not called at all.
- `link_module_with_custom_cuda_library` (`llvm_context.cpp:574-591`) wraps its
  whole body in `if (!cuda_library_path.empty())`, and
  `get_custom_cuda_library_path` (`taichi/util/lang_util.cpp:18-28`) returns
  `""` unless the file is present in `runtime_lib_dir()`. Its only `install()`
  rule is `runtime_module/CMakeLists.txt:24`, inside
  `COMPILE_CUSTOM_CUDA_LIBRARY`, whose sole call site is commented out at
  `:36-42`, so the file never reaches the install tree.

Recorded as a note for agent 04, not as a concern.

---

## 2. The exact memory footprint implied by 1024

### 2.1 What I measured, and how

I compiled the real `runtime.cpp` with appended `static_assert`s, piping it
through stdin so that no file was created:

```
(cat taichi/runtime/llvm/runtime_module/runtime.cpp; printf '...asserts...') \
  | clang++ -x c++ - -std=c++17 -fno-exceptions \
      -I /opt/project/taichi \
      -I /opt/project/taichi/taichi/runtime/llvm/runtime_module \
      -fsyntax-only -w
```

It compiled clean, so all of these are **[V]** measured, not hand-computed:

| Quantity | Bytes |
|---|---|
| `sizeof(LLVMRuntime)` | 35256 |
| `offsetof(LLVMRuntime, element_lists)` | 8296 |
| `offsetof(LLVMRuntime, node_allocators)` | 16488 |
| `offsetof(LLVMRuntime, ambient_elements)` | 24680 |
| `sizeof(ListManager)` | 1048616 |
| `sizeof(NodeManager)` | 56 |
| `sizeof(Element)` | 64 |
| `sizeof(PhysicalCoordinates)` | 48 |
| `sizeof(StructMeta)` | 72 |
| `sizeof(RandState)` | 20 |

### 2.2 The three arrays

Pointers are 8 bytes on x86-64 and on NVPTX64.

```
element_lists     1024 x 8 =  8192 bytes
node_allocators   1024 x 8 =  8192 bytes
ambient_elements  1024 x 8 =  8192 bytes
                            -------------
                              24576 bytes = 24.0 KiB
```

The two SNode-tree arrays alongside them:

```
roots             512 x 8  =  4096 bytes
root_mem_sizes    512 x 8  =  4096 bytes
                            -------------
                               8192 bytes =  8.0 KiB
```

Against the measured `sizeof(LLVMRuntime) = 35256`:

```
24576 / 35256 = 69.71 %   the three SNode arrays
 8192 / 35256 = 23.24 %   the two SNode-tree arrays
                --------
                92.94 %   of the runtime struct is fixed-size id-indexed tables
```

Everything else in the struct is 2488 bytes. **[V]** (35256 - 24576 - 8192)

Closed form, with N = `taichi_max_num_snodes` and T = `kMaxNumSnodeTreesLlvm`:

```
sizeof(LLVMRuntime) = 2488 + 24*N + 16*T
check: 2488 + 24*1024 + 16*512 = 2488 + 24576 + 8192 = 35256   [matches V]
```

### 2.3 What is actually committed

On CUDA/AMDGPU the runtime object sits at the head of the device preallocated
buffer, rounded to `taichi_page_size` (4096) at `runtime.cpp:929-930`. **[V]**

```
ceil(35256 / 4096) = ceil(8.607) = 9 pages
9 x 4096 = 36864 bytes = 36.0 KiB committed in device memory
```

Scaling, using the closed form: **[I]** (arithmetic from the verified form)

| N | T | sizeof | pages | committed | delta vs today |
|---|---|---|---|---|---|
| 1024 | 512 | 35256 | 9 | 36864 | — |
| 4096 | 512 | 108984 | 27 | 110592 | +73728 |
| 8192 | 1024 | 215480 | 53 | 217088 | +180224 |
| 16384 | 2048 | 428472 | 105 | 430080 | +393216 |
| 65536 | 8192 | 1706424 | 417 | 1708032 | +1671168 |
| 1048576 | 65536 | 26216888 | 6401 | 26218496 | +26181632 |

**The tables are cheap.** Going from 1024 SNodes to 65536 costs 1.59 MiB more
device memory. On a 2 GB GTX 750 that is 0.08 % of the card.

### 2.4 What the constant actually gates: a ListManager per SNode

`taichi_max_num_snodes` is a bounds constant. What it bounds is not a 24 KiB
table but a `ListManager` per SNode, and `sizeof(ListManager)` is **1048616
bytes**, just over 1 MiB. **[V]** It is dominated by
`Ptr chunks[128 * 1024]` at `runtime.cpp:427-428` = 1048576 bytes. **[V]**

Allocation, per SNode tree that is not all-dense
(`runtime.cpp:1000-1016`): **[V]**

```
if (all_dense) {
  return;
}
for (int i = root_id; i < root_id + num_snodes; i++) {
  // TODO: some SNodes do not actually need an element list.
  runtime->element_lists[i] =
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

- `num_snodes` is `(int)snode_metas.size()`
  (`llvm_runtime_executor.cpp:444`), i.e. **every** SNode in the tree, `place`
  nodes included. Upstream's own TODO at `runtime.cpp:1004` says so. **[V]**
- `all_dense` is a **conjunction**, and the config half of it matters.
  `llvm_runtime_executor.cpp:402` seeds it with
  `config_.demote_dense_struct_fors` and the loop at `:403-410` can only clear
  it. So `all_dense == demote_dense_struct_fors AND (every SNode is dense,
  place or root)`. **[V]** The default is `demote_dense_struct_fors = true`
  (`taichi/program/compile_config.cpp:18`), forced true again for SPIR-V archs
  in `CompileConfig::fit` (`compile_config.cpp:72-73`). **[V]**
  **Correction, revision 3.** Revision 2 said this flag was "settable only from
  Python (`taichi/python/export_lang.cpp:201-202`)" and "effectively a
  hard-coded `true` at C++ level", and marked that **[V]**. It is false. It is a
  public field of a plain struct (`taichi/program/compile_config.h:8`, the field
  at `:28`); `default_compile_config` is `extern TI_DLL_EXPORT CompileConfig` at
  `compile_config.h:110`, defined at `taichi/util/lang_util.cpp:14`; and
  `Program::Program` copies that global wholesale at
  `taichi/program/program.cpp:75`. A C++ driver sets it before constructing a
  `Program` with no new mechanism.
  **Correction, revision 4.** Revision 3 added to this sentence that
  `config.fit()` at `program.cpp:77` "does not touch this field". That is false
  for *this* field: `fit()` sets `demote_dense_struct_fors = true` for SPIR-V
  archs at `compile_config.cpp:72-73`, which this same bullet states correctly
  two sentences earlier and E5c states again. The accurate statement is that on
  the SPIR-V archs `fit()` overrides whatever was copied, and on every other
  arch, including all five that `arch_uses_llvm` covers
  (`taichi/rhi/arch.cpp:54-57`), it leaves the copied value alone. The clause is
  true of `device_memory_GB`, where 2.6 uses it, and was wrong here. **[V]** The pybind binding at
  `export_lang.cpp:201-202` is one client of that global, not its owner. **[V]**
  With it false, `all_dense` is false unconditionally and **every** tree pays a
  `ListManager` per SNode, dense trees included. **[I]** Given brief 4.1
  "Sparsity is required", the sparse branch is the operative one either way.
  **[I]**
- `all_dense` also gates whether the allocator pool exists at all, not merely
  how much it holds. `preallocate_runtime_memory` is called from
  `llvm_runtime_executor.cpp:413` under the guard at `:412`
  (`config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense`), and
  otherwise from `:721` under `!use_device_memory_pool()`. **[V]** See 2.6.
- `LLVMRuntime::create<T>` (`runtime.cpp:606-612`) calls
  `allocate_aligned(runtime_memory_chunk, sizeof(T), 4096, request=true)`, so
  the full 1048616 bytes is committed **eagerly** at materialisation. **[V]**
  The chunks inside it are lazy, but "lazy" is not "free" and it is not
  deferred for long: one element-list chunk is **4 MiB** and the root's is
  taken during materialisation. Priced in 2.5. **[V]**

Additionally, for every `pointer` or `dynamic` SNode
(`is_gc_able`, `taichi/ir/snode_types.cpp:21-23`, called at
`llvm_runtime_executor.cpp:447`) **[V]**:

- one `NodeManager` (56 bytes) whose constructor creates **three more**
  `ListManager`s — `free_list`, `recycled_list`, `data_list`
  (`runtime.cpp:658-663`) **[V]**
- one ambient element of the SNode's cell size
  (`runtime.cpp:1033-1040`). Note this one requests alignment **128**, not
  4096 (`runtime.cpp:1038-1039`), so its allocator charge is round-up to 128,
  not to a page. **[V]**

Raw object sizes, before the allocator's charge: **[V]**

```
plain sparse SNode (root, dense, place, bitmasked):
    1 element list                              = 1,048,616 bytes

pointer or dynamic SNode:
    1 element list                              = 1,048,616
  + 1 NodeManager                               =        56
  + 3 ListManagers inside it                    = 3,145,848
                                                  ---------
                                                  4,194,520 bytes
                                                  (plus the ambient element)
```

Those are the object sizes. They are not what the allocator spends. See 2.5.

### 2.5 The allocator's alignment charge, and the chunks

**The charge.** `allocate_from_reserved_memory` bills the caller for the
alignment padding before the fit test and before the head advance,
`runtime.cpp:848-853`: **[V]**

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

With A the alignment and r = head mod A: for r = 0, `(h+A-1) mod A = A-1` and
the padding is 0; for r > 0, `(h+A-1) mod A = r-1` and the padding is `A-r`.
So it is ordinary round-up, charged to the caller. **[I]**

Consequence: from an aligned head, each 4096-aligned request advances the head
by `roundup(S, 4096)`. `1048616 mod 4096 = 40`, so a `ListManager` costs
**1,052,672 bytes, 257 pages**, not 1,048,616. My earlier figures were about
0.4 % low throughout. **[I]**

**The chunks.** `ListManager::touch_chunk` (`runtime.cpp:1664-1679`) allocates
`max_num_elements_per_chunk * element_size` at alignment 4096. **[V]**

- **Element lists.** Constructed with `sizeof(Element)` and `1024 * 64`
  (`runtime.cpp:1006`), and `sizeof(Element) == 64` (measured, 2.1), so one
  chunk is `65536 * 64` = **4,194,304 bytes, exactly 1024 pages**. **[V]**
- The root's element list takes chunk 0 during materialisation, at
  `runtime.cpp:1016` (`append` -> `allocate` -> `reserve_new_element` ->
  `touch_chunk`). Every other SNode's list takes its chunk on the first listgen
  append (`runtime.cpp:1282-1329`, `:1331-1383`). **[V]**
- `clear_list` (`runtime.cpp:1270-1273`) only zeroes `num_elements`. Chunks are
  never returned, so the touched set is monotonic for the life of the process.
  4 MiB per visited SNode is a floor, not a peak. **[V]**
- **`NodeManager`'s three lists.** The ctor's `128 * 1024` default
  (`runtime.cpp:649`) is **overridden at the only call site**:
  `runtime.cpp:1030` passes `1024 * 16`, so `chunk_num_elements` is 16384 and
  the 128 MB halving loop at `:652-655` engages only when `node_size > 8192`.
  **[V]** `free_list` and `recycled_list` hold `i32`, so one chunk each is
  `16384 * 4` = 65,536 bytes. `data_list` holds `node_size`, so one chunk is
  `16384 * node_size` — a 1 KiB cell gives a single **16 MiB** allocation, a
  4 KiB cell gives 64 MiB. `node_size` is set at
  `llvm_runtime_executor.cpp:448-457`. **[V]**

Charges, from an aligned head: **[I]**

| Allocation | Size | Charge |
|---|---|---|
| `ListManager` header | 1,048,616 | 1,052,672 |
| `NodeManager` | 56 | 4,096 |
| element-list first chunk | 4,194,304 | 4,194,304 |
| `free_list` / `recycled_list` first chunk | 65,536 | 65,536 |

Per-SNode totals: **[I]**

```
plain sparse SNode, header only            = 1,052,672
plain sparse SNode, header + first chunk   = 5,246,976   (~5.0 MiB)

pointer/dynamic SNode, headers only:
    element list header       1,052,672
  + NodeManager                   4,096
  + 3 ListManager headers     3,158,016
                              ---------
                              4,214,784

pointer/dynamic, headers + element chunk + the two i32 chunks
  (excluding the data_list chunk, which is workload-sized)
                            = 8,540,160
```

**A populated sparse SNode is 5.0 MiB, not 1.0 MiB.** My earlier report gave
the header alone and built its tier claim on it.

### 2.6 The pool. This, not the card, is what binds.

`ListManager`s are not cut from free device memory. They are bump allocated out
of a fixed preallocated chunk whose size is a configuration constant.

`LlvmRuntimeExecutor::preallocate_runtime_memory`,
`taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632`: **[V]**

```
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
```

and the defaults, `taichi/program/compile_config.cpp:63-64`: **[V]**

```
63:  device_memory_GB = 1;  // by default, preallocate 1 GB GPU memory
64:  device_memory_fraction = 0.0;
```

That buffer becomes `runtime_memory_chunk` via `runtime_initialize_memory`
(`llvm_runtime_executor.cpp:629-631` calling `runtime.cpp:962-971`). **[V]**
Every `ListManager`, every `NodeManager`, every ambient element and every
touched chunk is cut from it.

So the default pool is **1 GiB on every CUDA card**, baseline or Ampere. It
does not scale with the card. `get_total_memory` is
`CUDAContext::get_total_memory` (`taichi/rhi/cuda/cuda_context.cpp:88-92`),
returning the device total from `mem_get_info`, and `TI_ASSERT` at `:620` caps
a fraction-based pool at that total.

**Correction, revision 3. `device_memory_GB` is reachable from C++.** Revision 2
said `device_memory_GB` and `device_memory_fraction` were "reachable only from
Python (`taichi/python/export_lang.cpp:208-210`)" and that "at C++ level the
pool is a hard-coded 1 GiB", and marked that **[V]**. It is false, on the same
facts as the `demote_dense_struct_fors` correction in 2.4. Both are public
`float64` fields of a plain struct (`taichi/program/compile_config.h:8`, the
fields at `:71-72`), on a global declared
`extern TI_DLL_EXPORT CompileConfig default_compile_config;`
(`compile_config.h:110`) and defined at `taichi/util/lang_util.cpp:14`, which
`Program::Program` copies wholesale at `taichi/program/program.cpp:75`.
`config.fit()` at `:77` does not touch either field
(`compile_config.cpp:67-76`). So writing
`taichi::lang::default_compile_config.device_memory_GB` before constructing a
`Program` sets the pool size from C++ with no new mechanism and no Python. The
binding at `export_lang.cpp:208-210` is one client of that global; a second,
`export_lang.cpp:264-267`, hands Python a reference to the same object. Both
fields are `float64`, so fractional values are legal. **[V]**

**Scope of the pool: CUDA and AMDGPU only.** `preallocate_runtime_memory` has
exactly two call sites in the tree, and both are arch-gated: **[V]**

- `llvm_runtime_executor.cpp:413`, inside `initialize_llvm_runtime_snodes`,
  under `config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense`
  at `:412`.
- `llvm_runtime_executor.cpp:721`, inside `materialize_runtime`, under
  `config_.arch == Arch::cuda || config_.arch == Arch::amdgpu` at `:719` and
  `!use_device_memory_pool()` at `:720`.

`use_device_memory_pool_` is assigned only on the CUDA branch of the executor
constructor (`llvm_runtime_executor.cpp:39-50`, the assignment at `:49`), from
`CUDAContext::supports_mem_pool()` (`taichi/rhi/cuda/cuda_context.h:77-79`,
backing field at `:31`), which `cuda_context.cpp:52-53` sets true only when the
test computed at `:35-50` passes: driver 11.2 or newer *and* the device
reporting `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`. **[V]** So on AMDGPU
`use_device_memory_pool()` is always false and the `:721` site always fires.

**The exception set is CPU *and DX12*, not CPU alone. Added in revision 4.**
`arch_uses_llvm` (`taichi/rhi/arch.cpp:54-57`) carries five archs — `x64`,
`arm64`, `cuda`, `dx12`, `amdgpu` — and the two pool guards above admit only
`cuda` and `amdgpu`. **[V]** DX12 is not a CPU arch, yet it takes neither
guard; `LlvmRuntimeExecutor`'s constructor hands it a `cpu::CpuDevice` at
`llvm_runtime_executor.cpp:158-164`, with upstream's own `FIXME: add dx12
device` at `:160`. **[V]** `DX12_ARCH` is in the bitcode arch loop at
`runtime_module/CMakeLists.txt:29`, so a `runtime_dx12.bc` is built with its own
baked constant. **[V]** So the conclusion below holds on DX12 for the same
reason it holds on CPU. Revision 3 wrote this exception as "the CPU path" alone
and that enumeration was short by one arch; the positive statement, that the
pool is a CUDA and AMDGPU fact, was and is exact. DX12 is agent 04's backend
territory and I am recording the arithmetic consequence only. **[I]**

On those paths neither guard passes, `runtime_memory_chunk.preallocated_size`
stays 0, and `LLVMRuntime::allocate_aligned` takes the `host_allocator` branch
at `runtime.cpp:830-834` instead. **[V]** That branch reaches
`host_allocate_aligned` (`llvm_runtime_executor.cpp:28-32`) and
`UnifiedAllocator::allocate`, which mmaps a fresh chunk sized
`std::max(size, default_allocator_size)` whenever the current chunk cannot
serve the request (`taichi/rhi/common/unified_allocator.cpp:61-85`, the max at
`:64-69`). **[V]** There is no pool and no ceiling there.

**Consequence, and it is the qualifier E5 needs.** Where there is no pool
nothing bounds the `ListManager`s except host memory and the
`element_lists[1024]` array itself (`runtime.cpp:567`), whose write at
`runtime.cpp:1005` is unchecked. So on CPU, and equally on DX12,
`taichi_max_num_snodes` is the binding limit and raising it alone *is*
effective, at the ~5.0 MiB per populated sparse SNode computed in 2.5. That is
the opposite of the CUDA and AMDGPU conclusion, and brief section 2 makes
CPU-only a first-class target. **[I]**

**I withdraw the claim in my earlier report that the card is the binding
limit.** It is not. The pool is, and the pool is the same size on all three
target tiers.

### 2.7 Capacity against the 1 GiB default pool

Pool = 1 x 2^30 = 1,073,741,824 bytes. Cumulative cost of k identical
allocations from an aligned base is `S + (k-1)*roundup(S, 4096)`, since the
last one is not charged its trailing slack until the next request. **[I]**

**(a) element-list headers only** — `cum(k) = 1,048,616 + (k-1) x 1,052,672`

```
k = 1020 : 1019 x 1,052,672 = 1,072,672,768 ; + 1,048,616 = 1,073,721,384  fits (slack 20,440)
k = 1021 : 1020 x 1,052,672 = 1,073,725,440 ; + 1,048,616 = 1,074,774,056  does not fit
```

**Capacity 1020.** The 20,440-byte slack means the answer is 1020 whether or
not the pool base happens to be page-aligned. **[I]**

**The current constant already overruns the current default pool.** 1024
element lists cost `1023 x 1,052,672 + 1,048,616 = 1,077,932,072`, which is
**4,190,248 bytes over** the 1,073,741,824-byte pool — about four lists' worth.
**[I]**

**(b) other cases** **[I]**

| Case | Bytes per SNode | Fit in 1 GiB pool |
|---|---|---|
| element-list header only | 1,052,672 | 1020 |
| plain sparse SNode, header + first chunk | 5,246,976 | 204 |
| pointer SNode, headers only | 4,214,784 | 254 |
| pointer SNode, headers + element chunk + 2 i32 chunks | 8,540,160 | 125 |

Worked: `204 x 5,246,976 = 1,070,383,104` fits, `205 x` = 1,075,630,080 does
not. `254 x 4,214,784 = 1,070,555,136` fits, `255 x` = 1,074,769,920 does not.
`125 x 8,540,160 = 1,067,520,000` fits, `126 x` = 1,076,060,160 does not.

**(c) the baseline card with the pool raised.** At
`device_memory_fraction = 0.9` on a nominal 2 GiB GTX 750 the pool is
`0.9 x 2,147,483,648 = 1,932,735,283`: 1836 element-list headers, 368 populated
plain sparse SNodes, or 458 pointer SNodes. **[I]**

**Correction, revision 3. The root buffers are not always outside the pool.**
Revision 2 said that pool "leaves roughly 200 MB of the card for the root
buffers, the runtime objects buffer and the CUDA context, all of which are
separate allocations outside it". The runtime-objects buffer and the CUDA
context are outside it; the root buffers are not, whenever
`use_device_memory_pool()` is false. The chain, every hop read: **[V]**

1. `initialize_llvm_runtime_snodes` allocates the tree's root buffer at
   `llvm_runtime_executor.cpp:419-420` through
   `snode_tree_buffer_manager_->allocate`.
2. `SNodeTreeBufferManager::allocate`
   (`taichi/runtime/llvm/snode_tree_buffer_manager.cpp:12-18`) calls
   `runtime_exec_->allocate_memory_on_device` at `:15`.
3. `LlvmRuntimeExecutor::allocate_memory_on_device`
   (`llvm_runtime_executor.cpp:476-491`) calls
   `llvm_device()->allocate_memory_runtime`, passing `use_device_memory_pool()`
   as the `use_memory_pool` field at `:485`.
4. `CudaDevice::allocate_memory_runtime` (`taichi/rhi/cuda/cuda_device.cpp:50-78`)
   branches at `:56`. With `use_memory_pool` **true** it takes `malloc_async` at
   `:57-58`, genuinely outside the pool. With it **false** it takes
   `DeviceMemoryPool::get_instance().allocate_with_cache` at `:60-61`.
5. `DeviceMemoryPool::allocate_with_cache`
   (`taichi/rhi/llvm/device_memory_pool.cpp:27-33`) forwards to
   `CachingAllocator::allocate` (`taichi/rhi/llvm/allocator.cpp:33-58`), which
   on a cache miss calls `device->allocate_llvm_runtime_memory_jit` at `:54-55`.
6. `CudaDevice::allocate_llvm_runtime_memory_jit` (`cuda_device.cpp:80-90`)
   JIT-calls `runtime_memory_allocate_aligned` at `:82-84`.
7. `runtime_memory_allocate_aligned` (`runtime.cpp:879-886`) is
   `runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)`
   at `:883-885`. **The same bump allocator the `ListManager`s come from.**

`AmdgpuDevice::allocate_memory_runtime` (`taichi/rhi/amdgpu/amdgpu_device.cpp:55-78`)
has no `use_memory_pool` branch at all and always calls `allocate_with_cache`
at `:63`, reaching the same JIT hop at `:80-90`. **[V]** Since
`use_device_memory_pool_` is set only under CUDA
(`llvm_runtime_executor.cpp:49`), on AMDGPU the root buffers always come out of
the pool, and on CUDA they do whenever the driver is below 11.2 or the device
does not report `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`
(`taichi/rhi/cuda/cuda_context.cpp:35-53`). **[I]**

The same route carries `Ndarray` storage (`taichi/program/ndarray.cpp:61` via
`Program::allocate_memory_on_device`, `taichi/program/program.h:245-248`) and
argpack buffers (`taichi/program/argpack.cpp:17-18`). **[V]**

**A fourth consumer class, added in revision 4: per-launch staging buffers.**
Revision 3 named three classes entering the pool at hop 3. There is a fourth,
and it is the one class whose size follows launch traffic rather than SNode
count. For every external-array argument whose host pointer is not already
device memory, the kernel launcher allocates a device staging buffer for the
launch and copies into it: **[V]**

- `taichi/runtime/cuda/kernel_launcher.cpp:83-84`, under the `else` at `:82` of
  `if (on_cuda_device(data_ptr))` at `:78`, and again at `:92-94` for the
  gradient pointer under `if (grad_ptr != nullptr)` at `:91`.
- `taichi/runtime/amdgpu/kernel_launcher.cpp:59-60`, under the `else` at `:58`
  of `if (on_amdgpu_device(data_ptr))` at `:56`.

Both call `executor->allocate_memory_on_device` on the
`LlvmRuntimeExecutor` fetched at `cuda/kernel_launcher.cpp:19` and
`amdgpu/kernel_launcher.cpp:19`, which is hop 3 of the chain above, so they
reach the same bump allocator by the same remaining hops whenever
`use_device_memory_pool()` is false. **[V]** On AMDGPU that is unconditional in
the sense that matters here: `AmdgpuDevice::allocate_memory_runtime`
(`amdgpu_device.cpp:55-78`) has no branch on the flag at all, so every staging
buffer it allocates comes out of the pool. **[V]**

They are released after the launch, `cuda/kernel_launcher.cpp:184` and
`amdgpu/kernel_launcher.cpp:147`, through
`LlvmRuntimeExecutor::deallocate_memory_on_device`
(`llvm_runtime_executor.cpp:493-498`). **[V]** On the non-mem-pool path that
reaches `CudaDevice::dealloc_memory`'s `info.use_cached` branch
(`cuda_device.cpp:109-111`, with `info.use_cached = true` assigned
unconditionally at `:68`), which returns the block to `CachingAllocator`'s free
list (`device_memory_pool.cpp:43-51` -> `allocator.cpp:60-68`). The block is
reusable by later runtime-memory allocations; the bump head at
`runtime.cpp:853` never moves back. **[V]** So staging buffers are transient in
occupancy but permanent in the pool's high-water mark. **[I]** I am recording
the mechanism and estimating nothing; per standing instruction 3 I call none of
it surplus.

Both launcher files sit inside my declared territory, `taichi/runtime/`. I did
not find this class in revision 2 or 3, and neither did any adversary until
round three: my entry into the pool was through the SNode tree, and I traced
the consumers I had already named rather than enumerating every caller of
`allocate_memory_on_device`. The enumeration I should have run is the one
`adversary3-03-2.md` ran, and it is now in E15.

The CPU path does not: `CpuDevice::allocate_memory_runtime`
(`taichi/rhi/cpu/cpu_device.cpp:44-51`) calls `allocate_memory` directly and
never reaches `allocate_with_cache`, and there is no pool there anyway per 2.6.
**[V]**

**What this does to the capacity rows.** Every row in (a), (b) and (c) assumes
the whole pool is available to `ListManager`s. On the non-mem-pool path it is
not: root buffers, sized `iroundup(root_size, taichi_page_size)`
(`llvm_runtime_executor.cpp:417`), and every ndarray and argpack, come out of
the same denominator. Those rows are therefore **upper bounds** on AMDGPU and
on any CUDA device that does not report memory-pool support, and exact only on
the CUDA mem-pool path. How much they overstate is workload-dependent and I am
not estimating it. **[I]** I am not asserting which of the three tiers in brief
5.2 falls on which branch: the branch is a driver-version and device-attribute
test at runtime (`cuda_context.cpp:36-50`), and nothing in this tree records
the answer for the GTX 750, the 1070 or the 3060. Escalation E15.

The runtime-objects buffer is genuinely separate: it is its own
`preallocate_memory` call at `llvm_runtime_executor.cpp:687-690`, reaching
`llvm_device()->allocate_memory` at `:594-596`, and it feeds
`runtime_objects_chunk` rather than `runtime_memory_chunk`. **[V]**

### 2.8 The failure mode is a grid abort

`allocate_from_reserved_memory` is a pure bump with no free path. On failure,
`runtime.cpp:859-872`: under `ARCH_cuda` it calls `__assertfail`
(`:864-869`) with the text "Out of CUDA pre-allocated memory.\nConsider using
ti.init(device_memory_fraction=0.9) or ti.init(device_memory_GB=4)", then falls
to `taichi_assert_runtime` at `:872`. The comment at `:861-863` says a
`taichi_assert_runtime` alone would not halt the grid fast enough. **[V]**

Exceeding the pool is therefore a hard kernel abort at materialisation or at
first listgen, not degraded performance. **[I]**

### 2.9 Corrected consequence for 6.1

**The tables are nearly free.** Going from N=1024 to N=65536 costs 1.59 MiB of
the separate runtime-objects allocation. That half of my earlier conclusion
stands unchanged. **[I]**

**Raising `taichi_max_num_snodes` alone is inert on every tier, not only the
baseline one.** The constant and the default pool are already matched to within
0.4 %: the ceiling is 1024 and the pool holds 1020. Because the pool is a fixed
1 GiB regardless of card, the 1070 and the 3060 hit exactly the same wall as
the 750. My earlier framing — that the card binds at the baseline tier and the
constant binds at the mid tier — was wrong in both halves. **[I]**

**Ratio, restated.** The three pointer tables cost 24 KiB at N=1024. A
populated all-pointer configuration at that N would cost
`1024 x 8,540,160 = 8,745,123,840` bytes, except that it cannot be reached
because the pool holds 125 such SNodes. The tables are not the footprint by
roughly five orders of magnitude, and the footprint is not the ceiling either;
the pool is. **[I]**

Any replacement for 1024 is therefore a **pair** at minimum,
`(taichi_max_num_snodes, device_memory_GB)`, and arguably a set of four with
`ListManager::max_num_chunks` (`runtime.cpp:427`) and the element-list chunk
count `1024 * 64` (`runtime.cpp:1006`). Moving one without the others is
arithmetic that does not close. I am not proposing values. Escalations E5 and
E11.

---

## 3. Structural findings on the ceiling

### 3.1 The assertion is per-tree; the array index is global

**[V]** facts:

- `struct_llvm.cpp:266` asserts on `StructCompiler::snodes` (declared
  `struct/struct.h:11`), populated by `StructCompiler::collect_snodes`
  (`struct/struct.cpp:7-13`) from the root passed to
  `StructCompilerLLVM::run` (`struct_llvm.cpp:247-250`).
- A fresh `StructCompilerLLVM` is constructed **per tree**, at
  `taichi/runtime/program_impls/llvm/llvm_program.cpp:45-56`.
- The runtime arrays are indexed by the **global** `SNode::id`:
  `runtime.cpp:1005` (`element_lists[i]`, `i` from `root_id` to
  `root_id + num_snodes`), `runtime.cpp:1029` (`node_allocators[snode_id]`),
  `runtime.cpp:1038` (`ambient_elements[snode_id]`).
- `SNode::id` comes from a **global** `static std::atomic<int> counter`
  (`ir/snode.h:88`, `ir/snode.cpp:12`), `id = counter++` at `ir/snode.cpp:220`.
- The counter is reset in exactly one place, `program/program.cpp:144`
  (`SNode::counter = 0;`), in the `Program` constructor.
  `SNode::reset_counter()` (`ir/snode.h:348-350`) has zero callers.
- SNode **tree** ids, by contrast, are recycled:
  `Program::allocate_snode_tree_id` (`program/program.cpp:559-567`) pops from
  `free_snode_tree_ids_` (`program/program.h:336`), pushed by
  `destroy_snode_tree` (`program/program.cpp:235`).

**[I]** conclusions:

- The assertion bounds the size of one tree. It cannot bound the global id
  used as the array index. With several live trees, or with any create/destroy
  cycling, global ids pass 1024 while each tree stays far under the assert. The
  write is then out of bounds and unchecked.
- `taichi_max_num_snodes` is a ceiling on the **cumulative** number of SNodes
  created over a `Program`'s lifetime, not on the live count. Recomposing the
  "materialised working set" described in brief 4.6 walks that counter upward
  every time.
- `kMaxNumSnodeTreesLlvm` is unchecked entirely. `roots[snode_tree_id]` and
  `root_mem_sizes[snode_tree_id]` at `runtime.cpp:996-997` are written with an
  id nothing bounds. Tree ids are recycled, so this needs 512 simultaneously
  live trees rather than 512 cumulative ones.

### 3.2 Tree destruction leaves the device tables dirty

`LlvmRuntimeExecutor::destroy_snode_tree`
(`llvm_runtime_executor.cpp:758-761`) is two lines: **[V]**

```
get_llvm_context()->delete_snode_tree(snode_tree->id());
snode_tree_buffer_manager_->destroy(snode_tree);
```

It does not null `element_lists[i]`, `node_allocators[i]`,
`ambient_elements[i]`, `roots[tree_id]` or `root_mem_sizes[tree_id]`; it does
not free the `ListManager`s and `NodeManager`s created for those SNodes; and it
does not erase `snode_tree_allocs_[tree_id]` (`llvm_runtime_executor.cpp:440`
assigns it, `:387` reads it, nothing erases it). **[V]**

**There is no mechanism by which a slot could be cleared, not merely no
call.** `STRUCT_FIELD_ARRAY` at `runtime.cpp:616-619` emits
`LLVMRuntime_set_element_lists`, `_set_node_allocators`, `_set_roots` and
`_set_root_mem_sizes`. A grep for all four across the whole tree, build
excluded, over `.cpp`, `.h` and `.py`, returns **nothing outside the macro that
defines them**. **[V]** `ambient_elements` does not even get an accessor pair.
**[V]**

The only writes to any of the five arrays anywhere are `runtime.cpp:996`,
`:997`, `:1005`, `:1029` and `:1038`, all on the create path. **[V]**

There is no free path at all in `allocate_from_reserved_memory`
(`runtime.cpp:838-874`); it is a monotonic bump allocator over the preallocated
chunk. **[V]** One qualification: the pool as a whole is a
`DeviceAllocationGuard` (`llvm_runtime_executor.cpp:587-604`), so it is
released at executor teardown. The leak is unbounded within a `Program`'s life
and reclaimed when the `Program` dies. **[V]**

`snode_tree_allocs_` is worse than a leak. `SNodeTreeBufferManager::destroy`
(`snode_tree_buffer_manager.cpp:20-24`) really does free the underlying device
allocation and erase its own map entry, so the entry left behind in
`snode_tree_allocs_` is a dangling `DeviceAllocation`, and
`get_snode_tree_device_ptr` (`llvm_runtime_executor.cpp:386-389`) will hand it
out on a recycled tree id. **[I]**

**Reachability, which my earlier report asserted without establishing.**
`grep -rn "destroy_snode_tree"` over the tree, build excluded:
`Program::destroy_snode_tree` (`program/program.cpp:214-236`) has **exactly one
caller in the repository**, the pybind lambda at
`taichi/python/export_lang.cpp:569-570`, driven from
`python/taichi/_snode/snode_tree.py:21`. There is no C++ test caller and no
internal caller. (`program/program.cpp:234` calls
`program_impl_->destroy_snode_tree`, a different function declared at
`program_impl.h:55`.) **[V]**

So the honest statement is: **today this is latent, not live.** It is reachable
only through the Python front end, which brief 1.2 puts out of scope, so it
cannot fire from the C++ core as the fork currently stands. It becomes live the
moment this fork adds a core-owned destroy or recompose path. **[I]** Brief 4.6
says the engine SNode configuration is "composed from the larger data stores for
working scope" and is "a materialised working set, not the store itself", which
is a description of recomposition. **This fork is expected to reach it.** **[I]**

**[I]** Combined with 3.1, a create/destroy cycle advances the SNode counter,
leaves stale device pointers behind it, and never reclaims the `ListManager`s —
which at the corrected populated figure is 5.0 MiB per SNode, against a pool
that holds 204 of them.

### 3.3 SNodeTree lifecycle and the contiguous chunk

**[V]**

- `taichi/struct/snode_tree.h:15-44` — the class is an `int id_` and a
  `unique_ptr<SNode> root_`. `check_tree_validity` (`snode_tree.cpp:22-32`)
  only checks that non-place, non-root nodes have children.
- The header comment at `snode_tree.h:13` says the tree "will be backed by a
  contiguous chunk of memory". The class does not own that chunk.
- The chunk is one `DeviceAllocation` per tree, held in
  `SNodeTreeBufferManager::snode_tree_id_to_device_alloc_`
  (`snode_tree_buffer_manager.h:28`, a `std::map<int, DeviceAllocation>`),
  allocated at `snode_tree_buffer_manager.cpp:12-18`, released at `:20-24`.
- It is sized `iroundup(root_size, taichi_page_size)`
  (`llvm_runtime_executor.cpp:417`), where `root_size` is
  `struct_compiler.root_size` (`struct/struct.h:12`, `std::size_t`) set from
  `tlctx_->get_type_size(node_type)` at `struct_llvm.cpp:269`. 64-bit clean.
- Zeroed by backend-specific memset at `llvm_runtime_executor.cpp:421-435`.
- `Program::add_snode_tree` `program/program.cpp:238-255`;
  `Program::destroy_snode_tree` `program/program.cpp:214-236`;
  `Program::get_snode_root(int)` `program/program.cpp:257-259` (unchecked
  `snode_trees_[tree_id]`);
  `Program::get_snode_tree_size` `program/program.cpp:269-271` (returns `int`
  from a `size_t`).

### 3.4 The gfx / SPIR-V side has no 1024 array, and a different id problem

**[V]**

- Root buffers are a `std::vector<std::unique_ptr<DeviceAllocationGuard>>
  root_buffers_` (`taichi/runtime/gfx/runtime.h:152`), grown by
  `GfxRuntime::add_root_buffer` (`gfx/runtime.cpp:730-750`). No fixed-size
  table, so `taichi_max_num_snodes` does not bind on this path.
- `gfx/runtime.h:40` and `:94` declare `std::size_t num_snode_trees{0}`.
- `gfx::SNodeTreeManager::materialize_snode_tree`
  (`gfx/snode_tree_manager.cpp:11-16`) only ever `push_back`s onto
  `compiled_snode_structs_`.
- `gfx::SNodeTreeManager::destroy_snode_tree` (`:18-29`) resets
  `root_buffers_[root_id]` and does **not** erase from
  `compiled_snode_structs_`.
- `get_field_in_tree_offset(int tree_id, ...)` (`:31-47`) indexes
  `compiled_snode_structs_[tree_id]`; `get_snode_tree_device_ptr(int tree_id)`
  (`:49-51`) indexes `root_buffers_[tree_id]`.

**[I]** These vectors are indexed by push-back order while the `tree_id` they
receive is a `Program` id that is recycled (`program/program.cpp:559-567`).

My earlier report escalated whether they actually diverge. **They do, and it is
determinable from four functions, so I am closing it as a finding rather than
leaving it open.** **[I]**

Create trees 0 and 1; destroy tree 1; create a third.

1. `materialize_snode_tree` (`gfx/snode_tree_manager.cpp:11-16`) pushed structs
   at indices 0 and 1 (`:15`) and buffers at 0 and 1 via `add_root_buffer`
   (`:14`, pushing at `gfx/runtime.cpp:747`).
2. `destroy_snode_tree` (`:18-29`) finds `root_id = 1` by linear scan on the
   root pointer and resets `root_buffers_[1]` (`:28`). Neither vector shrinks;
   `compiled_snode_structs_[1]` is untouched.
3. `Program::destroy_snode_tree` pushed id 1 onto `free_snode_tree_ids_`
   (`program/program.cpp:235`); `allocate_snode_tree_id` (`:559-567`) pops it,
   so the third tree receives `tree_id == 1`.
4. Its buffer is pushed at `root_buffers_[2]`. `get_snode_tree_device_ptr(1)`
   (`snode_tree_manager.cpp:49-51`) then calls `->get_ptr()` on the
   `unique_ptr` reset in step 2 and dereferences null.
   `get_field_in_tree_offset(1, ...)` (`:31-47`) reads the destroyed tree's
   descriptors and trips its own `TI_ASSERT_INFO` at `:34-38`.

Divergence after one destroy-then-add cycle. The remedy is a design decision
and stays escalated; the question of whether it diverges is answered.

The LLVM path does not have the analogue: `snode_tree_allocs_` is an
`unordered_map` keyed by tree id (`llvm_runtime_executor.h:152`), so a recycled
id overwrites rather than shifts. **[V]** Its defect is the different one in
3.2.

---

## 4. Complete inventory of 32-bit width assumptions in my territory

All **[V]**, read directly. Grouped by role.

### 4.1 SNode identity, the host/device ABI

| Location | Declaration |
|---|---|
| `runtime.cpp:308` | `StructMeta::snode_id` is `i32` |
| `runtime.cpp:986-993` | `runtime_initialize_snodes(..., const int root_id, const int num_snodes, const int snode_tree_id, ...)` |
| `runtime.cpp:1026-1030` | `runtime_NodeAllocator_initialize(LLVMRuntime *, int snode_id, std::size_t)` |
| `runtime.cpp:1033-1038` | `runtime_allocate_ambient(LLVMRuntime *, int snode_id, std::size_t)` |
| `runtime.cpp:1422-1429` | `parallel_struct_for(RuntimeContext *, int snode_id, int element_size, int element_split, ...)` |
| `runtime.cpp:1691` | `node_gc(LLVMRuntime *, int snode_id)` |
| `runtime.cpp:1721,1738,1782` | `gc_parallel_{0,1,2}(RuntimeContext *, int snode_id)` |
| `runtime.cpp:69-77` | `STRUCT_FIELD_ARRAY(S, F)` generates getters/setters taking `int i` |
| `runtime.cpp:85-88` | `RUNTIME_STRUCT_FIELD_ARRAY(S, F)` likewise `int i` |
| `runtime.cpp:616-619` | those macros applied to `element_lists`, `node_allocators`, `roots`, `root_mem_sizes` |
| `runtime.cpp:749-750` | applied to `node_allocators`, `element_lists` for host query |
| `llvm_runtime_executor.cpp:399-400` | `const int tree_id`, `const int root_id` |
| `llvm_runtime_executor.cpp:442-444` | the JIT call, template list `<void *, std::size_t, int, int, int, std::size_t, Ptr>` |
| `llvm_runtime_executor.cpp:460-466` | `snode_id` crossed as `int` twice |
| `llvm_runtime_executor.h:97` | `DevicePtr get_snode_tree_device_ptr(int tree_id)` |
| `llvm_runtime_executor.h:152` | `std::unordered_map<int, DeviceAllocation> snode_tree_allocs_` |
| `llvm_context.h:57` | `add_struct_module(std::unique_ptr<llvm::Module>, int tree_id)` |
| `llvm_context.h:114` | `get_struct_function(const std::string &, int tree_id)` |
| `llvm_context.cpp:968` | `delete_snode_tree(int id)` |
| `llvm_context.cpp:1036` | `std::unordered_set<int> used_tree_ids` |
| `snode_tree_buffer_manager.h:20-22,28` | `allocate(..., const int snode_tree_id, ...)`, `std::map<int, DeviceAllocation>` |
| `llvm_program.h:52-59` | `cache_field(int snode_tree_id, int root_id, ...)`, `get_cached_field(int snode_tree_id)` |
| `llvm_program.cpp:58-65`, `:67-76`, `:97-122` | `int snode_tree_id`, `int root_id` |
| `program_impl.h:94` | `virtual DevicePtr get_snode_tree_device_ptr(int tree_id)` |
| `program.h:214,232` / `program.cpp:257,559` | `allocate_snode_tree_id`, `get_snode_root(int)`, `get_snode_tree_device_ptr(int)` |
| `program.h:335-336` | `snode_trees_`, `std::stack<int> free_snode_tree_ids_` |
| `snode_tree.h:17,25,27,40` | `kFirstID`, ctor `int id`, `int id()`, `int id_` |
| `snode_tree.h:52` / `snode_tree.cpp:34-39` | `std::unordered_map<int,int> get_snodes_to_root_id` |
| `gfx/snode_tree_manager.h:33,35` | `get_field_in_tree_offset(int tree_id, ...)`, `get_snode_tree_device_ptr(int tree_id)` |
| `ir/snode.h:88-89` | `static std::atomic<int> counter; int id{0};` (agent 01's file, the origin) |

**Serialised, so a width change is an offline-cache format change:**
`llvm_offline_cache.h:68` `SNodeCacheData::id` is `int` with
`TI_IO_DEF(id, type, cell_size_bytes, chunk_size)` at `:73`;
`llvm_offline_cache.h:76-77` `tree_id`, `root_id` are `int` with `TI_IO_DEF` at
`:81`; `:119` `std::unordered_map<int, FieldCacheData> fields`.

### 4.2 Coordinates and element addressing

| Location | Declaration |
|---|---|
| `runtime.cpp:288-290` | `PhysicalCoordinates { i32 val[taichi_max_num_indices]; }` |
| `runtime.cpp:517-521` | `Element { Ptr element; int loop_bounds[2]; PhysicalCoordinates pcoord; }` |
| `runtime.cpp:312` | `Ptr (*lookup_element)(Ptr, Ptr, int i)` |
| `runtime.cpp:316` | `u1 (*is_active)(Ptr, Ptr, int i)` |
| `runtime.cpp:318` | `i32 (*get_num_elements)(Ptr, Ptr)` |
| `runtime.cpp:320-322` | `void (*refine_coordinates)(PhysicalCoordinates *, PhysicalCoordinates *, int index)` |

**Truncating returns.** `StructMeta::max_num_elements` is `i64`
(`runtime.cpp:310`) but every accessor narrows it to `i32`:
`node_dense.h:10-12`, `node_pointer.h:10-12`, `node_bitmasked.h:10-12`.
`node_root.h:21-23` returns a literal 1; `node_dynamic.h:116-119` returns
`node->n`, itself `i32` (`node_dynamic.h:5`).

**Pointer arithmetic performed in `int`.** In `node_pointer.h`, `8 * i` and
`8 * (num_elements + i)` are `int * int` before the pointer add:
`:44`, `:45`, `:69`, `:70`, `:86`, `:92`. `[I]` This overflows at
i >= 2^28 = 268435456; `num_elements + i` can overflow before the multiply.

Other node-type index arithmetic, all `int`:
`node_dense.h:14,18,22`; `node_bitmasked.h:14,20,23,29,32,38,41`;
`node_dynamic.h:16,21,22,35,39,61,65,67,79,90,92,95,99,103,109`;
`node_root.h:9,12,16`.

### 4.3 ListManager — upstream's own TODO is at `runtime.cpp:424-425`

| Location | Declaration |
|---|---|
| `runtime.cpp:427` | `static constexpr std::size_t max_num_chunks = 128 * 1024;` |
| `runtime.cpp:431` | `i32 log2chunk_num_elements` |
| `runtime.cpp:432` | `i32 lock` |
| `runtime.cpp:433` | `i32 num_elements` |
| `runtime.cpp:451-456` | `i32 reserve_new_element()`, `atomic_add_i32(&num_elements, 1)` |
| `runtime.cpp:467-473` | `i32 get_num_active_chunks()` |
| `runtime.cpp:479-481` | `void resize(i32 n)` |
| `runtime.cpp:483-486` | `Ptr get_element_ptr(i32 i)` — index and shift in 32 bits (`element_size` is `size_t`, so that one multiply is 64-bit) |
| `runtime.cpp:488-491` | `T &get(i32 i)` |
| `runtime.cpp:493-496` | `Ptr touch_and_get(i32 i)` |
| `runtime.cpp:498-500` | `i32 size()` |
| `runtime.cpp:502-512` | `i32 ptr2index(Ptr)`, composed index `(i << log2chunk_num_elements) + i32(...)` at `:507-508` |
| `runtime.cpp:1664` | `void ListManager::touch_chunk(int chunk_id)` |

**[I]** Capacity mismatch: `max_num_chunks` 131072 x the 65536 elements per
chunk used for element lists (`runtime.cpp:1006`) is 2^33 addressable slots,
against an `i32 num_elements` counter that saturates at 2^31.

### 4.4 NodeManager

`runtime.cpp:631-641`: `i32 lock`, `i32 element_size`, `i32 chunk_num_elements`,
`i32 free_list_used`, `i32 recycle_list_size_backup`,
`using list_data_type = i32`.
Constructor `runtime.cpp:643-645` takes `i32 element_size, i32
chunk_num_elements`; the host passes `std::size_t node_size`
(`runtime.cpp:1026-1030`), narrowed at the `create<NodeManager>` call.
`allocate()` uses `int old_cursor` (`runtime.cpp:667`); `i32 locate(Ptr)`
(`:679`); `gc_serial()` loops on `int i` (`:690,699`).

Chunk counts, which my earlier report listed as constructor parameters without
stating either value: the ctor's default is `128 * 1024` at `runtime.cpp:649`
(inside the `if` at `:648-650`), and the 128 MB halving loop is at `:652-655`.
**The only call site overrides the default**: `runtime.cpp:1030` passes
`1024 * 16`, so `chunk_num_elements` is **16384**, and the halving loop engages
only when `node_size > 8192` (16384 x 8192 = 128 MiB). **[V]** The three lists
are created at `runtime.cpp:658-659`, `:660-661` and `:662-663`. Chunk sizes
follow in 2.5.

### 4.5 Loop indices handed to generated kernels

`runtime.cpp:43` `RangeForTaskFunc` third parameter `int i`.
`runtime.cpp:44` `MeshForTaskFunc` third parameter `uint32_t i`.
`runtime.cpp:45-49` `parallel_for_type` — `int splits`, `int
num_desired_threads`, callback `(void *, int thread_id, int i)`.
`runtime.cpp:1385` `BlockTask` — the two bounds are `int, int`.
`runtime.cpp:1387-1394` `cpu_block_task_helper_context` — `int element_size`,
`int element_split`.
`runtime.cpp:1403-1421` `cpu_struct_for_block_helper` — `element_id`,
`part_size`, `part_id`, `lower`, `upper` all `int`.
`runtime.cpp:1471-1481` `range_task_helper_context` — `int begin, end,
block_size, step`.
`runtime.cpp:1483-1508` `cpu_parallel_range_for_task` — `ctx.begin + task_id *
ctx.block_size` at `:1495` is a 32-bit multiply.
`runtime.cpp:1511-1537` `cpu_parallel_range_for(..., int begin, int end, int
step, int block_dim, ...)`.
`runtime.cpp:1541-1561` `gpu_parallel_range_for(..., int begin, int end, ...)`,
`int idx = thread_idx() + block_dim() * block_idx() + begin;` at `:1548`.
`runtime.cpp:1567-1575` `mesh_task_helper_context` — `int num_patches, block_size`.
`runtime.cpp:1599-1625` `cpu_parallel_mesh_for(..., int num_patches, int block_dim, ...)`.
`runtime.cpp:1627-1647` `gpu_parallel_mesh_for(..., int num_patches, ...)`.
`runtime.cpp:1650-1656` `i32 linear_thread_idx(RuntimeContext *)`, used to
index `rand_states` at `runtime.cpp:1790-1792`.

### 4.6 Listgen

`runtime.cpp:1270-1273` `clear_list`.
`runtime.cpp:1282-1329` `element_listgen_root` — `int c_start, c_step, c`;
`ch_num_elements` and `ch_element_size` are `i32` from `get_num_elements`;
`c * ch_element_size` at `:1322` and `(c + 1) * ch_element_size` at `:1323` are
32-bit multiplies.
`runtime.cpp:1331-1383` `element_listgen_nonroot` — `int num_parent_elements`,
`i_start/i_step/j_start/j_step`, `i`, `j`, `j_lower`, `j_higher`, `ch_lower`.

### 4.7 Host-side widths in `taichi/program/` and `taichi/runtime/`

`program/program.cpp:279-280,291` — the SNode **reader** kernel hard-codes
`PrimitiveType::i32` for every index argument and for every scalar param.
`program/program.cpp:305-306,315,322` — the SNode **writer**, likewise.
`program/program.cpp:330` `uint64 Program::fetch_result_uint64(int i)`.
`program/snode_rw_accessors_bank.cpp:8-14` `set_kernel_args(const
std::vector<int> &I, int num_active_indices, ...)`; the six accessors at
`:38,49,61,73,84,95` all take `const std::vector<int> &I`. This is the host
read/write path for a field and it carries `int` per-axis coordinates.
`program/snode_expr_utils.cpp:52-56` / `.h` `place_child(..., const
std::vector<int> &offset, int id_in_bit_struct, ...)`.
`program/context.h:20` `int32_t cpu_thread_id`.
`llvm_runtime_executor.h:87,93,133` `fetch_result(int i, ...)`,
`fetch_result(char *, int offset)`, `fetch_result_uint64(int i, ...)`.
`llvm_runtime_executor.cpp:188-209` `print_list_manager_info` fetches
`ListManager_get_element_size` and `_get_max_num_elements_per_chunk` as
`int32`, though both are `std::size_t` on the device
(`runtime.cpp:429-430`) — an existing host/device width mismatch.
`llvm_runtime_executor.cpp:257-270` `get_snode_num_dynamically_allocated`
returns `std::size_t` from an `int32` query.
`llvm_runtime_executor.cpp:639` `int starting_rand_state = config_.random_seed
* 1048391;` — 32-bit multiply.
`llvm_runtime_executor.cpp:642-653` `int num_rand_states`, computed as
`config_.saturating_grid_dim * config_.max_block_dim` at `:648`, then used as
`sizeof(RandState) * num_rand_states` at `runtime.cpp:902-903`.
`llvm_runtime_executor.cpp:676-677` `call<void *, int32_t, int32_t>(
"runtime_get_memory_requirements", ...)`.
`llvm_program.h:199` `offset += child->cell_size_bytes *
child->num_cells_per_container;` — both operands 64-bit, clean, but the
enclosing `get_field_in_tree_offset` at `:188` carries `int tree_id` and the
function is marked `FIXME` at `:189`.

### 4.8 Verified 64-bit clean, listed so it is not re-derived

`runtime.cpp:546-550` `PreallocatedMemoryChunk` — `std::size_t
preallocated_size`.
`runtime.cpp:563` `size_t root_mem_sizes[]`.
`runtime.cpp:574-582` the two allocator declarations — `std::size_t size,
alignment`.
`runtime.cpp:597` `i64 total_requested_memory`, `atomic_add_i64` at `:828`.
`runtime.cpp:600` `set_result(std::size_t i, T t)`.
`runtime.cpp:823-874` the whole allocation path.
`runtime.cpp:891-906` `runtime_get_memory_requirements` — `i64 size`.
`runtime.cpp:1876-1898` the local-stack ops — `u64` counter, `std::size_t
element_size`.
`struct/struct.h:12` `std::size_t root_size`.
`llvm_offline_cache.h:70-71,78` `cell_size_bytes`, `chunk_size`, `root_size`
are `size_t`.
`gfx/runtime.h:40,94` `std::size_t num_snode_trees`.
`gfx/runtime.cpp:730` `add_root_buffer(size_t)`.
`gfx/snode_tree_manager.cpp:31-47` `size_t get_field_in_tree_offset`, `size_t
offset`.
`rhi/llvm/device_memory_pool.h:18,24-25,31,36` — `std::size_t` throughout.

### 4.9 The IR-side origin (seam with agent 01, listed not owned)

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

A **warning**, not an error. `AxisExtractor::num_elements_from_root`, `::shape`,
`::acc_shape` are `int` (`ir/snode.h:41,45,49`) while
`SNode::num_cells_per_container` is `int64` (`ir/snode.h:97`) and
`SNode::max_num_elements()` returns `int64` (`ir/snode.h:306-308`).
`ir/snode.h:98` `int chunk_size{0}` is 32-bit.
`PhysicalCoordinates::val` (`runtime.cpp:289`) is the runtime side of this
same width.

### 4.10 Fixed-size arrays whose dimension is an `inc/constants.h` value

**The rule that generates this list, revision 4.** Revision 2 headed this
section "Every fixed-size array sized by an `inc/constants.h` value" over a
`grep -rn "\[taichi_max_num\|\[kMaxNum"`, which is keyed on two name prefixes
and cannot see a constant with any other name. `adversary2-03-1.md` section 7.1
named the two it misses and revision 3 did not act on it. Withdrawn and
recounted under a rule that can see the whole class. **[V]**

Rule: extract all 20 `constexpr` names defined in `taichi/inc/constants.h`,
then match `[<name>` followed by `]`, `*`, `+` or `-` across `.cpp`, `.h`, `.cu`
and `.inc.h` under `taichi/`, build excluded. **12 matches**, of which **10 are
array declarations**, one is a comment (`codegen/llvm/codegen_llvm.cpp:2363`)
and one is an index expression, not a declaration
(`runtime.cpp:712`, `ctx->result_buffer[taichi_result_buffer_ret_value_id + idx]`).
The ten declarations:

- `ir/snode.h:78` `AxisExtractor extractors[taichi_max_num_indices]`
- `ir/snode.h:81` `int physical_index_position[taichi_max_num_indices]`
- `runtime.cpp:289` `i32 val[taichi_max_num_indices]`
- `runtime.cpp:562` `Ptr roots[kMaxNumSnodeTreesLlvm]`
- `runtime.cpp:563` `size_t root_mem_sizes[kMaxNumSnodeTreesLlvm]`
- `runtime.cpp:567` `ListManager *element_lists[taichi_max_num_snodes]`
- `runtime.cpp:568` `NodeManager *node_allocators[taichi_max_num_snodes]`
- `runtime.cpp:569` `Ptr ambient_elements[taichi_max_num_snodes]`
- `runtime.cpp:587` `char error_message_template[taichi_error_message_max_length]`
  (`constants.h:18`) — **added in revision 4**
- `runtime.cpp:588` `uint64 error_message_arguments[taichi_error_message_max_num_arguments]`
  (`constants.h:19`) — **added in revision 4**

Ten, and the eight rows revision 3 carried were eight of the ten. Neither added
row scales with SNode count, so nothing in sections 2 or 5 moves. **[V]**

**What this rule can and cannot see.** It sees any array whose dimension names a
constant from that header, including a dimension written as an expression: I ran
the looser form, matching a constant anywhere inside a bracket rather than at
its start, and it returns **no match the strict form misses**, so no
`[2 * taichi_max_num_snodes]`-shaped declaration exists. **[V]** It cannot see a
constant used as a **bound** rather than as a dimension, which is the separate
fault below, and it cannot see an array whose dimension is a second constant
defined elsewhere and initialised from one of these. I did not search for the
latter class and do not claim it is empty. **[I]**

`taichi/python/export_lang.cpp:1222` exports `get_max_num_indices` to Python.
Out of scope per brief 1.2, noted because it is a C++ binding reading the
constant.

**A second fault in the original grep, corrected in revision 2 and unchanged.**
Whatever names it keys on, a declaration-shaped search matches array
**declarations** only. It cannot catch a constant used as a bound rather than a
dimension, and two such uses in my territory were missed: **[V]**

- **`taichi_listgen_max_element_size = 1024`** (`taichi/inc/constants.h:28`),
  used as a `std::min` cap on the per-element loop-bound split at
  `runtime.cpp:1316` and `:1369`, and mirrored in codegen at
  `codegen/llvm/codegen_llvm.cpp:2291`. It is not SNode-count-scaled, so it is
  not a 6.1 item, but it is a third hard 1024 sitting in the struct-for
  dispatch path, and brief 6.4 makes dispatch throughput the sole criterion.
- **`taichi_max_num_mem_requests = 1024 * 64`** (`taichi/inc/constants.h:16`)
  has **zero uses anywhere in the tree**. My own grep over `.cpp`, `.h` and
  `.py`, build excluded, returns only the definition.

### 4.11 A loop bound that is the same 131072, on the sparse deactivate path

Not a width assumption, but it belongs with them because it is the same
constant that produces the 1 MiB table, and brief 6.4 makes dispatch throughput
the only criterion.

`ListManager::ptr2index` (`runtime.cpp:502-512`) loops to `max_num_chunks`
= 131072 and calls `taichi_assert_runtime(runtime, chunks[i] != nullptr, ...)`
on every iteration. It is reached from `Pointer_deactivate`
(`node_pointer.h:67-82`) and `Dynamic_deactivate` (`node_dynamic.h:43-59`) via
`NodeManager::recycle` (`runtime.cpp:683-686`) and `locate` (`:679-681`). **[V]**

The cost depends on the arch, because the assert's failure path does. Reading
`taichi_assert_format` (`runtime.cpp:766-815`): the early return at `:780-781`
fires when the test **passes**; on failure the error is recorded and the thread
is killed only under `ARCH_cuda` (`asm("exit;")` at `:800`) or `ARCH_amdgpu`
(`asm("S_ENDPGM")` at `:802`). The CPU build has no `exit` and returns
normally. **[V]**

So: **[I]**

- **Success case, every arch.** The loop exits at the chunk holding the
  pointer, so it is O(index of that chunk), bounded by the touched set, not by
  131072.
- **Pointer-not-found, CUDA/AMDGPU.** The thread dies at the first untouched
  chunk.
- **Pointer-not-found, CPU.** The error flag is set once (guarded by
  `if (!runtime->error_code)` at `:782`) and the loop then runs all 131072
  iterations, because `chunks[i] == nullptr` satisfies `chunks[i] <= ptr` and
  fails `ptr < chunks[i] + chunk_size`, so no iteration matches and none
  breaks.

Brief section 2 makes CPU-only a first-class target, not a fallback, so the
third case is not a corner.

`get_num_active_chunks` (`runtime.cpp:467-473`) genuinely does walk all 131072
unconditionally, but its only reachable caller is the host debug printer
(`runtime.cpp:743-747` -> `llvm_runtime_executor.cpp:200-201`). **[V]**

Recorded, not judged. Escalation E14.

---

## 5. Escalations

**E1 — The assertion does not bound the array index.**
`struct_llvm.cpp:266` bounds a per-tree SNode count; the arrays are indexed by
a global, never-recycled `SNode::id`. Multiple trees, or any create/destroy
cycling, put the index past the assertion's reach. Section 3.1. The correct
response could be a bound at the write site, a per-tree id space, id recycling,
or something else. Each is a design decision. I am not choosing one.

**E2 — `kMaxNumSnodeTreesLlvm = 512` is unbounded and undiscussed.**
It is not named in brief 6.1 but it sizes two runtime arrays and has no
assertion anywhere. `Program::allocate_snode_tree_id`
(`program/program.cpp:559-567`) will hand out 512 and beyond without
complaint. Whether it is in scope for 6.1, and what its replacement value is,
needs the planner.

**E3 — SNode ids are cumulative, not live.**
`SNode::counter` resets only in the `Program` constructor
(`program/program.cpp:144`). Any figure chosen for a replacement ceiling is a
budget on total SNodes ever created in a process, not on the working set size.
This bears directly on item 2 of brief 8.1. It also interacts with brief 4.6:
an SNode configuration "composed from the larger data stores for working scope"
implies recomposition, which consumes ids.

**E4 — Tree destruction leaves stale device state and leaks, and this fork is
expected to reach it.** Section 3.2. `destroy_snode_tree` clears neither the
runtime tables nor the objects they point at, the generated setters that could
clear them have zero call sites anywhere, the device allocator has no free
path, and `snode_tree_allocs_` keeps a dangling `DeviceAllocation` that
`get_snode_tree_device_ptr` will hand out on a recycled id. Repeated
recomposition accumulates 5.0 MiB per populated sparse SNode against a pool
that holds 204 of them. Today the only caller of `Program::destroy_snode_tree`
is the Python binding (`export_lang.cpp:569-570`), which brief 1.2 puts out of
scope, so it is latent rather than live. It becomes live the moment this fork
adds its own recompose path, and brief 4.6 describes exactly such a path.
Fixing it is a lifecycle design decision, not a width change.

**E5 — The replacement for 1024 is a tuple, not a number.**
Sections 2.5 to 2.9. Four constants bind together:

| Constant | Where | Today |
|---|---|---|
| `taichi_max_num_snodes` | `taichi/inc/constants.h:12` | 1024 |
| `CompileConfig::device_memory_GB` | `taichi/program/compile_config.cpp:63` | 1 |
| `ListManager::max_num_chunks` | `runtime.cpp:427` | 128 * 1024 |
| element-list chunk count | `runtime.cpp:1006` | 1024 * 64 |

**On CUDA and AMDGPU**, the first two are already matched to within 0.4 %: the
ceiling is 1024 and the default pool holds 1020 element-list headers. Raising
either alone is inert. The third sets the 1,052,672-byte header charge and the
fourth the 4,194,304-byte first-touch chunk, which together make a populated
sparse SNode 5.0 MiB and cap the default pool at 204 of them.

**Qualification, revision 3.** Revision 2 stated "raising either alone is
inert" without an arch qualifier. That is false on the CPU path, where no pool
exists at all: both call sites of `preallocate_runtime_memory` are gated on
CUDA or AMDGPU (`llvm_runtime_executor.cpp:412-413` and `:719-723`),
`preallocated_size` stays 0, and `allocate_aligned` falls through to
`host_allocator` (`runtime.cpp:830-834`) and on to `UnifiedAllocator::allocate`
(`taichi/rhi/common/unified_allocator.cpp:61-85`), which has no ceiling.
**On CPU `taichi_max_num_snodes` is the binding limit and raising it alone is
effective**, bounded only by host RAM at the same ~5.0 MiB per populated sparse
SNode. Section 2.6. Brief section 2 makes CPU-only a first-class target and
brief 5.2's three tiers are all GPUs, so item 8.1.2 does not have one answer
across the target set. Whether the install-time loader emits one value or a
per-arch pair is the planner's call, not mine.

**Correction, revision 3, and it inverts the relative cost of the pair.**
Revision 2 said `device_memory_GB` was "reachable only from Python
(`export_lang.cpp:208-210`)" and "a hard-coded 1" at C++ level, marked **[V]**.
That is false; section 2.6 carries the source. The two halves of the pair are
not equally expensive to move: **[I]**

- `taichi_max_num_snodes` is a `constexpr int` (`taichi/inc/constants.h:12`)
  compiled into each per-arch `.bc` by a standalone `clang` command that
  inherits no project compile definitions
  (`taichi/runtime/llvm/runtime_module/CMakeLists.txt:8`, the arch loop at
  `:29-31`). Moving it needs a build-time mechanism that does not exist today.
  Section 1.1 items 1 and 2, escalation E9.
- `device_memory_GB` is a public `float64` field
  (`taichi/program/compile_config.h:71`) on a `TI_DLL_EXPORT` global
  (`compile_config.h:110`, defined `taichi/util/lang_util.cpp:14`) that
  `Program::Program` copies at `taichi/program/program.cpp:75`. Moving it needs
  no mechanism at all: a C++ driver writes the field before constructing a
  `Program`.

So the expensive half of the pair is the constant, not the pool knob. Brief
section 5 makes configuration an install-time decision, and both are such a
decision, but whether they are parameterised by the same mechanism is the
planner's call. I am not proposing values for any of the four and per standing
instruction 3 I am not calling any of them surplus.

**E5a — The pool does not scale with the card, so the tiers in brief 5.2 do
not differ under the default.** Section 2.6. A 2 GB GTX 750 and a 12 GB
RTX 3060 both get a 1 GiB `runtime_memory_chunk`. Whether the install-time
loader is permitted to set `device_memory_GB` or `device_memory_fraction` from
detected hardware is undecided and is the same decision as E5.

**E5b — Exhaustion is a grid abort, not degradation.** Section 2.8.
`__assertfail` at `runtime.cpp:864-869` under `ARCH_cuda`, then
`taichi_assert_runtime` at `:872`. Raising the constant without raising the
pool produces a hard kernel abort at materialisation or first listgen. Whether
that failure mode is acceptable for an edge device, or wants a preflight check,
is a decision I am not making.

**E5c — `demote_dense_struct_fors` is a fourth input to the footprint.**
Section 2.4. `all_dense` is the conjunction of that flag
(`compile_config.cpp:18`, default true; forced true for SPIR-V at `:72-73`)
with "every SNode is dense, place or root". With the flag false, every tree
pays a `ListManager` per SNode, dense trees included, and on CUDA with the
device memory pool the flag also gates whether the pool is created at all
(`llvm_runtime_executor.cpp:412-413`). **Corrected in revision 3:** it is not
Python-only. It is a public field at `taichi/program/compile_config.h:28` on
the global `default_compile_config` (`compile_config.h:110`,
`taichi/util/lang_util.cpp:14`) that `Program::Program` copies at
`taichi/program/program.cpp:75`, so a C++ driver can set it with no new
mechanism; `export_lang.cpp:201-202` is one client of that global. Whether the
install-time loader may touch it is undecided.

**E6 — Widening SNode ids changes the offline cache format.**
`SNodeCacheData::id`, `FieldCacheData::tree_id` and `::root_id` are `int` and
are serialised (`llvm_offline_cache.h:68,73,76-77,81`). A width change needs a
cache version bump and a decision on whether existing caches are invalidated.
Not mine to decide.

**E7 — `gfx` tree ids diverge after one destroy-then-add cycle.**
Section 3.4. My earlier report escalated *whether* they diverge; that question
is answered there and is no longer open. What remains escalated is the remedy:
`compiled_snode_structs_` and `root_buffers_` are indexed by push-back order
while `Program` recycles ids, and reconciling them could mean a map keyed by
tree id, an id-preserving vector, or not recycling ids at all. Pre-existing, not
caused by anything in section 6, but a raised ceiling makes multi-tree
configurations more likely and so makes it more reachable.

**E8 — `int` pointer arithmetic in `node_pointer.h`.**
`8 * i` and `8 * (num_elements + i)` at `node_pointer.h:44,45,69,70,86,92` are
computed in `int`. Overflow at i >= 2^28. Reported as a width finding under
item 6.2; whether it is fixed as part of 6.2 or separately is not my call.

**E9 — Where the build parameter should live.**
`runtime.cpp` is compiled by a standalone clang command that inherits no
project compile definitions (`runtime_module/CMakeLists.txt:8`), and its output
lands in the source tree (`:10`). Two shapes present themselves — extend that
command with a `-D`, or generate a configured header on the include path. Both
are viable and the choice is architectural. I am not choosing.

**E10 — A stale `.bc` paired with a fresh host binary would not be caught.**
The only host-side C++ use of `taichi_max_num_snodes` is the assertion at
`struct_llvm.cpp:266`, which by E1 is not checking the array bound. So a
mismatch between the value the bitcode was built with and the value the host
was built with has no detection path today. If the constant becomes
install-time variable, mismatch becomes possible where it previously was not.
Flagging; a version stamp or equivalent is a design decision.

**E11 — The element-list chunk count is not a bounds constant and I am not
touching it.** `runtime.cpp:1006` passes `1024 * 64` as the elements per chunk,
which with `sizeof(Element) == 64` makes each touched chunk 4 MiB. It is the
larger half of the 5.0 MiB populated per-SNode figure. Listed in E5's tuple.
Recorded, not judged.

**E12 — `taichi_listgen_max_element_size = 1024`** (`taichi/inc/constants.h:28`,
used at `runtime.cpp:1316`, `:1369` and `codegen/llvm/codegen_llvm.cpp:2291`).
A third hard 1024, in the struct-for dispatch path rather than the footprint.
Not SNode-count-scaled, so not obviously a 6.1 item, but brief 6.4 makes
dispatch throughput the sole criterion. Whether it is in scope is undecided.
Separately, `taichi_max_num_mem_requests` (`constants.h:16`) has zero uses
anywhere; I am recording that and, per standing instruction 3, not calling it
surplus.

**E13 — The install-rule variable fault at
`runtime_module/CMakeLists.txt:13`.** Section 1.1 item 6. Line 13 reads
`${arch}`, the caller's `foreach` variable, where line 8 reads the function
parameter `${rtm_arch}`. Latent today, surfaces under parameterisation. This
sits in agent 04's build territory; I am flagging it, not chasing it.

**E14 — The 131072-iteration bound on the sparse deactivate path.**
Section 4.11. `ListManager::ptr2index` behaves differently by arch on the
pointer-not-found path, and on the CPU build it runs all 131072 iterations
after flagging an error. Brief section 2 makes CPU-only a first-class target
and brief 6.4 makes throughput the sole criterion. Recorded, not judged.

**E15 — Which branch each target tier takes on the memory-pool test, and what
headroom the capacity rows must reserve.** Section 2.7. When
`use_device_memory_pool()` is false, the SNode tree root buffers, ndarrays and
argpacks are cut from the same 1 GiB `runtime_memory_chunk` as the
`ListManager`s, through
`snode_tree_buffer_manager.cpp:15` -> `llvm_runtime_executor.cpp:485` ->
`cuda_device.cpp:56-61` -> `allocator.cpp:54-55` -> `cuda_device.cpp:82-84` ->
`runtime.cpp:883-885`. That makes every capacity row in 2.7 an upper bound on
AMDGPU always, and on CUDA whenever the driver is below 11.2 or the device does
not report `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`
(`taichi/rhi/cuda/cuda_context.cpp:35-53`). Two things stay undecided and I am
deciding neither: which branch the GTX 750, the GTX 1070 and the RTX 3060
actually take, which this tree does not record and which I did not test; and
how much headroom a sizing rule for item 8.1.2 should reserve for root and
ndarray storage, which is workload-dependent.

---

## 6. Revision record

### 6.0 Revision 2

After adversarial review by `adversary-03-1.md` and
`adversary-03-2.md`. Working notes for this pass are entries N26 to N40 of
`notes-03b-runtime-struct.md`.

**Withdrawn.** "The card, not the constant, is the binding limit at the
baseline tier", and the figure of roughly 512 pointer SNodes on a 2 GB card.
The pool binds, not the card, and the corrected figure at the default
configuration is 254. Sections 2.6 and 2.7.

**Corrected.** All per-SNode footprint figures now carry the allocator's
alignment charge (section 2.5) and the 4 MiB first-touch chunk, so a populated
sparse SNode is 5.0 MiB rather than the 1.0 MiB I reported. The `all_dense`
conjunction is now stated in full (section 2.4). Eight citations into
`llvm_program.{h,cpp}` and `llvm_runtime_executor.cpp` are corrected (N39).

**Strengthened.** The leak in 3.2 now records that the generated setters have
zero call sites, that `snode_tree_allocs_` retains a dangling
`DeviceAllocation`, and that the only caller of `Program::destroy_snode_tree`
is the Python binding, making it latent today and live as soon as this fork
adds its own recompose path.

**Closed.** The gfx tree-id divergence, previously escalated as an open
question, is answered in 3.4. The remedy stays escalated.

**Added.** Sections 2.5 to 2.9, 4.11, the `NodeManager` chunk-count override in
4.4, the two missed constants in 4.10, the CMake variable fault in 1.1 item 6,
and escalations E5a, E5b, E5c, E11, E12, E13, E14.

**Downgraded.** The sm_60 bitcode flag in 1.3, which is double-gated and cannot
fire.

**Adjudicated where the two adversaries disagreed**, with the work shown in
notes N27, N28, N31 and N32:

| Question | Ruling | Basis |
|---|---|---|
| Pool capacity, element-list headers | **1020**, not 1023 | the alignment charge at `runtime.cpp:848-853` is billed to the caller |
| Overrun of 1024 lists against the default pool | **4,190,248 bytes**, not 40,960 | same |
| `TI_ASSERT(total_prealloc_size <= total_mem)` | `llvm_runtime_executor.cpp:620` | offset print |
| `__assertfail` span and its fallback | `runtime.cpp:864-869` and `:872` | offset print |
| Is `ptr2index` a 131072-iteration scan? | **Arch-dependent**; full scan on CPU, thread death on GPU, early exit on success | `taichi_assert_format` at `runtime.cpp:766-815` kills the thread only under `ARCH_cuda` / `ARCH_amdgpu` |
| 2 GB card at `device_memory_fraction = 0.9` | 1836 / 368 / 458 | adversary 03-2's 351 and 211 do not reproduce and are not mutually consistent |

### 6.1 Revision 3 — amendment after round-two adversarial review

An amendment, not a re-investigation. I read `adversary2-03-1.md` and
`adversary2-03-2.md` and adjudicated four claims against source. Working notes
are entries N43 to N47 of `notes-03b-runtime-struct.md`. Where the two round-two
adversaries split on severity I took the harsher list, which is
`adversary2-03-2.md`'s.

| Claim | Ruling | Where it landed |
|---|---|---|
| E5's "raising either alone is inert" is stated too broadly and must be a CUDA and AMDGPU statement | **UPHELD** | 2.6, E5 |
| The SNode tree root buffers do not sit outside the 1 GiB pool when `use_device_memory_pool()` is false | **UPHELD**, with one qualifier of my own | 2.7(c), E15 |
| `device_memory_GB` is reachable from C++, not only from Python, and that inverts the relative cost of the pair in plan 8.1.2 | **UPHELD**, and my **[V]** marking was wrong | 2.6, E5, and the same fault for `demote_dense_struct_fors` in 2.4 and E5c |
| The listgen spans are wrong | **UPHELD** | 2.5, 4.6 |

**Withdrawn in revision 3.** Two statements I had marked **[V]**:
`device_memory_GB` and `device_memory_fraction` are "reachable only from
Python … so at C++ level the pool is a hard-coded 1 GiB" (was 2.6), and
`demote_dense_struct_fors` is "settable only from Python … effectively a
hard-coded `true` at C++ level today" (was 2.4). Both are false. The source is
in 2.6: `CompileConfig` is a plain struct with public fields
(`taichi/program/compile_config.h:8`, `:28`, `:71-72`); `default_compile_config`
is `extern TI_DLL_EXPORT` at `compile_config.h:110`, defined at
`taichi/util/lang_util.cpp:14`; and `Program::Program` copies it at
`taichi/program/program.cpp:75`, with `config.fit()` at `:77` touching neither
field (`compile_config.cpp:67-76`). Also withdrawn: "the root buffers … are
separate allocations outside it" (was 2.7(c)).

**Qualifier of my own, on the root-buffer claim.** The adversary's chain
reproduces hop for hop and I confirmed every one. Its further statement that
this makes the capacity rows upper bounds "on exactly the baseline-tier
hardware" does not follow from this tree. The branch is a driver-version and
device-attribute test taken at runtime (`taichi/rhi/cuda/cuda_context.cpp:35-53`),
and nothing in the repository records which branch the GTX 750, the GTX 1070 or
the RTX 3060 take. I recorded the mechanism and escalated the tier mapping as
E15 rather than asserting it. The CPU path is also not on that chain:
`CpuDevice::allocate_memory_runtime` (`taichi/rhi/cpu/cpu_device.cpp:44-51`)
calls `allocate_memory` directly and never reaches `allocate_with_cache`.

**Corrected.** The two listgen spans: `element_listgen_root` closes at
`runtime.cpp:1329`, not 1328, and `element_listgen_nonroot` at `:1383`, not
1380. Both appear twice, in 2.5 and 4.6.

**Added.** Escalation E15, the pool's arch scope and the CPU consequence in 2.6,
and the seven-hop root-buffer chain in 2.7(c).

**Not disturbed**, per the round-two verdicts of both adversaries: `ptr2index`,
the pool capacity of 1020 and the 4,190,248-byte overrun, and the 2 GB card
figures of 1836 / 368 / 458. Both adversaries withdrew their round-one
positions in this report's favour on all three. The self-audit was tested
independently by both and passed, with zero references past end of file across
roughly 504 line citations.

---

## 7. What I did not do

- I did not read agent 03's notes or report. For revision 2 I read
  `adversary-03-1.md` and `adversary-03-2.md` only; for revision 3,
  `adversary2-03-1.md` and `adversary2-03-2.md` only.
- I did not modify, create or delete any file except this report and
  `notes-03b-runtime-struct.md`. Revision 3 changed no source file and no other
  agent's file. The `sizeof` verification in section 2.1 was
  done by piping source through `clang++ -fsyntax-only` on stdin, creating
  nothing.
- I did not propose removals, cleanups or added abstraction.
- I did not enter `taichi/codegen/`, `taichi/transforms/` or `taichi/ir/`
  except to read the specific lines cited as seams with agents 01 and 02, which
  are marked as such.
