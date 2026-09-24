# Adversary 3, 03-2 — runtime and struct layer, round three

Judging `report-03-runtime-struct.md` (pass A, revision 3) and
`report-03b-runtime-struct.md` (pass B, revision 3) against the source at
`/opt/project/taichi`, after the round-two adversaries
(`adversary2-03-1.md`, `adversary2-03-2.md`) and the amendment pass recorded in
notes entries 29-37 (A) and N43-N47 (B).

Everything below was re-derived by me this round. I recompiled `runtime.cpp`
with my own `static_assert`s for the struct sizes and offsets, re-simulated the
bump allocator from the source text rather than multiplying, ran my own
mechanical bounds check over every line citation in both reports, and printed
the source for every citation I quote. I also measured the development box
where the source could not answer a question the reports left open.

---

## 0. The two verdicts

**CORRECT — pass A: yes. Pass B: yes on every load-bearing claim, with one
false sentence that round two identified and revision 3 did not repair.**

All seven claims put to me reproduce. Every figure in both reports — 35256,
1048616, 24576, 1,052,672, 5,246,976, 4,214,784, 1020, 204, 254, 125, 85,
4,190,248, 1836, 368, 458 — reproduces against my own compilation and my own
simulation of the allocator, not against theirs. Every citation I sampled
lands on the line it claims.

The one exception is pass B section 4.10, whose heading, "Every fixed-size
array sized by an `inc/constants.h` value", is still false. Two arrays are
missing from its list. `adversary2-03-1.md` section 7.1 raised exactly this and
pass B's revision record does not mention it. Section 4.1 below. It changes no
number and I would not hold consensus for it alone, but it is a heading
contradicting the tree, which plan standing instruction 8 names as this
project's recurring fault.

**COMPLETE — no, for both, on one shared gap that is new this round, plus one
gap each.**

Shared and new: the enumeration of who else draws from the 1 GiB pool is short
by one consumer class. Both reports name the SNode tree root buffers, ndarrays
and argpacks. Neither names the **kernel-launch staging buffers** allocated per
launch by `taichi/runtime/cuda/kernel_launcher.cpp:83` and `:93` and
`taichi/runtime/amdgpu/kernel_launcher.cpp:59`, which take the same route into
the same bump allocator, unconditionally on AMDGPU. Both files are inside
territory 03's declared directories, and pass A section 9 lists
`taichi/runtime/{cpu,cuda,amdgpu}/` as swept. Section 4.2.

Pass A alone: it never states the positive CPU consequence for open question
8.1.2. It has every component and stops at the negative scoping. Section 4.3.

Pass B alone: the section 4.10 quantifier above.

**Consensus: not reached, narrowly.** Section 6 lists what remains. It is three
sentences of writing and no new investigation. Separately, section 5 records a
measurement that answers half of escalation E15 and that the planner should
have before sizing item 6.1.

---

## 1. What I verified, and how

| Check | Method |
|---|---|
| Struct sizes and offsets | Piped `runtime.cpp` through `g++ -fsyntax-only` on stdin with my own `static_assert`s. No file created, no source touched. |
| Allocator capacity rows | Python simulation of `allocate_from_reserved_memory` transcribed from `runtime.cpp:845-853`, iterating a real allocation sequence rather than multiplying a per-SNode figure. |
| Citation bounds | Regex extraction of every `` `path:line` `` and `` `path:a-b` `` from both reports, basenames resolved against the tree, each number compared to the real file length. |
| Citation content | `grep -n` on every line I quote. Where my own hand-counted `sed` offsets disagreed with `grep -n`, `grep -n` won; three of my apparent off-by-one findings against the reports evaporated that way and are not reported. |
| The one question the tree cannot answer | Direct query of the CUDA driver API on this box, using the same attribute number the source uses. Section 5. |

The size asserts all pass:

```
sizeof(LLVMRuntime)==35256   sizeof(ListManager)==1048616  sizeof(NodeManager)==56
sizeof(Element)==64          sizeof(PhysicalCoordinates)==48  sizeof(StructMeta)==72
sizeof(RandState)==20
offsetof roots==88  element_lists==8296  node_allocators==16488
ambient_elements==24680  temporaries==32872
```

8296 + 24576 = 32872, so the three SNode arrays are contiguous and occupy bytes
8296 to 32871 as pass A states. The closed form `2488 + 24*N + 16*T` follows
from three 8-byte arrays of N and two 8-byte arrays of T; at N=1024, T=512 it
gives 35256. Pass A holds T at 512 in its scaling table and pass B scales T;
each labels its assumption, so the two tables are not in conflict. I recomputed
every row of both. All correct.

My citation bounds check, on my own extraction:

| Report | Line references | Past end of file | Unresolvable paths |
|---|---|---|---|
| `report-03-runtime-struct.md` | 418 | 0 | 0 |
| `report-03b-runtime-struct.md` | 532 | 0 | 0 |

