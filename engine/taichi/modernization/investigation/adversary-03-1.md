# Adversary 03-1 — runtime and struct layer

Judging `report-03-runtime-struct.md` (pass A) and `report-03b-runtime-struct.md`
(pass B) against the source at `/opt/project/taichi`.

All line numbers below were re-read from the source by this adversary. Where I
say a report is wrong I give the source line that shows it.

---

## 0. Verdicts

**CORRECT: qualified pass.** Every load-bearing size figure in both reports is
right, and I confirmed the two headline `sizeof` values with the compiler
myself. Pass A carries a systematic line-number fault in two files. Pass B's
one quantitative tier claim is wrong on its premise, not its arithmetic.

**COMPLETE: no.** Both passes stopped one call short of the fact that decides
open question 8.1.2. Neither found `LlvmRuntimeExecutor::preallocate_runtime_memory`
(`taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632`) or the constant it
reads, `device_memory_GB = 1` (`taichi/program/compile_config.cpp:63`). That is
the pool the ~1 MiB `ListManager`s are cut from, and it exhausts at roughly
1020 element lists — just *below* the 1024 constant the whole project is trying
to raise. Neither report mentions it. Section 6 below.

**Major revision required: yes, but narrowly.** Both reports need section 2 /
section 2.4 reworked against the preallocation pool. Pass A additionally needs
its `struct/snode_tree.cpp` and `gfx/snode_tree_manager.cpp` citations
corrected, all of which are wrong. Nothing else in either report needs to go
back.

---

## 1. The convergent claim against the plan: upheld

The brief asked me to establish, not accept, that the real cost of the 1024
constant is the per-SNode `ListManager`, not the 24 KiB of pointer arrays that
section 6.1 of the plan records.

**It holds.** I verified it independently of both reports.

`taichi/runtime/llvm/runtime_module/runtime.cpp:426-428`:

```
struct ListManager {
  static constexpr std::size_t max_num_chunks = 128 * 1024;
  Ptr chunks[max_num_chunks];
```

131072 pointers, declared inline, not a pointer to a table. I compiled the real
`runtime.cpp` through `clang++ -fsyntax-only` on stdin with appended
`static_assert`s (no file created, nothing modified) and every one passed:

| Assertion | Result |
|---|---|
| `sizeof(LLVMRuntime) == 35256` | pass |
| `sizeof(ListManager) == 1048616` | pass |
| `sizeof(NodeManager) == 56` | pass |
| `sizeof(Element) == 64` | pass |
| `sizeof(RandState) == 20` | pass |
| `sizeof(StructMeta) == 72` | pass |
| `sizeof(PhysicalCoordinates) == 48` | pass |
| `offsetof(LLVMRuntime, roots) == 88` | pass |
| `offsetof(LLVMRuntime, element_lists) == 8296` | pass |
| `offsetof(LLVMRuntime, node_allocators) == 16488` | pass |
| `offsetof(LLVMRuntime, ambient_elements) == 24680` | pass |
| `offsetof(LLVMRuntime, temporaries) == 32872` | pass |

Both reports' measurements are therefore confirmed by a third independent
measurement. The closed form both derive,
`sizeof(LLVMRuntime) = 2488 + 24*N + 16*T`, follows from the field list and
checks at N=1024, T=512.

The allocation is eager and per SNode. `runtime.cpp:1003-1007` creates one
`ListManager` for **every** SNode in a non-all-dense tree, place nodes
included, with upstream's own TODO on line 1004 saying so.
`LLVMRuntime::create<T>` (`runtime.cpp:606-612`) calls
`allocate_aligned(runtime_memory_chunk, sizeof(T), 4096, request=true)`, so the
full 1048616 bytes is committed at that moment, not on demand.
`runtime_NodeAllocator_initialize` (`runtime.cpp:1026-1031`) adds a
`NodeManager` per garbage-collectable SNode, and its constructor
(`runtime.cpp:643-663`) creates three more `ListManager`s at lines 658, 660 and
662-663.

`is_gc_able` is `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`),
called at `llvm_runtime_executor.cpp:447`.

**Conclusion: section 6.1 of the plan is incomplete as written.** Recording the
three arrays at `runtime.cpp:567-569` as "a footprint, not only a bounds check"
is true but names the wrong footprint by three orders of magnitude. The planner
does have to correct it. Both passes are right about that, and they are right
for the same verifiable reason, not by shared misreading.

**But neither pass got the footprint right either.** See sections 2 and 6.

---

## 2. The magnitude disagreement, resolved

Pass A: 1024 MiB. Pass B: up to 4 GiB, and 512 pointer SNodes exhaust a 2 GB
card. The brief says both cannot be right.

**They are not actually in contradiction.** They are answering different
questions and neither says so plainly.

- Pass A section 2.4 computes `1024 * 1048616 = 1073782784` for **element lists
  only**. Arithmetically correct: 1024.02 MiB.
- Pass B section 2.4 computes the same figure (1.00 GiB) *and* the
  all-`pointer` case, `1024 * 4194520 = 4295188480`, 4.00 GiB. Also
  arithmetically correct.
- Pass A states the same 4 MiB per garbage-collectable SNode in its own
  arithmetic block and simply never multiplies it out.