My totals differ from `adversary2-03-2.md`'s 416 and 504 because the extraction
patterns differ. The conclusion is the same and is now established three times
independently.

---

## 2. The seven claims

### 2.1 Claim 1 — capacity 1020, overrun 4,190,248, alignment billed before the fit test. UPHELD.

The billing order is exactly as both reports state, `runtime.cpp:848-853`:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

The pad is added to `size` at `:850`, the fit test at `:851` is against the
padded size, and the head advances by the padded size at `:853`. So the last
allocation that fits must fit including its own pad, which is what makes 1020
rather than 1021 the answer.

`1048616 mod 4096 = 40`, so from an aligned head the first `ListManager` costs
1,048,616 and every one after it costs 1,052,672, which is 257 pages.

```
cum(1020) = 1048616 + 1019*1052672 = 1,073,721,384  <= 1,073,741,824   fits, slack 20,440
cum(1021) = 1048616 + 1020*1052672 = 1,074,774,056  >  1,073,741,824   does not
cum(1024) = 1048616 + 1023*1052672 = 1,077,932,072  overrun 4,190,248
```

My simulation, iterating the real sequence rather than using the closed form,
returns 1020 and a final head of 1,073,721,384. Both match.

The whole 1 GiB is available to this chunk. `runtime_initialize_memory`
(`runtime.cpp:962-971`) sets `runtime_memory_chunk.preallocated_head` to the
buffer base and the tail to base+size, with no reservation taken off the front.
The `LLVMRuntime` object itself and `temporaries` and `rand_states` come out of
`runtime_objects_chunk`, a different buffer from a different
`preallocate_memory` call (`llvm_runtime_executor.cpp:687-690`). Both reports
have this right; I checked it because it is the one thing that would move 1020.

The base-alignment question is closed correctly.
`DeviceMemoryPool::allocate` (`device_memory_pool.cpp:35-41`) declares an
`alignment` parameter at `:36` and never reads it; `:40` forwards
`allocate_raw_memory(size, managed)` only, which is a bare `CUDADriver::malloc`
at `:67`. Slack after 1020 headers is 20,440 bytes against a largest possible
extra first pad of 4095, so no row moves at any base.

The other capacity rows reproduce by simulation, including the two reports'
different fourth rows, which describe different configurations and are both
right:

| Row | Bytes/SNode | Fit |
|---|---|---|
| element-list header only | 1,052,672 | 1020 |
| header + first 4 MiB chunk | 5,246,976 | 204 |
| pointer SNode, all headers, no chunks | 4,214,784 | 254 |
| pass A: + element chunk + `data_list` at node_size 256 | 12,603,392 | 85 |
| pass B: + element chunk + the two i32 chunks | 8,540,160 | 125 |

The 2 GB card at `device_memory_fraction = 0.9` gives a pool of 1,932,735,283
bytes and 1836 / 368 / 458, reproducing pass B's figures exactly.

### 2.2 Claim 2 — the CPU exemption. UPHELD in full.

Every link holds.

- `preallocate_runtime_memory` has exactly two call sites in the tree. My own
  `grep -rn` returns `llvm_runtime_executor.cpp:413` and `:721` and nothing
  else. Two, derived from the enumeration.
- `:413` sits under `:412`
  `if (config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense)`.
  `:721` sits under `:719` `if (config_.arch == Arch::cuda || config_.arch ==
  Arch::amdgpu)` and `:720` `if (!use_device_memory_pool())`. Both
  architecture-gated.
- `use_device_memory_pool_` is initialised `false` at
  `llvm_runtime_executor.h:162` and assigned in exactly one place,
  `llvm_runtime_executor.cpp:49`, inside the `config.arch == Arch::cuda` branch
  opened at `:39`.
- On CPU neither guard passes, `preallocated_size` stays 0, and
  `allocate_aligned` takes the fall-through at `runtime.cpp:834`,
  `host_allocator(memory_pool, size, alignment)`.
- That reaches `host_allocate_aligned` (`llvm_runtime_executor.cpp:28-32`),
  `HostMemoryPool::allocate` (`host_memory_pool.cpp:20-30`) and
  `UnifiedAllocator::allocate`. On a chunk that cannot serve the request it
  mmaps a fresh one sized `std::max(size, default_allocator_size)`
  (`unified_allocator.cpp:64-69`, the raw `mmap` at `host_memory_pool.cpp:59`).
  I read the whole path looking for a cap and there is none.
- I also checked the CPU branch of `materialize_runtime`
  (`llvm_runtime_executor.cpp:698-700`): `runtime_objects_prealloc_size` stays
  0, so `runtime_initialize` takes `host_allocator(memory_pool,
  sizeof(LLVMRuntime), 128)` at `runtime.cpp:933` and the runtime struct itself
  has no ceiling either. Nothing else on the CPU path scales with SNode count.

So the constant binds alone on CPU and raising it alone works, subject only to
host RAM. Plan section 6.1 states this as fact and the fact is sound. It is
load-bearing for a first-class target and it survives.

One qualification on the word "alone", which neither report needs to change but
which the planner should hold alongside it: the constant has two consumers, the
per-arch bitcode and the host binary's assertion at
`struct_llvm.cpp:266`, and both read the same `constants.h:12`. "Raising it
alone" means moving one header value, not one build artefact. Pass A E7 and
pass B E10 already record that a mismatch between the two has no detection
path.

### 2.3 Claim 3 — `device_memory_GB` is a public field on an exported global. UPHELD.

Every link, by `grep -n`:

- `struct CompileConfig` at `compile_config.h:8`, plain struct, all fields
  public. `device_memory_GB` and `device_memory_fraction` are `float64` at
  `:71-72`. `demote_dense_struct_fors` is `bool` at `:28`.
- `extern TI_DLL_EXPORT CompileConfig default_compile_config;` at
  `compile_config.h:110`, defined at `taichi/util/lang_util.cpp:14`.
- `Program::Program` binds `auto &config = compile_config_;` at
  `program.cpp:74` and copies the global wholesale at `:75`, then
  `config.arch = desired_arch;` at `:76` and `config.fit()` at `:77`.
  `CompileConfig::fit` (`compile_config.cpp:67-76`) touches neither memory
  field and only forces `demote_dense_struct_fors = true` for SPIR-V at `:73`.
- The pybind `def_readwrite`s at `export_lang.cpp:201-202` and `:208-210` bind
  the struct fields, and `m.def("default_compile_config", ...)` at `:264-267`
  hands Python a reference to that same global with
  `py::return_value_policy::reference`. Python is a client, not the owner.

So a C++ driver writes `taichi::lang::default_compile_config.device_memory_GB`
before constructing a `Program`, with no new mechanism. Both fields are
`float64`, so fractional values are legal, as both reports note.

The consequence both reports draw is right and it is the one correction from
round two that changes a planner decision. The pool knob needs no build-time
mechanism; the constant does, because it is baked into per-arch bitcode by a
standalone clang command that inherits no project definitions
(`runtime_module/CMakeLists.txt:8`, driven over the arch loop at `:29-31`).
**The expensive half is the constant.** Plan section 6.1 records this
correctly.

I note for the record that `adversary2-03-1.md` escalation Y2 still carries the
opposite ("its only external setter is the Python binding"), and Y4 the same
for `demote_dense_struct_fors`. Both reports and `adversary2-03-2.md` are
right and Y2 and Y4 are wrong. The reports are the artefact that matters, so
nothing needs repairing, but an arbiter reading Y2 alongside plan 6.1 would see
a contradiction that is already resolved against Y2.

### 2.4 Claim 4 — root buffers inside the pool, and the restraint on the tier. UPHELD; the restraint was correct.

I read all eight hops rather than accepting the chain:

| # | Site | Verified |
|---|---|---|
| 1 | `llvm_runtime_executor.cpp:419-420` | `snode_tree_buffer_manager_->allocate(rounded_size, tree_id, result_buffer)` |
| 2 | `snode_tree_buffer_manager.cpp:15` | `runtime_exec_->allocate_memory_on_device(size, result_buffer)` |
| 3 | `llvm_runtime_executor.cpp:476-491` | `allocate_memory_runtime`, `use_device_memory_pool()` passed as the last field at `:485` |
| 4 | `cuda_device.cpp:50-78` | branch at `:56`; true takes `malloc_async` `:57-58`, false takes `allocate_with_cache` `:60-61` |
| 5 | `device_memory_pool.cpp:27-33` | `allocator_->allocate(device, params)` at `:32` |
| 6 | `allocator.cpp:33-58` | cache miss calls `allocate_llvm_runtime_memory_jit` at `:54-55` |
| 7 | `cuda_device.cpp:80-90` | JIT-calls `runtime_memory_allocate_aligned` at `:82-84` |
| 8 | `runtime.cpp:879-886` | `allocate_aligned(runtime->runtime_memory_chunk, size, alignment)` at `:884-885` |

`AmdgpuDevice::allocate_memory_runtime` (`amdgpu_device.cpp:55-78`) has no
`use_memory_pool` branch and calls `allocate_with_cache` unconditionally at
`:63`, reaching the same JIT hop at `:80-90`. Since the flag is written only on
the CUDA branch, on AMDGPU the root buffers always come out of the pool.
`CpuDevice::allocate_memory_runtime` (`cpu_device.cpp:44-51`) calls
`allocate_memory` at `:47` and never reaches that chain.