So B is a superset of A on this point, not a contradiction. **B's number is the
one to keep**, because the brief requires sparsity (section 4.1) and `pointer`
is the sparse node type that carries a `NodeManager`.

Two corrections to both:

**(a) Both omit the bump allocator's alignment slack, so both are ~0.4% low.**
`allocate_from_reserved_memory` (`runtime.cpp:838-874`) aligns each request to
4096 by advancing the head past the padding (lines 848-855). `1048616 mod 4096
= 40`, so every `ListManager` after the first consumes 1052672 bytes, exactly
257 pages. `NodeManager` at 56 bytes with 4096 alignment consumes a full page.
Corrected figures:

| Item | Bytes consumed from the pool |
|---|---|
| one element list (header only) | 1,052,672 |
| one pointer/dynamic SNode (header only, 4 lists + NodeManager) | 4,214,784 |

Pass A flags the slack qualitatively ("up to 4096 bytes per allocation") and
then does not apply it. Pass B ignores it entirely.

**(b) Both stop at the header and neither prices the first touched chunk,
which is 4 MiB.** `ListManager::touch_chunk` (`runtime.cpp:1664-1679`)
allocates `max_num_elements_per_chunk * element_size`. For element lists that is
`65536 * sizeof(Element)` = `65536 * 64` = **4,194,304 bytes per chunk**
(`runtime.cpp:1006` passes `1024 * 64`; `sizeof(Element) == 64` measured
above). Both reports write "plus the data chunks that are actually touched"
(A section 2.4) and "only the chunks inside it are lazy" (B section 2.4) and
then build their tier claims on the header figure alone.

This matters immediately, not eventually: `runtime.cpp:1016` appends the root
element to `element_lists[root_id]` during `runtime_initialize_snodes`, which
calls `reserve_new_element` → `touch_chunk(0)`. So 4 MiB is committed for the
root's list at materialisation of every sparse tree, before any kernel runs.
Every other element list takes its 4 MiB the first time listgen puts anything
in it.

Populated cost per SNode is therefore **5,246,976 bytes, not 1,048,616** — five
times what either report reports. Pass A's "half of a 2 GB card, before a
single particle" is off by 5x in the direction that matters.

`NodeManager`'s `data_list` is worse: its chunk is
`chunk_num_elements * element_size` where `chunk_num_elements` starts at 16384
(`runtime.cpp:1030` passes `1024 * 16`) and is halved only until the product
drops under the 128 MB cap at `runtime.cpp:653-656`. A single pointer SNode
with a 4 KiB cell therefore takes a 64 MiB chunk on first allocation. Neither
report names this.

**(c) B's "512 pointer SNodes exhausts the card" is wrong on its premise.**
See section 6. The arithmetic (`512 * 4194520 = 2147594240 > 2^31`) is right;
the premise that the whole 2 GB card backs the runtime memory chunk is not.

---

## 3. B's conclusion on open question 8.1.2: half sound

B concludes: *"raising `taichi_max_num_snodes` alone is nearly free in device
memory, and it does not unlock a raised ceiling on the baseline tier because
`sizeof(ListManager)` is what consumes the 2 GB."*

I tried to break both halves.

### 3.1 "Nearly free" — survives

Verified. The marginal cost is 24 bytes per slot, and I checked every path the
struct's size could reach:

- Device: `runtime_get_memory_requirements` (`runtime.cpp:891-906`) rounds
  `sizeof(LLVMRuntime)` to a page (line 897) and the host preallocates that
  plus 256 bytes of result buffer (`llvm_runtime_executor.cpp:675-694`, result
  buffer size at line 681). Linear, no cliff.
- Host/CPU: `runtime_initialize` falls through to
  `host_allocator(memory_pool, sizeof(LLVMRuntime), 128)` (`runtime.cpp:933`).
  I checked whether an oversized request can fail there:
  `UnifiedAllocator::allocate` does
  `allocation_size = std::max(allocation_size, default_allocator_size)`
  (`taichi/rhi/common/unified_allocator.cpp:68`), so a request larger than the
  default chunk simply gets its own chunk. No ceiling.
- Nothing stack-allocates an `LLVMRuntime`. Codegen only ever fetches the type
  by name (`llvm_context.cpp:1005`, called from `codegen_llvm.cpp:2697`).

At N = 65536 the struct is 1.63 MiB, 417 pages. B's table is right.

### 3.2 "Does not by itself unlock the baseline tier" — right answer, wrong reason

The conclusion survives, and in fact survives *more strongly* than B argues,
but B's mechanism is wrong and its number is out by roughly 2x. The binding
limit at the baseline tier is not the card. It is `device_memory_GB`, which
defaults to 1 (`taichi/program/compile_config.cpp:63`), consumed through
`preallocate_runtime_memory` (`llvm_runtime_executor.cpp:607-632`). Full
argument in section 6.

Corrected capacity at the default configuration, 1 GiB pool:

| Case | Slots before the pool is exhausted |
|---|---|
| element list headers only | 1020 |
| element list header + its first 4 MiB chunk | 204 |
| pointer SNode, headers only | 254 |