Ndarray (`ndarray.cpp:61`) and argpack (`argpack.cpp:18`) take the same route
through `Program::allocate_memory_on_device` (`program.h:245-248`). Verified.

The path-conditional "freed" note in pass A section 5.5 is also right:
`CudaDevice::dealloc_memory` (`cuda_device.cpp:92-117`) takes `mem_free_async`
at `:107-108` only when `info.use_memory_pool` is set, and otherwise falls to
`:109-111` because `info.use_cached` is assigned unconditionally at `:68`. That
returns the block to the caching allocator's free list, never to the bump head.

**On the restraint. It was correct.** The gate is
`supports_mem_pool()`, set at `cuda_context.cpp:53` only when the test computed
at `:36-50` passes: `cuDriverGetVersion` major/minor 11.2 or newer (`:36-37`)
and the device reporting `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`
(`:38-40`), with the flag forced 0 at `:49` otherwise. Neither input is
recorded anywhere in the repository. Extending the finding to a named tier from
the tree alone would have been extrapolation, which plan standing instruction 2
forbids and which instruction 6 says a citation cannot certify. Both agents
were right to stop, and pass B was right to reject
`adversary2-03-2.md`'s "on exactly the baseline-tier hardware" as not following
*from this tree*.

What follows from the tree is exactly what both reports state: unconditional on
AMDGPU, and unconditional on CUDA below driver 11.2 or without the attribute.
That is correct and it is where a source investigation should stop.

The question is nevertheless answerable, and I answer it in section 5, because
the answer is not in the tree but is on the desk.

### 2.5 Claim 5 — `ptr2index` is arch-dependent and unbounded on CPU. UPHELD.

`ListManager::ptr2index` is `runtime.cpp:502-512`. The loop runs to
`max_num_chunks` = 131072 (`:427`). Line `:505` is
`taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.")`,
inside the loop and before the range test at `:506`. `enable_assert` is
`constexpr bool ... = true` at `:339`.

`taichi_assert_runtime` (`:817-819`) forwards to `taichi_assert_format`
(`:766-815`), which returns at `:780-781` when the test passes, records the
error once under `if (!runtime->error_code)` at `:782`, and then kills the
thread only under `#if ARCH_cuda` (`asm("exit;")` at `:800`) or
`#elif ARCH_amdgpu` (`asm("S_ENDPGM")` at `:802`), with `#endif` at `:814` and
a normal return at `:815`. There is no CPU arm.

The host bitcode is built with `-D "ARCH_${rtm_arch}"`
(`runtime_module/CMakeLists.txt:8`) over `HOST_ARCH`
(`:29`), and `HOST_ARCH` is `ARCH` from `cmake/TaichiCXXFlags.cmake:149`, which
is `"x64"` on x86_64 (`:139`) and `"arm64"` on aarch64 (`:142`). Neither define
compiles a kill. So on the CPU build control returns to the loop, `:506` tests
`nullptr <= ptr` (true) and `ptr < nullptr + chunk_size` (false), no iteration
matches, none breaks, and all 131072 run before `:511` returns -1.

Both reports state this correctly and identically. Reached from
`Pointer_deactivate` (`node_pointer.h:67-82`) and `Dynamic_deactivate`
(`node_dynamic.h:43-59`) via `NodeManager::recycle` (`runtime.cpp:683-686`) and
`locate` (`:679-681`).

### 2.6 Claim 6 — the destroy path. UPHELD, all three parts.

**Four generated setters, zero call sites.** `STRUCT_FIELD_ARRAY`
(`runtime.cpp:69-77`) is applied at `:616-619` to `element_lists`,
`node_allocators`, `roots` and `root_mem_sizes`. Four, derived from that list.
`ambient_elements` gets no accessor pair at all; its only two occurrences in the
file are the declaration at `:569` and the write at `:1038`. My own grep for
each of `LLVMRuntime_set_element_lists`, `_set_node_allocators`, `_set_roots`
and `_set_root_mem_sizes` across `.cpp`, `.h`, `.py` and `.txt`, build
excluded, returns zero hits each, including as string literals, which is the
form a JIT-by-name caller would take. Pass A's stronger phrasing, that there is
no mechanism by which a slot can be cleared, is earned.

**No free path.** `preallocated_head` appears at `runtime.cpp:547, 845, 849,
851, 852, 853, 938, 967`. The only modification is `+= size` at `:853`. There is
no decrement and no free function.

**The dangling allocation.** `snode_tree_allocs_` has exactly three occurrences
tree-wide: declared `llvm_runtime_executor.h:152`, written `:440`, read `:387`.
Nothing erases it. `SNodeTreeBufferManager::destroy`
(`snode_tree_buffer_manager.cpp:20-24`) does free the underlying allocation and
does erase its own map entry, so `get_snode_tree_device_ptr` (`:386-389`)
returns a stale `DeviceAllocation` on a recycled tree id. `Program::
destroy_snode_tree` (`program.cpp:214-236`) recycles the id at `:235` and
`allocate_snode_tree_id` (`:559-567`) hands it back out.