B's 512 becomes 254 at the default, or ~458 if the user sets
`device_memory_fraction = 0.9` on a 2 GB card. B is out by roughly a factor of
two in the direction that flatters the baseline tier.

### 3.3 Does this answer 8.1.2?

**No, and it should not be allowed to stand as one.** It narrows the question
usefully and it is directionally right, but the sizing rule it implies is built
on a number that is 5x low (header only, section 2b) against a pool it never
identifies (section 6). A replacement value for 1024 chosen from B's table as
it stands would be chosen against the wrong constraint.

---

## 4. The claimed leak: verified, and stronger than B states

`LlvmRuntimeExecutor::destroy_snode_tree`,
`taichi/runtime/llvm/llvm_runtime_executor.cpp:758-761`, in full:

```
void LlvmRuntimeExecutor::destroy_snode_tree(SNodeTree *snode_tree) {
  get_llvm_context()->delete_snode_tree(snode_tree->id());
  snode_tree_buffer_manager_->destroy(snode_tree);
}
```

Every element of B's claim checks out, and I can add hard evidence B did not
give:

1. **Nothing clears the device tables.** The only writes to `element_lists`,
   `node_allocators`, `ambient_elements`, `roots` and `root_mem_sizes` in the
   whole tree are `runtime.cpp:996`, `997`, `1005`, `1029` and `1038`, all on
   the create path. The generated setters *do* exist —
   `STRUCT_FIELD_ARRAY(LLVMRuntime, element_lists)` and its three siblings at
   `runtime.cpp:616-619` emit `LLVMRuntime_set_element_lists` and so on — and
   `grep -rn "LLVMRuntime_set_element_lists\|LLVMRuntime_set_node_allocators\|LLVMRuntime_set_roots\|LLVMRuntime_set_root_mem_sizes"`
   over the tree returns **nothing outside the macro that defines them**. There
   is no mechanism by which a slot can be cleared, not merely no call.

2. **No free path.** `allocate_from_reserved_memory` (`runtime.cpp:838-874`)
   only advances `memory_chunk.preallocated_head` (line 853). Confirmed.
   One qualification both reports omit: the pool as a whole is a
   `DeviceAllocationGuard` (`llvm_runtime_executor.cpp:587-604`), so it is
   released at executor teardown. The leak is unbounded within a `Program`'s
   life and reclaimed when the `Program` dies.

3. **`snode_tree_allocs_` is never erased.** B found this and A did not.
   Confirmed: assigned at `llvm_runtime_executor.cpp:440`, read at `:387`,
   declared at `llvm_runtime_executor.h:152`, and those are the only three
   occurrences. Meanwhile `SNodeTreeBufferManager::destroy`
   (`snode_tree_buffer_manager.cpp:20-24`) really does free the underlying
   device allocation and erase its own map entry. So the entry left in
   `snode_tree_allocs_` is a dangling `DeviceAllocation`, and
   `get_snode_tree_device_ptr` (`:385-388`) will hand it out on a recycled tree
   id. That is worse than a leak; B undersells it.

**Reachability — neither report established this, and the brief asked.**
`Program::destroy_snode_tree` (`program.cpp:214-236`) has exactly one caller in
the entire repository: the pybind lambda at
`taichi/python/export_lang.cpp:569-570`, driven from
`python/taichi/_snode/snode_tree.py:21`. There is no C++ test caller and no
internal caller.

So: today the leak is reachable only through the Python front end, which the
brief puts out of scope (section 1.2). It is **not** reachable from the C++
core as it currently stands. It becomes reachable the moment this fork drives
the core from C++ and calls `Program::destroy_snode_tree`, which brief section
4.6 makes likely, since "composed from the larger data stores for working
scope" implies recomposition. Both reports treat it as a live hazard without
checking; the honest statement is that it is a latent one that this project's
own architecture will activate.

---

## 5. Spot-check of B's self-audit

B claims it re-extracted every citation and corrected roughly thirty, including
five larger ones from concatenated file numbering (`notes-03b-runtime-struct.md`
N25).

**The audit was real and it worked on the big items.** I checked every one of
the five concatenation corrections and B's report carries the corrected value:

| B's corrected citation | Source says |
|---|---|
| `struct/struct.h:11,12` | `std::vector<SNode *> snodes;` / `std::size_t root_size{0};` — correct |
| `struct/struct.cpp:7-13` | `collect_snodes` — correct |
| `snode_tree.cpp:22-32` | `check_tree_validity` — correct |
| `snode_tree.cpp:34-39` | `get_snodes_to_root_id` — correct |
| `ir/snode.h:98` | `int chunk_size{0};` — correct |

I also re-checked about forty other B citations at random across
`runtime.cpp`, `node_pointer.h`, `node_root.h`, `node_dynamic.h`,
`llvm_offline_cache.h`, `llvm_context.h`, `program.h`, `gfx/runtime.h`,
`gfx/snode_tree_manager.{h,cpp}`, `snode_tree.h`, `context.h`,
`rhi/llvm/device_memory_pool.h` and `export_lang.cpp:1222`. All correct.

**Residual drift remains, in one file the audit did not cover.** Six citations
into `taichi/runtime/program_impls/llvm/llvm_program.{h,cpp}` are off by one to
four lines:

| B says | Source |
|---|---|
| `llvm_program.cpp:58-64` | 58-65 |
| `llvm_program.cpp:66-75` | 67-76 |
| `llvm_program.cpp:93-119` | 97-122 |
| `llvm_program.h:190` (enclosing function) | 188 |
| `llvm_program.h:191` (the FIXME) | 189 |
| `llvm_runtime_executor.cpp:257-269` | 257-270 |

None changes a conclusion. The audit was thorough where it was applied and did
not reach this file.

---

## 6. What both reports missed

This is the section that matters. The territory owns the ceiling and neither
pass reached the bottom of it.

### 6.1 The 1 GiB preallocation pool — the actual binding constraint

Neither report contains the word `preallocate_runtime_memory`,
`device_memory_GB` or `device_memory_fraction`.

`LlvmRuntimeExecutor::preallocate_runtime_memory`,
`taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632`:

```
std::size_t total_prealloc_size = 0;
const auto total_mem = llvm_device()->get_total_memory();
if (config_.device_memory_fraction == 0) {
  TI_ASSERT(config_.device_memory_GB > 0);
  total_prealloc_size = std::size_t(config_.device_memory_GB * (1UL << 30));
} else {
  total_prealloc_size = std::size_t(config_.device_memory_fraction * total_mem);
}
TI_ASSERT(total_prealloc_size <= total_mem);
```

and the defaults, `taichi/program/compile_config.cpp:63-64`:

```
device_memory_GB = 1;  // by default, preallocate 1 GB GPU memory
device_memory_fraction = 0.0;
```

That buffer is handed to `runtime_initialize_memory` (`runtime.cpp:962-971`),
which is what sets `runtime_memory_chunk`. **Every `ListManager`, every
`NodeManager`, every ambient element and every touched chunk is cut from this
1 GiB**, not from the card. Exhaustion is not a card OOM; it is a hard
`__assertfail` inside the kernel at `runtime.cpp:864-871`, text
`"Out of CUDA pre-allocated memory."`.

Consequences for open question 8.1.2, none of which appear in either report:

1. **At the default configuration the pool exhausts at ~1020 element list
   headers, which is *below* the 1024 constant.** Cumulative consumption after
   k lists is `1048616 + (k-1)*1052672`; that passes `2^30` at k = 1021. Add
   the root's mandatory 4 MiB chunk (`runtime.cpp:1016`) and it is ~1016. The
   1024 constant and the 1 GiB default are, by accident, matched to within
   under one percent.

2. **Therefore raising `taichi_max_num_snodes` alone changes nothing on any
   tier, not just the baseline.** The pool runs out first at *every* tier
   because the pool default does not scale with the card. B's conclusion in
   section 3 above is right, but its reason is wrong and its scope is too
   narrow: this is not a 2 GB-card problem, it is a
   `device_memory_GB`-default problem that applies equally to the 1070 and the
   3060.

3. **The two knobs must move together.** Any answer to 8.1.2 is a pair,
   `(taichi_max_num_snodes, device_memory_GB)`, not a single number. Brief
   section 5 makes configuration an install-time decision, and
   `device_memory_GB` is exactly such a decision. It is currently reachable
   only from Python (`export_lang.cpp:208-210`), which by brief section 1.2 is
   out of scope, so at C++ level it is a hard-coded 1.

4. **The 2 GB baseline card cannot be given much more.** `device_memory_GB` is
   asserted against `get_total_memory()` at `llvm_runtime_executor.cpp:619`,
   and the root buffers, the runtime objects buffer and the CUDA context all
   come out of the same card outside this pool. So the baseline tier's real
   budget is well under 2 GB, and at the corrected populated cost of 5.0 MiB
   per sparse SNode it supports a few hundred, not a thousand.

I am recording this as fact and arithmetic. What value the pair should take is
the planner's, and is in Escalations.

### 6.2 `all_dense` defaults to true, so the whole cost is conditional

`bool all_dense = config_.demote_dense_struct_fors;`
(`llvm_runtime_executor.cpp:402`), and `demote_dense_struct_fors = true` by
default (`taichi/program/compile_config.cpp:18`), forced true again for SPIR-V
archs at `compile_config.cpp:72-73`.

Pass A notes the config coupling but cites line 401 (the line is 402) and does
not state the default. Pass B does not mention the config at all. The default
matters: in a default C++ build **no element lists are allocated whatsoever**
unless a tree contains a node that is not dense, place or root. The entire
footprint discussion is conditional on the sparse machinery brief section 4.1
requires being present. Worth stating explicitly in a corrected report.

A second-order consequence neither report drew: on CUDA with the device memory
pool, `preallocate_runtime_memory` is called from
`llvm_runtime_executor.cpp:411-413` **only when `!all_dense`**. So `all_dense`
gates whether `runtime_memory_chunk` exists at all, not merely how much it
holds. It is load-bearing for allocator validity, not only for footprint.

### 6.3 A latent build fault in the file both reports name as the parameterisation seam

Both reports correctly identify
`taichi/runtime/llvm/runtime_module/CMakeLists.txt` as where an install-time
parameter has to be hung. Neither noticed that line 13 of that file uses the
wrong variable:

```
 3  function(COMPILE_LLVM_RUNTIME rtm_arch)
 8      COMMAND ... -o "runtime_${rtm_arch}.bc" ...
13      install(FILES ".../runtime_${arch}.bc" DESTINATION ...)
29  foreach(arch IN LISTS HOST_ARCH CUDA_ARCH DX12_ARCH AMDGPU_ARCH)
30    compile_llvm_runtime(${arch})
```

The build target is named from the function parameter `rtm_arch`; the install
rule reads `${arch}`, which is not a parameter but the caller's `foreach`
variable leaking into the function through CMake's parent-scope read. It
happens to hold the same value at call time, so it works today by coincidence.
Anyone parameterising this function for item 6.1 or 6.3 will trip on it.

### 6.4 A third `inc/constants.h` ceiling in the runtime, unreported by both

`constexpr int taichi_listgen_max_element_size = 1024;`
(`taichi/inc/constants.h:28`), used at `runtime.cpp:1316` and `:1369` to cap
the per-element loop-bound split during listgen, and mirrored in codegen at
`codegen_llvm.cpp:2291`. It is not SNode-count-scaled so it is not a 6.1 item,
but it is a hard 1024 in the struct-for dispatch path and brief section 6.4
makes dispatch throughput the sole criterion. Both "exhaustive fixed-size
array" inventories (A section 7, B section 4.10) miss it because both grepped
for array *declarations* rather than for uses of `inc/constants.h` values.

Also unreported: `taichi_max_num_mem_requests = 1024 * 64`
(`taichi/inc/constants.h:16`) has no use anywhere in the tree. Dead constant.

### 6.5 The sm_60 bitcode concern B raises cannot actually fire

B flags `cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc` as pinned to Pascal
against a Maxwell baseline card. The pinning is real, but the link is gated:
`link_module_with_custom_cuda_library` (`llvm_context.cpp:574-591`, called from
`:506`) first calls `get_custom_cuda_library_path`
(`taichi/util/lang_util.cpp:18-28`), which returns `""` if the file is not
present in `runtime_lib_dir()`. The only `install()` rule for that file is
`runtime_module/CMakeLists.txt:24`, inside `COMPILE_CUSTOM_CUDA_LIBRARY`, whose
only call site is commented out at lines 36-42. So the file never reaches the
install tree and the link never happens. B's flag should be downgraded to a
note.

---

## 7. Every other divergence between the two reports, resolved

### 7.1 Pass A's citations into two files are systematically wrong

This is A's one serious fault. A read `taichi/struct/snode_tree.{h,cpp}` and
`taichi/runtime/gfx/snode_tree_manager.{h,cpp}` as concatenations and recorded
the concatenated numbering. B hit the same trap, caught it in its own audit,
and corrected it. A did not.

| A says | Source is | Offset |
|---|---|---|
| `snode_tree.cpp:71-74` (constructor) | `snode_tree.cpp:17-20` | +54 |
| `snode_tree.cpp:76-86` (`check_tree_validity`) | `:22-32` | +54 |
| `snode_tree.cpp:88-93` (`get_snodes_to_root_id`) | `:34-39` | +54 |
| `snode_tree_manager.cpp:54-59` (`materialize_snode_tree`) | `:11-16` | +43 |
| `snode_tree_manager.cpp:58` (push_back order) | `:15` | +43 |
| `snode_tree_manager.cpp:61-72` (`destroy_snode_tree`) | `:18-29` | +43 |
| `snode_tree_manager.cpp:74-90` (`get_field_in_tree_offset`) | `:31-47` | +43 |
| `snode_tree_manager.cpp:74, 92` (the two accessors) | `:31, 49` | +43 |

`snode_tree.cpp` is 41 lines and `snode_tree_manager.cpp` is 54 lines, so
several of A's citations point past end of file. The offsets are exactly the
lengths of the two headers, 54 and 43 lines respectively. A's *content* is
right in every case; only the addresses are wrong. **B is correct on all eight
and A must be corrected.**

### 7.2 Macro line numbers

A: `runtime.cpp:68-76` and `82-87`. B: `:69-77` and `:85-88`.
Source: `#define STRUCT_FIELD_ARRAY` at 69, closing `};` at 77;
`#define RUNTIME_STRUCT_FIELD_ARRAY` at 85, closing at 88. **B correct.**

### 7.3 The 32-bit multiply in `element_listgen_root`

A: "line 1316 `c * ch_element_size < ch_num_elements` is an int multiply".
B: `:1322` and `:1323`.
Source: line 1316 is
`std::min(ch_num_elements, taichi_listgen_max_element_size)`; the comparison is
at 1319 and the two multiplies are at 1322 and 1323. **B correct, A wrong by
three to seven lines and quoting the wrong statement.**

### 7.4 `gc_parallel_*` line numbers — not a real disagreement

A cites 1723/1740/1784 in its read list and 1721/1738/1782 in its width
inventory; B cites 1721/1738/1782. Both are right: 1721/1738/1782 are the
function signatures, 1723/1740/1784 are the `node_allocators[snode_id]` reads
inside them. No conflict.

### 7.5 `set_arg_ndarray_impl`