Reachability is as both reports state. `Program::destroy_snode_tree` has
exactly one caller, the pybind lambda at `export_lang.cpp:569-570`, driven from
`python/taichi/_snode/snode_tree.py:21`. No C++ core caller, no test caller.
Plan section 4.8 already weights this correctly and I have nothing to add to
that weighting.

The gfx divergence in pass A 7.2 and pass B 3.4 also reproduces. I re-derived
it from the four functions: `materialize_snode_tree`
(`snode_tree_manager.cpp:11-16`) only push_backs, `destroy_snode_tree`
(`:18-29`) resets `root_buffers_[root_id]` at `:28` without shrinking either
vector, `Program` recycles ids, and `get_snode_tree_device_ptr` (`:49-51`)
indexes `root_buffers_` by the recycled id. Divergence after one
destroy-then-add cycle. Closing it as a finding rather than an escalation was
correct.

### 2.7 Claim 7 — citation repair and self-audit. UPHELD.

Pass A's section 5 regeneration is sound. `snode_tree.h` is 54 lines with
`kFirstID` at `:17`, `id_` at `:40`, `root_` at `:41` and
`get_snodes_to_root_id` at `:52`. `snode_tree.cpp` is 41 lines with the
anonymous-namespace impl at `6-13`, the constructor at `17-20`,
`check_tree_validity` at `22-32` and `get_snodes_to_root_id` at `34-39`.
`struct.h` is 26 lines with `stack` at `:10`, `snodes` at `:11` and `root_size`
at `:12`. `struct.cpp` is 15 lines. Every one of those is exactly as pass A
now states, and the +54 fault is gone.

I sampled roughly forty further citations across both reports by printing the
source, weighted to lines neither round-two adversary listed as sampled. All
land. Specifically checked and correct: `runtime.cpp:606-612` (`create<T>`,
alignment 4096 and `request=true` at `:608-609`), `:643-664` (the `NodeManager`
constructor, the `128*1024` default inside the `if` at `:648-650`, the 128 MB
halving loop at `:652-655`, the three lists at `:658-659`, `:660-661`,
`:662-663`), `:1000-1007` (the `all_dense` early return and the element-list
loop, `1024*64` at `:1006`), `:1016`, `:1026-1031` (the `1024*16` override at
`:1030`), `:1033-1040` (alignment 128 at `:1038-1039`), `:1435`, `:1552`,
`:1636` (the three fixed-extent `tls_buffer`s pass A added) with `:1412`,
`:1487`, `:1554`, `:1581`, `:1638` correctly excluded as variable-length,
`llvm_runtime_executor.cpp:442-444`, `:446-468`, `:687-690`,
`llvm_program.cpp:113` (the corrected `snode_cache_data.id` line),
`ndarray.cpp:37`, `:53`, `:61`, `:77`, `:96`,
`cuda_context.cpp:36-37`, `:38-40`, `:49`, `:53`,
`amdgpu_device.cpp:63`, `cpu_device.cpp:44-51`, `unified_allocator.cpp:64-69`,
`snode_tree_manager.cpp:11-16`, `:18-29`, `:31-47`, `:49-51`,
`CMakeLists.txt:8`, `:13`, `:29-31`, `:36-42`.

The `${rtm_arch}` / `${arch}` fault at `runtime_module/CMakeLists.txt:13` is
real and is exactly as both reports describe.

Pass B's self-audit claim, zero references past end of file, holds on my own
extraction as well. Three independent audits now agree.

The corrected listgen spans are right: `element_listgen_root` closes at
`runtime.cpp:1329` and `element_listgen_nonroot` at `:1383`. Pass A had these
right in revision 2; pass B corrected to them in revision 3.

---

## 3. Numbers I derived from my own enumeration

Per plan standing instruction 8, each of these is the count of the list beneath
it, not a total written alongside one.

**Occurrences of the two ceiling constants: eight.**
`constants.h:12`, `constants.h:13`, `struct_llvm.cpp:266`, `runtime.cpp:562`,
`:563`, `:567`, `:568`, `:569`. One `grep -rn` over the tree with `build/` and
`modernization/` excluded, matching `.cpp`, `.h`, `.py`, `.txt`, `.cu` and
`.inc.h`. Nothing in tests, `c_api/` or CMake. Pass A's "exactly eight" is
right and pass B's five rows for `taichi_max_num_snodes` are the same
enumeration restricted to that constant.

**Arrays in the tree sized by an `inc/constants.h` value: ten declarations.**
`ir/snode.h:78`, `:81`; `runtime.cpp:289`, `:562`, `:563`, `:567`, `:568`,
`:569`, `:587`, `:588`. External submodules and `tests/` excluded. Pass B's
section 4.10 lists eight of the ten. Section 4.1 below.

**Call sites of `preallocate_runtime_memory`: two.**
`llvm_runtime_executor.cpp:413` and `:721`, plus the declaration at
`llvm_runtime_executor.h:105` and the definition at `:607`.

**Generated setters over the SNode-indexed arrays: four.**
`LLVMRuntime_set_element_lists`, `_set_node_allocators`, `_set_roots`,
`_set_root_mem_sizes`, from `STRUCT_FIELD_ARRAY` applied at
`runtime.cpp:616-619`. Zero call sites each.

**Host-side entries into the pool through `allocate_memory_on_device`: four
classes in the core, plus tests.**
SNode tree root buffers (`snode_tree_buffer_manager.cpp:15`), ndarrays
(`ndarray.cpp:61`), argpacks (`argpack.cpp:18`), and kernel-launch staging
buffers (`cuda/kernel_launcher.cpp:83` and `:93`,
`amdgpu/kernel_launcher.cpp:59`). The AOT tests under `tests/cpp/aot/llvm/`
call it directly as well. Both reports carry three of the four core classes.

---

## 4. Newly wrong, or still missing

### 4.1 Pass B section 4.10's heading is still false, and round two already said so

The heading is "Every fixed-size array sized by an `inc/constants.h` value" and
the section lists five bullet rows covering seven declarations. Two are
missing:

```
runtime.cpp:587  char error_message_template[taichi_error_message_max_length];
runtime.cpp:588  uint64 error_message_arguments[taichi_error_message_max_num_arguments];
```

Both constants are `inc/constants.h:18` and `:19`. The disclosed grep,
`\[taichi_max_num\|\[kMaxNum`, cannot match them. Revision 3 corrected the word
"exhaustive" for two *bound* uses that a declaration-shaped grep misses, which
is a different fault, and left the quantifier in the heading untouched.
`adversary2-03-1.md` section 7.1 named this precisely and pass B's revision
record does not mention it.

Nothing downstream moves: neither array scales with SNode count and pass A
carries both in its sections 2.2 and 9. The repair is to narrow the heading to
the grep, or to add the two rows. I would not hold consensus for it alone, but
it is the fault class plan standing instruction 8 exists to stop, and it is now
the second round in which it has been reported unfixed.

### 4.2 Neither report enumerates the kernel-launch staging buffers as pool consumers

This is the one gap I found that is new this round and that both reports share.

`taichi/runtime/cuda/kernel_launcher.cpp:83-84` and `:93-95`, and
`taichi/runtime/amdgpu/kernel_launcher.cpp:59-60`, allocate a device staging
buffer per kernel launch for every external array argument whose host pointer
is not already device memory:

```
cuda/kernel_launcher.cpp:83   DeviceAllocation devalloc = executor->allocate_memory_on_device(
cuda/kernel_launcher.cpp:84       arr_sz, (uint64 *)device_result_buffer);
```

That is the same `LlvmRuntimeExecutor::allocate_memory_on_device`
(`llvm_runtime_executor.cpp:476-491`) that both reports trace for the root
buffers, so it reaches the same bump allocator by the same hops whenever
`use_device_memory_pool()` is false. On AMDGPU that is unconditional, because
`AmdgpuDevice::allocate_memory_runtime` has no branch on the flag.

They are released after the launch, `cuda/kernel_launcher.cpp:184` and
`amdgpu/kernel_launcher.cpp:147`, through `deallocate_memory_on_device`
(`llvm_runtime_executor.cpp:493-498`). On the non-mem-pool path that is the
same `info.use_cached` route pass A section 5.5 documents for root buffers:
the block goes onto the caching allocator's free list and is reusable, but the
bump head at `runtime.cpp:853` never moves back. So the pool's high-water mark
includes them.

Why it matters, and why it is not a detail. Escalation E15 in both reports is
the statement that the 1020 / 204 / 254 / 125 / 85 rows are upper bounds
because the pool's denominator is shared. That escalation is the input to the
sizing rule for item 6.1. Its list of sharers is short by a class that scales
with launch traffic and argument size rather than with SNode count, which is a
different shape of consumer from the three already listed. Both files are
inside the territory's declared directories, and pass A section 9 records
`taichi/runtime/{cpu,cuda,amdgpu}/` as swept and clear.

I am recording the mechanism. I am not estimating what it costs, and per
standing instruction 3 I call nothing surplus.

### 4.3 Pass A never states the positive CPU consequence for open question 8.1.2