A: "306-327 ... at line 324". Source: the function is
`launch_context_builder.cpp:308-330` and the `(int32)shape[i]` truncation is at
line 326. **A wrong by two, B does not cover this file.**

### 7.6 Coverage each pass has and the other lacks

Found only by A, and verified by me:

- **`Ndarray::nelement_` overflows in `int`.** `taichi/program/ndarray.cpp:35-38`
  and `:75-78` pass the literal `1` as the `std::accumulate` init, so the
  accumulator is `int` over a `std::vector<int> shape`, and the result is then
  stored in a `std::size_t` (`ndarray.h:80`) and multiplied for the device
  allocation at `ndarray.cpp:61`. The identical idiom at `:52-54` and `:95-97`
  correctly uses `1LL`. Real defect, cleanly identified. B does not cover
  `ndarray.*` at all.
- **`Program::destroy_snode_tree` never resets `snode_trees_[id]`**
  (`program.cpp:214-236`), so the `SNodeTree` and its `SNode` objects survive
  until the slot is reused at `:249`. Verified. B misses it.
- The `taichi/runtime/{cpu,cuda,amdgpu}` and `internal_functions.h` negative
  sweep (A section 7). I checked `internal_functions.h:84` and `:119`,
  `taichi/runtime/cuda/jit_cuda.cpp:28-29` and
  `taichi/runtime/amdgpu/kernel_launcher.cpp:8`. All correct.

Found only by B, and verified by me:

- **`snode_tree_allocs_` is never erased** (section 4 above). Material.
- The offline-cache serialisation consequence of widening ids:
  `llvm_offline_cache.h:68` with `TI_IO_DEF` at `:73`, and `:76-77` with
  `TI_IO_DEF` at `:81`. Verified. This is a genuine constraint on item 6.2 that
  A lists the fields for but never draws the conclusion from.
- `node_root.h:21-23` returning a literal 1 and `node_dynamic.h:116-119`
  returning `node->n`, completing A's partial `get_num_elements` inventory.
  Verified.
- `llvm_runtime_executor.cpp:188-209`: the host queries
  `ListManager_get_element_size` and `_get_max_num_elements_per_chunk` as
  `int32` while both are `std::size_t` on the device (`runtime.cpp:429-430`).
  A real existing host/device width mismatch that A lists the device side of
  and does not connect.

### 7.7 Where they agree and are both right

I re-verified all of these and found no fault:

- Exactly eight occurrences of the two constants, at
  `constants.h:12,13`, `struct_llvm.cpp:266` and `runtime.cpp:562,563,567,568,569`.
  My own `grep -rn` over `.cpp .h .py .txt .cmake` excluding `build/` returns
  precisely that set.
- The assertion at `struct_llvm.cpp:266` bounds `StructCompiler::snodes`
  (`struct.h:11`), filled per tree by `collect_snodes` (`struct.cpp:7-13`) from
  the single root passed to `StructCompilerLLVM::run` (`struct_llvm.cpp:247`),
  with a fresh compiler per tree (`llvm_program.cpp:45-56`); while the arrays
  are indexed by the program-global `SNode::id` (`ir/snode.cpp:12`, `:220`),
  reset only in the `Program` constructor (`program.cpp:144`), with
  `SNode::reset_counter()` (`ir/snode.h:348-350`) having zero callers — I
  confirmed the only `reset_counter` call in the tree is `Stmt::reset_counter`
  at `program.cpp:347`, a different class. Both passes are right and this is
  the sharpest structural finding either produced.
- `kMaxNumSnodeTreesLlvm` has no bound check; `Program::allocate_snode_tree_id`
  (`program.cpp:559-567`) returns `snode_trees_.size()` or a recycled id with no
  comparison.
- The contiguity assumption at `runtime.cpp:1003` versus the actual-id loops at
  `llvm_runtime_executor.cpp:446-468`.
- The gfx path has no fixed-size SNode table, and `compiled_snode_structs_` /
  `root_buffers_` are indexed by push-back order while `tree_id` is recycled.
- The host carries no `LLVMRuntime` definition (`program/context.h:11`,
  `rhi/llvm/llvm_device.h:8` are both bare forward declarations) and codegen
  fetches the type by name, so raising the constant is layout-safe provided the
  `.bc` and the host binary are built from the same value.
- The `int` pointer arithmetic at `node_pointer.h:44,45,69,70,86,92`.
- The `2^33` chunk-table capacity against an `i32 num_elements` counter.

---

## 8. Escalations

Unresolved, requiring a decision the project owner has not made. I am not
choosing any of them.

**X1 — The replacement for 1024 is a pair, not a number.**
Section 6.1. `taichi_max_num_snodes` and `device_memory_GB`
(`compile_config.cpp:63`) bind at almost exactly the same point today. Raising
one without the other is inert. Whether `device_memory_GB` becomes part of the
install-time parameterisation in item 6.1, or is separate work, is the
planner's call.

**X2 — `ListManager::max_num_chunks` and the element-list chunk size.**
`runtime.cpp:427` and the `1024 * 64` at `runtime.cpp:1006` together set the
1 MiB header and the 4 MiB first-touch chunk. Both reports flag the header;
neither flags the chunk. Whether either is in scope for 6.1 is undecided. I am
not proposing a change to either and per standing instruction 3 I am not
calling either surplus.