Pass A has every component. Section 3.2 carries the scope limit, "The 1 GiB
ceiling is a CUDA and AMDGPU fact, not a universal one", with the fall-through
to `host_allocator` cited. Section 2.2 records that the host allocator has no
ceiling, though for the runtime struct rather than for the `ListManager`s.

It never joins them. Every statement of the conclusion in pass A is the
negative scoping: summary item 4, "raising `taichi_max_num_snodes` alone is
inert on CUDA and AMDGPU"; section 3.3, "changes nothing on any CUDA tier";
escalation E1, "raising one alone is inert on CUDA and AMDGPU". A grep of the
report for "alone" returns three hits and none of them is a CPU statement.

`adversary2-03-2.md`'s remaining item 6 asked both reports to state the CPU
consequence positively. Pass B did, in section 2.6 and escalation E5, in the
form plan section 6.1 now quotes. Pass A did not.

The plan is not harmed, because it took the sentence from pass B and from both
round-two adversaries, and section 2.2 above confirms the sentence is true. But
the answer to an open question in section 8.1 should not depend on which of the
two reports the reader opens, and the pair is supposed to converge.

### 4.4 Minor, recorded and not held against either report

- Pass B's charge table row giving the `NodeManager`'s charge as 4,096 still
  contradicts its own prose at 2.5: from an aligned head a 56-byte request at
  alignment 4096 pays no padding, and the page is consumed by the next request.
  `adversary2-03-2.md` section 2 raised it as cosmetic. I confirmed by
  simulation that the per-SNode total 4,214,784 is unaffected: the true
  sequence charges 4,112 for the `NodeManager` and 1,052,656 for the following
  list, offsetting by 16 bytes against the table's 4,096 and 1,052,672.
- Pass A did not adopt pass B's 64 MiB `data_list` example
  (`16384 * 4096` at a 4 KiB cell), which `adversary2-03-1.md` 7.4 suggested.
  Pass A gives the formula and prices a 256-byte cell. Both are correct; only
  pass B makes the largest single allocation visible. Discretionary.
- Neither report mentions the early-return guard at
  `llvm_runtime_executor.cpp:608-609`,
  `if (preallocated_runtime_memory_allocs_ != nullptr) return;`, which makes
  `preallocate_runtime_memory` idempotent. It supports rather than disturbs
  both reports' four-row arch table, and I record it only so the next reader
  does not think the pool can be created twice.

---

## 5. The measurement neither report could take from the tree, and I could take from the box

Section 2.4 rules that declining to map the memory-pool gate onto the tiers in
plan 5.2 was correct **as a source-reading judgement**. It leaves E15's first
sub-question open: which branch the GTX 750, the GTX 1070 and the RTX 3060
take.

Plan section 5.1a and section 8.2.0 establish that direct measurement of the
development box is admissible in this project and outranks a code-reading
conclusion. Both territory 05 adversaries measured this box for the 64-bit
guard. I therefore measured this one, using the same driver API and the same
attribute number the source uses
(`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED = 115`,
`taichi/rhi/cuda/cuda_driver.h:38`), through `libcuda.so.1` directly. No file in
the project was created or touched; the probe lives in the session scratchpad.

```
driver CUDA version 12.2
dev 0: NVIDIA GeForce GTX 1070   sm_61  MEMORY_POOLS_SUPPORTED=1
dev 1: NVIDIA GeForce GTX 750 Ti sm_50  MEMORY_POOLS_SUPPORTED=0
```

`nvidia-smi` reports driver 535.309.01 for both. `CUDADriver` derives its major
from `cuDriverGetVersion` divided by 1000 (`cuda_driver.cpp:140`), so the
`> 11` test at `cuda_context.cpp:36` passes and the attribute is queried, which
is exactly the path the source takes.

Consequences, stated only as far as the measurement carries:

- **Baseline tier, GTX 750 Ti.** `supports_mem_pool()` is false. Therefore
  `use_device_memory_pool()` is false, the `:721` call site fires, the pool is
  created eagerly in `materialize_runtime`, and the SNode tree root buffers,
  ndarrays, argpacks and the staging buffers of section 4.2 are all cut from
  the same 1 GiB bump allocator as the `ListManager`s. **Every capacity row in
  both reports is an upper bound on the baseline tier, not a budget.**
- **Mid tier, GTX 1070.** `supports_mem_pool()` is true. The `:413` call site
  is the only one that can fire, so the pool exists only once a non-all-dense
  tree is materialised, and root buffers take `malloc_async`, genuinely outside
  it. On that card the capacity rows are exact.
- **Upper tier, RTX 3060.** Not present on this box. Unmeasured, and I make no
  claim about it.
- `CUDAContext::CUDAContext` calls `driver_.device_get(&device_, 0)` at
  `cuda_context.cpp:22`, device index 0 hardcoded, so which of the two cards
  the gate is evaluated against is decided by the environment rather than by
  the code. Both branches are reachable on this one machine. I note this only
  because it means the split is not hypothetical; per plan 5.1a I am not
  investigating multi-card operation.