**X3 — Whether the C++ core will call `Program::destroy_snode_tree`.**
Section 4. Today the only caller is the Python binding, which brief 1.2 puts
out of scope. Whether this fork's C++ driver will destroy and recompose trees
determines whether the stale-table and dangling-`DeviceAllocation` findings are
live or dormant. Brief 4.6 suggests it will. Not mine to decide.

**X4 — The install-rule variable fault at `runtime_module/CMakeLists.txt:13`.**
Section 6.3. It is latent today and would surface under parameterisation. This
sits in agent 04's build territory; I am flagging it, not chasing it.

**X5 — `taichi_listgen_max_element_size = 1024`** (`constants.h:28`). A third
1024 in the runtime dispatch path that neither pass recorded. Whether it is in
scope is undecided.

**X6 — Host/bitcode value agreement.** Both passes raise it. I add one fact
that sharpens it: the only host-side C++ use of `taichi_max_num_snodes` is the
assertion at `struct_llvm.cpp:266`, which by the per-tree/global-id finding is
not checking the array bound anyway. So a value mismatch between the installed
`.bc` and the host binary has no detection path at all. B's E10 stands.

---

## 9. Divergence from adversary-03-2

`adversary-03-2.md` existed and I read it after writing everything above. It was
written before mine and records that mine did not yet exist.

**We converge on all four verdicts and on the single most important finding.**
Independently and blind, both of us went to
`preallocate_runtime_memory` (`llvm_runtime_executor.cpp:607-632`) and
`device_memory_GB = 1` (`compile_config.cpp:63`), and both of us concluded that
this, not the constant and not the card, is what binds. I confirmed by grep that
neither identifier appears anywhere in either report or either notes file. Both
of us also independently priced the 4 MiB `touch_chunk` allocation that both
passes left as "lazy". Both of us reached: correct with qualification, not
complete, major revision required, pass A's citations into two files must be
regenerated, pass B must withdraw the 512-pointer-SNode sentence.

Where we differ, and what the source says.

### 9.1 The pool capacity arithmetic — the source supports me

Adversary 03-2 section 7.1 computes 1023 element lists into the 1 GiB pool, and
builds a headline on it: *"the existing constant overruns the existing default
by 40,960 bytes — under one page over."*

That drops the bump allocator's alignment padding, which is charged to the
caller. `runtime.cpp:848-853`:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

`1048616 mod 4096 = 40`, so after the first `ListManager` the head is 40 bytes
past a page and every subsequent 4096-aligned request is charged 4056 bytes of
padding. Consumption after k lists is `1048616 + (k-1)*1052672`. At k=1020 that
is 1,073,721,384, inside the 1,073,741,824-byte pool. At k=1021 it is
1,074,774,056, outside it.

**Capacity is 1020, not 1023.** And 1024 lists cost 1,077,932,072 bytes, so the
constant overruns the default pool by 4,190,248 bytes, about four lists' worth,
not "under one page".

The qualitative conclusion we share — that the constant and the default pool are
matched to within about half a percent, so raising one alone buys nothing — is
unaffected and stands. Only 03-2's precision claim fails. Its pointer-SNode row
(255) becomes 254 for the same reason.

### 9.2 Two line numbers where 03-2 is right and I am not

- `TI_ASSERT(total_prealloc_size <= total_mem)` is at
  `llvm_runtime_executor.cpp:620`. I wrote 619 in sections 3.2 and 6.1 above.
  03-2 is correct.
- I wrote the CUDA abort as `runtime.cpp:864-871`. The `__assertfail` call is
  `:864-869`, `#endif` is `:870`, and the `taichi_assert_runtime` fallback is
  `:872`. 03-2's `:860-868` and `:871` are also wrong. The correct pair is
  864-869 and 872.

### 9.3 Two findings 03-2 has that I did not reach

I concede both.

- **Pass A inverts the `all_dense` condition.** 03-2 section 6.1. A writes that
  `all_dense` is "also forced on by `config_.demote_dense_struct_fors`". Reading
  `llvm_runtime_executor.cpp:402-410`, the flag is the seed value and the loop
  can only clear it, so it is a necessary precondition for the skip, never a
  force-on; with the flag false, every tree allocates a `ListManager` per SNode,
  dense trees included. That is a content error in A, not just the line 401/402
  slip I recorded in section 6.2 above. 03-2's reading is correct and sharper
  than mine.
- **The gfx numbering divergence is resolvable and 03-2 resolved it.** Both
  passes escalated it unresolved; I recorded that and did not construct the
  case. 03-2's construction checks out against
  `snode_tree_manager.cpp:11-16`, `:18-29`, `:49-51` and
  `program.cpp:235,559-567`: create trees 0 and 1, destroy 1, create a third.
  The third gets `tree_id == 1` from the free stack while its buffer lands at
  `root_buffers_[2]`, and `root_buffers_[1]` is the `unique_ptr` reset at
  `snode_tree_manager.cpp:28`. `get_snode_tree_device_ptr(1)` then dereferences
  null. It diverges after one destroy-then-add cycle.

### 9.4 Two places 03-2 overstates, where the source supports my narrower reading

- **`ptr2index` is not an O(131072) scan.** 03-2 section 7.4 puts both
  `ptr2index` (`runtime.cpp:502-512`) and `get_num_active_chunks` (`:467-473`)
  on "the sparse hot path" as 131072-iteration loops. `ptr2index` calls
  `taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.")` on
  every iteration, so it terminates at the first untouched chunk: it is O(chunks
  actually touched), and it aborts rather than scanning when the pointer is
  absent. `get_num_active_chunks` genuinely is unconditional over 131072, but
  its only reachable caller is `runtime_ListManager_get_num_active_chunks`
  (`runtime.cpp:743-747`) from `print_list_manager_info`
  (`llvm_runtime_executor.cpp:200-201`), a host debug printer. Neither is a hot
  path in the sense 03-2 implies. The underlying observation — that
  `Pointer_deactivate` reaches a linear chunk scan through
  `recycle` → `locate` → `ptr2index` — is real and worth keeping.
- **Neither report actually states the `128 * 1024` NodeManager default.**
  03-2 section 6.2 says "both reports quote the wrong `NodeManager` chunk
  count" and "both describe the constructor's default of `128 * 1024`". Reading
  report A section 5.4 and report B section 4.4, neither quotes that default at
  all; both list the constructor's parameters and stop. The substance of 03-2's
  point stands and is useful — the only call site overrides it with `1024 * 16`
  at `runtime.cpp:1030`, so `chunk_num_elements` is 16384 and the 128 MB
  halving loop at `:652-655` engages only when `node_size > 8192` — but it is a
  gap in both reports, not an error in them.

### 9.5 Findings only in my analysis

03-2 does not have these; I verified each against the source:

- The install rule at `runtime_module/CMakeLists.txt:13` reads `${arch}`, the
  caller's `foreach` variable, where the target at `:8` reads the function
  parameter `${rtm_arch}`. It works only by parent-scope leak. Section 6.3.
- The generated setters `LLVMRuntime_set_element_lists`,
  `_set_node_allocators`, `_set_roots`, `_set_root_mem_sizes`
  (`runtime.cpp:616-619`) exist and have zero call sites in the whole tree, so
  there is no mechanism to clear a stale slot, not merely no caller. Section 4.
- `taichi_listgen_max_element_size = 1024` (`constants.h:28`), a third 1024 in
  the dispatch path, at `runtime.cpp:1316`, `:1369` and `codegen_llvm.cpp:2291`.
  Missing from both reports' "exhaustive" array inventories and from 03-2.
  Section 6.4.
- `taichi_max_num_mem_requests` (`constants.h:16`) has no use anywhere.
- The sm_60 bitcode concern pass B raises cannot fire: the link is gated on
  `get_custom_cuda_library_path` (`lang_util.cpp:18-28`) finding the file, and
  the only `install()` rule for it (`runtime_module/CMakeLists.txt:24`) sits in
  a function whose sole call site is commented out at `:36-42`. Section 6.5.
- Pass A cites `runtime.cpp:1316` for the 32-bit multiply in
  `element_listgen_root`; line 1316 is the `std::min`, and the multiplies are at
  1322 and 1323. Section 7.3.
- Pass A's macro citations `runtime.cpp:68-76` and `82-87` should be `69-77` and
  `85-88`. Section 7.2.
- Pass A's `set_arg_ndarray_impl` citation `306-327 ... line 324` should be
  `308-330 ... line 326`. Section 7.5.
- `UnifiedAllocator::allocate` does
  `allocation_size = std::max(allocation_size, default_allocator_size)`
  (`rhi/common/unified_allocator.cpp:68`), so the CPU path has no ceiling on
  `sizeof(LLVMRuntime)`. This closes the "nearly free" half of pass B's
  conclusion on the CPU side, which 03-2 tests only on the device side.
- `preallocate_runtime_memory` is called from
  `llvm_runtime_executor.cpp:411-413` **only when `!all_dense`** under the CUDA
  memory-pool path, so `all_dense` gates whether the runtime memory chunk exists
  at all, not merely how much it holds.

### 9.6 Findings only in 03-2's analysis, which I accept

- Pass B's `llvm_runtime_executor.cpp:637` should be `:639`.
- `SNode::SNode(const SNode &)` is `TI_NOT_IMPLEMENTED`
  (`taichi/ir/snode.cpp:230-233`), so ids are consumed exactly once per
  constructed node. Verified; it strengthens the cumulative-ceiling finding.
- `clear_list` (`runtime.cpp:1270-1273`) only zeroes `num_elements`, so touched
  chunks are never returned and the touched set is monotonic for the life of the
  process. Verified; this makes the 4 MiB-per-list figure a floor, not a peak.

### 9.7 Residual citation drift in 03-2 itself

Recorded for symmetry, since we both charged the reports with this. All are
one-line-early references into `runtime.cpp`: `allocate_aligned` `822` should be
`823`; `allocate_from_reserved_memory` `837` should be `838`;
`runtime_initialize_memory` `961` should be `962`; the head advance `852` should
be `853`; the `NodeManager` default `646-649` should be `647-650`. None changes
a conclusion.