So `adversary2-03-2.md`'s round-two phrasing, that this lands on "exactly the
baseline-tier hardware", is **true on the hardware in hand**, and pass B was
right that it does not follow from the tree. Both can be true at once, and
both are.

This is a fact for the planner, not a correction to either report. It does not
change a single number either report states. It changes which of their two
tables the baseline tier reads.

---

## 6. What remains before consensus

Small, bounded, and no new investigation.

1. **Both reports** add the kernel-launch staging buffers to the E15 list of
   pool consumers: `cuda/kernel_launcher.cpp:83`, `:93`,
   `amdgpu/kernel_launcher.cpp:59`, released at `:184` and `:147` to the
   caching allocator's free list and never to the bump head. Section 4.2. This
   is the only item I would hold consensus for, because E15 is a list and the
   list is incomplete.
2. **Pass A** states the CPU consequence for open question 8.1.2 positively, in
   the form pass B section 2.6 uses. One sentence. Section 4.3.
3. **Pass B** narrows the section 4.10 heading to its grep, or adds
   `runtime.cpp:587` and `:588`. Section 4.1.
4. Discretionary, and I would not hold consensus for either: pass B's
   `NodeManager` charge row, pass A adopting pass B's 64 MiB `data_list`
   example.

With item 1 in, and items 2 and 3 which are single sentences, I consider this
territory correct and complete and will say so without qualification. Nothing
in the seven claims put to me needs to move.

---

## 7. Escalations

Unresolved. I decide none of them, and per standing instruction 3 I call
nothing surplus.

**Z1 — E15's tier mapping is half answered, by measurement, and the planner
should decide what to do with it.** Section 5. The baseline tier's GTX 750 Ti
does not support CUDA memory pools on this box's driver, so on that card the
pool's denominator is shared with root buffers, ndarrays, argpacks and launch
staging; the GTX 1070 does support them, so on that card it is not. The RTX
3060 is unmeasured. Whether a sizing rule for item 6.1 reserves headroom, and
how much, remains workload-dependent and undecided. I propose no figure.

**Z2 — The pool's consumer list, now four classes rather than three.** Section
4.2. Whether launch-time staging belongs in a sizing rule at all, given it is
transient and reusable through the caching allocator but never returned to the
bump head, is a design decision.

**Z3 — `adversary2-03-1.md` escalations Y2 and Y4 carry the withdrawn
Python-only claim.** Section 2.3. Both reports and `adversary2-03-2.md` have it
right and plan section 6.1 has it right. An arbiter reading Y2 beside plan 6.1
sees a contradiction that is already settled against Y2. Recorded so it is not
re-litigated.

**Z4 — The two ceilings are set for CPU and for GPU by the same header value.**
Section 2.2 establishes that on CPU the constant binds alone and on CUDA and
AMDGPU it does not bind at all until the pool moves. `constants.h:12` is one
number serving two different constraints, and plan 5.1a's install-time agent
emits per-architecture bitcode already. Whether it emits one value or a
per-arch value is the planner's call. Both reports reach this; I am recording
that I agree it is unresolved and did not resolve it.

**Z5 — Everything already escalated by both reports stands.** The per-tree
assertion against the program-global index, `kMaxNumSnodeTreesLlvm` unchecked,
the offline-cache format under an id widening, the CMake `${arch}` fault, the
`taichi_listgen_max_element_size` third 1024, the destroy path's stale tables
and dangling allocation, the gfx numbering divergence, and the arch-dependent
`ptr2index` bound. I verified each this round and am adding nothing to any of
them.

---

## 8. Divergence from `adversary3-03-1.md`

`/opt/project/taichi/modernization/investigation/adversary3-03-1.md` does not
exist at the time of writing. No divergence section can be recorded. If it
lands later, the three places I expect a split are the verdicts themselves,
whether section 4.2's kernel-launcher finding is held to be in territory, and
whether taking the measurement in section 5 was within remit or beyond it.

---

## 9. What I did not do

- I modified no file except this one. No source file was touched. The
  `static_assert` verification was done by piping `runtime.cpp` through
  `g++ -fsyntax-only` on stdin, creating nothing; the CUDA probe was written,
  compiled and run entirely in the session scratchpad outside the project.
- I decided nothing was unnecessary and removed nothing.
- I added no abstraction and proposed no values for any constant.
- I read `report-03-runtime-struct.md`, `report-03b-runtime-struct.md`,
  `adversary2-03-1.md`, `adversary2-03-2.md` and the amendment entries of both
  notes files, and nothing from other territories.
- Where a count appears above, it is the count of the list printed beneath it,
  and each list came from a grep I ran this round.
