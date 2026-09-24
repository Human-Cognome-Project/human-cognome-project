# Adversary 2, 03-1 — runtime and struct layer, round two

Same adversary as `adversary-03-1.md`. Judging the revised
`report-03-runtime-struct.md` (pass A) and `report-03b-runtime-struct.md`
(pass B) against the source at `/opt/project/taichi`, and judging whether my
own round-one objections were met or merely acknowledged.

Everything below was re-derived by me this round: I recompiled the real
`runtime.cpp` for the struct sizes, re-simulated the bump allocator from the
source text, and printed source lines for every citation I quote. Where I say a
report is wrong I give the line that shows it.

---

## 0. The two verdicts

**CORRECT — pass B: yes. Pass A: no, one substantive error.**

Every load-bearing number in both reports reproduces independently. Pass A
carries one factual error that it introduced *this* round, in the one place it
ruled against round one: its statement that `ListManager::ptr2index` terminates
at the first untouched chunk is false on the CPU build. Section 3 below. Pass B
has the correct reading and pass A does not.

**COMPLETE — as a pair, yes. Individually, no.**

Each report has exactly one gap the other fills, and the two gaps are the two
places the explore agents split. Pass A holds the fact that the 1 GiB pool is a
CUDA and AMDGPU constraint and does not exist on the CPU path; pass B does not
have it, and pass B's escalation E5 is wrong for CPU as a result. Pass B holds
the correct arch split on `ptr2index`; pass A has it backwards. Neither is
fatal, and both are one paragraph of work.

**Consensus: not yet, narrowly.** Section 8 states exactly what remains. It is
two corrections and no new investigation.

---

## 1. Did the revisions actually address round one, or only acknowledge it?

I took every objection in `adversary-03-1.md` and `adversary-03-2.md` and
checked the revised text against the source, not against the revision record.

### 1.1 Objections against pass A — all met but one

| Round-one objection | Where it now sits | Met? |
|---|---|---|
| `snode_tree.cpp` / `gfx/snode_tree_manager.cpp` citations offset by +54 / +43 | report A §5.1, §5.4, §7.2 | **Met.** Verified every one, below |
| Alignment slack dropped from the footprint | report A §2.5 | **Met.** Simulated, not asserted |
| First touched chunk (4 MiB) never priced | report A §2.4, §2.5 | **Met** |
| The 1 GiB preallocation pool absent entirely | report A §3, new section | **Met**, and better than I asked |
| `all_dense` inverted; line 401 vs 402 | report A §2.6 | **Met**, with the conjunction stated in full |
| `preallocate_runtime_memory` fires from `:412-413` only when `!all_dense` | report A §3.2 | **Met**, with one imprecision, §5.3 below |
| `runtime_module/CMakeLists.txt:13` reads `${arch}` not `${rtm_arch}` | report A §4.7 | **Met** |
| `taichi_listgen_max_element_size` (`constants.h:28`) missing | report A §4.6, E9 | **Met** |
| `taichi_max_num_mem_requests` (`constants.h:16`) has no use | report A §4.6 | **Met** |
| Macro citations `68-76` / `82-87` | report A §4.2 now `69-77` / `85-88` | **Met** |
| `element_listgen_root` multiply cited at 1316 | report A §8.5 now 1319, 1322, 1323 | **Met** |
| `set_arg_ndarray_impl` cited `306-327 ... 324` | report A §8.1C now `:308-330 ... line 326` | **Met** |
| Setters exist but have zero call sites | report A §4.2 | **Met**, stated as "no mechanism, not merely no call" |
| Pool is a `DeviceAllocationGuard`, reclaimed at teardown | report A §5.5 | **Met** |
| Reachability of the leak never established | report A §6 | **Met** |
| `ptr2index` / `get_num_active_chunks` | report A §8.3 | **Addressed and got it wrong.** §3 |

I verified the citation regeneration by reading both files in full.
`taichi/struct/snode_tree.cpp` is 41 lines: `get_snodes_to_root_id_impl` 6-13,
the constructor 17-20, `check_tree_validity` 22-32, `get_snodes_to_root_id`
34-39. `taichi/runtime/gfx/snode_tree_manager.cpp` is 54 lines:
`materialize_snode_tree` 11-16 with the `push_back` at 15 and
`add_root_buffer` at 14, `destroy_snode_tree` 18-29 with the reset at 28,
`get_field_in_tree_offset` 31-47 with its `TI_ASSERT_INFO` at 34-38,
`get_snode_tree_device_ptr` 49-51. Report A now matches all of it.

### 1.2 Objections against pass B — all met

| Round-one objection | Where it now sits | Met? |
|---|---|---|
| "The card, not the constant, is the binding limit" | withdrawn, report B §2.6, §6 | **Met**, explicitly withdrawn |
| The 512-pointer-SNode figure in E5 | withdrawn, replaced by 254 | **Met** |
| Alignment slack dropped | report B §2.5 | **Met**, with the round-up derivation shown |
| First touched chunk never priced | report B §2.5 | **Met** |
| Pool absent | report B §2.6, §2.7 | **Met** |
| `all_dense` conjunction half-stated | report B §2.4 | **Met** |
| `NodeManager` `1024 * 16` override at `runtime.cpp:1030` | report B §4.4 | **Met** |
| Citation drift in `llvm_program.{h,cpp}`, `llvm_runtime_executor.cpp:257-269` | report B §4.1, §4.7 | **Met**, verified below |
| `CMakeLists.txt:13` variable fault | report B §1.1 item 6, E13 | **Met** |
| `taichi_listgen_max_element_size`, dead `taichi_max_num_mem_requests` | report B §4.10, E12 | **Met** |
| sm_60 bitcode flag cannot fire | report B §1.3, downgraded | **Met**, and B found a second gate I had not: `llvm_context.cpp:504-506` guards the call on `get_compute_capability() >= 60` |
| E4 reachability asserted, not established | report B §3.2 | **Met** |
| gfx numbering escalated rather than closed | report B §3.4 | **Met** |

The six drifted citations I flagged in round one are now right. I printed each:
`llvm_program.cpp:58-65` is `compile_snode_tree_types`, `:67-76` is
`materialize_snode_tree`, `:97-122` is `cache_field`, `llvm_program.h:188` is
the `get_field_in_tree_offset` signature and `:189` is the `FIXME`, and
`llvm_runtime_executor.cpp:257-270` is
`get_snode_num_dynamically_allocated`. Adversary 03-2's two, `:639` and
`:641-653`, are also corrected.

**Nothing was merely acknowledged.** Both revisions did the work. Report A
rewrote rather than patched, which is why its section numbering moved.

---

## 2. Pool capacity: 1020, not 1023. Both explore agents are right, I was right, adversary 03-2 was wrong.

I did not accept the convergence. I re-simulated the allocator from the source
text rather than from either report.

`taichi/runtime/llvm/runtime_module/runtime.cpp:848-853`:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

Line 850 charges the pad to the caller before the fit test at 851 and before
the head advance at 853. That is the whole dispute. With alignment `A` and
`r = head mod A`, the expression at 848-849 yields 0 when `r = 0` and `A - r`
otherwise, so it is ordinary round-up billed to the requester.

`sizeof(ListManager)` I re-measured myself by piping the real `runtime.cpp`
through `clang++ -fsyntax-only` with appended `static_assert`s, creating no
file. It is **1048616**, and `1048616 mod 4096 = 40`, so every request after the
first is charged 4056 bytes of pad and advances the head by 1052672.

Simulating the allocator exactly against the 1073741824-byte pool:

| Quantity | Value |
|---|---|
| head after 1020 element-list headers | 1,073,721,384 |
| pool size | 1,073,741,824 |
| head after 1021 | 1,074,774,056 |
| head after 1024 | 1,077,932,072 |
| overrun of the pool by the 1024 constant | 4,190,248 |

**Capacity is 1020. The overrun is 4,190,248 bytes, about four lists' worth.**

Adversary 03-2's 1023 and its "40,960 bytes, under one page over" come from
`1024 * 1048616` with no padding. That drops line 850. **03-2 is wrong and both
explore agents ruled correctly.** I concede nothing here because it was my own
round-one figure, but I re-derived it rather than reusing it, and it holds.

I also reproduced every other capacity row in both reports by simulation:

| Case | Report | Fits in 1 GiB | My simulation |
|---|---|---|---|
| element-list headers only | A §3.3, B §2.7 | 1020 | 1020 |
| header + first 4 MiB chunk | A §3.3, B §2.7 | 204 | 204 |
| pointer SNode, headers only | A §3.3, B §2.7 | 254 | 254 |
| pointer, headers + element chunk + 2 i32 chunks | B §2.7 | 125 | 125 |
| pointer, headers + element chunk + `data_list` at `node_size`=256 | A §3.3 | 85 | 85 |

The last two are different scenarios, not a disagreement. Each report labels
what it excludes. Both are right.

**Pass A's stated assumption about the pool base is over-cautious, and the
source closes it.** Report A §3.3 says the closed form "takes the pool base as
4096-aligned" and that "if the base is not page-aligned … capacity drops by at
most one". I ran the simulation at base offsets 0, 1, 40, 2048 and 4095.
**Capacity is 1020 in every case**, because the slack after 1020 headers is
20,440 bytes and the worst possible first-request pad is 4,095. Pass B's tighter
statement at its §2.7(a) — "the answer is 1020 whether or not the pool base
happens to be page-aligned" — is the correct one. Pass A's hedge is true but
weaker than the source supports.

One source fact neither report has, which is the actual answer to A's
assumption: `DeviceMemoryPool::allocate`
(`taichi/rhi/llvm/device_memory_pool.cpp:35-41`) **discards its `alignment`
argument** and forwards straight to `allocate_raw_memory` (`:53-92`), which is
a bare `cuMemAlloc`. So the base is whatever the driver returns and nothing in
this tree aligns it. It does not change the answer. Recorded so the assumption
does not have to be carried forward.

---

## 3. `ptr2index`: pass B is right, pass A is wrong, and I am conceding my own round-one position

This is the one place the two explore agents disagree with each other, and it is
the one substantive defect left in the territory.

`ListManager::ptr2index`, `runtime.cpp:502-512`, calls
`taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.")` at line
505, inside the loop and before the range test at 506.

What that assert does is arch-dependent. `taichi_assert_format`,
`runtime.cpp:766-815`:

- `:780-781` — `if (!enable_assert || test != 0) return;`. The early return
  fires when the test **passes**.
- `:782` — on failure the error is recorded once, guarded by
  `if (!runtime->error_code)`.
- `:798-800` — `#if ARCH_cuda` … `asm("exit;")`.
- `:801-802` — `#elif ARCH_amdgpu` … `asm("S_ENDPGM")`.
- `:814` — `#endif`, then the function returns normally.

**There is no thread kill outside CUDA and AMDGPU.** The host-arch bitcode is
built with `-D ARCH_x64` (`runtime_module/CMakeLists.txt:8`, driven over
`HOST_ARCH` at `:29`), so on the CPU build neither branch is compiled in and
control returns to the loop. Line 506 then evaluates `chunks[i] <= ptr` against
`nullptr`, which holds, and `ptr < chunks[i] + chunk_size`, which does not, so
no iteration matches, none breaks, and the loop runs all 131072.

So the three cases are:

| Case | Behaviour |
|---|---|
| pointer found, any arch | exits at that chunk, O(index of the chunk) |
| pointer not found, CUDA / AMDGPU | thread dies at the first untouched chunk |
| pointer not found, CPU | error flagged once, then 131072 iterations |

**Pass B has this exactly right** (report B §4.11, and its adjudication table at
§6). **Pass A has it wrong.** Report A §8.3 says:

> So it terminates at the first untouched chunk and is O(chunks touched).

and repeats it in escalation E14 as "bounded by chunks touched rather than by
live data". Both are false on the CPU build. Report A raises this under a
heading that explicitly rules against round one, so it is not an oversight; it
is a wrong adjudication.

**Adversary 03-2's round-one framing was also wrong**, but differently: it put
`ptr2index` on "the sparse hot path" as an unconditional 131072-iteration scan.
The full scan is the not-found error path, not the hot path, and the success
path is bounded by the touched set. Pass B's three-way split is the only reading
the source supports.

**My round-one position was wrong and I withdraw it.** I wrote that `ptr2index`
"terminates at the first untouched chunk … and it aborts rather than scanning
when the pointer is absent." That is the GPU behaviour stated as universal. The
brief makes CPU-only a first-class target (section 2), so the case I dismissed
is the case that matters most here.

Practical weight: this is an error path, reached only if a `Pointer_deactivate`
(`node_pointer.h:67-82`) hands `recycle` (`runtime.cpp:683-686`) a pointer its
`data_list` never issued. It is not a steady-state throughput fact. Pass B
records it and does not judge it, which is the right treatment. Pass A must
simply correct the sentence.

`get_num_active_chunks` (`runtime.cpp:467-473`) genuinely is unconditional over
all 131072, and both reports correctly confine it to the host debug printer
(`runtime.cpp:743-747` → `llvm_runtime_executor.cpp:200-201`).

---

## 4. Adversary 03-2's 2 GB card figures: pass B is right, they do not reproduce

`adversary-03-2.md` §7.1 says that on a 2 GB GTX 750 at
`device_memory_fraction = 0.9` the pool holds "351 plain SNodes at first-touch
cost or 211 pointer SNodes".

Pool at that setting is `0.9 * 2,147,483,648 = 1,932,735,283`. Simulating the
allocator against it:

| Case | 03-2 says | Simulation |
|---|---|---|
| element-list headers only | — | 1836 |
| plain sparse, header + first chunk | 351 | **368** |
| pointer SNode, headers only | 211 | **458** |
| pointer, headers + element chunk + 2 i32 chunks | — | 226 |

Neither figure reproduces, and neither is consistent with 03-2's own 1 GiB rows
of 204 and 126 scaled by 1.8, which give 367 and 227. **Pass B's ruling is
correct.** Report B §2.7(c) states 1836 / 368 / 458 and all three reproduce
exactly. Report A §3.4 declines to give derived counts at all and states only
the `TI_ASSERT(total_prealloc_size <= total_mem)` ceiling at
`llvm_runtime_executor.cpp:620`, which is also correct and is the safer choice.

One caveat neither report needs to change but the planner should hold: 2 GiB is
the nominal card size. The value the assert compares against is
`CUDAContext::get_total_memory` (`taichi/rhi/cuda/cuda_context.cpp:88-92`),
which returns the driver's reported total from `mem_get_info` and is lower.
Report B says "nominal", so it is not overstating.

---

## 5. Pass A's two added facts, and the gfx closure

### 5.1 The CPU exemption. VERIFIED, correct, and material.

Report A §3.2 states that the 1 GiB ceiling is a CUDA and AMDGPU fact and that
on the CPU path neither call site fires. I verified the whole chain.

`preallocate_runtime_memory` (`llvm_runtime_executor.cpp:607-632`) has exactly
two call sites — my own grep over the tree returns only these:

- `llvm_runtime_executor.cpp:719-723`, inside `materialize_runtime`, guarded by
  `if (config_.arch == Arch::cuda || config_.arch == Arch::amdgpu)` at `:719`
  and `if (!use_device_memory_pool())` at `:720`.
- `llvm_runtime_executor.cpp:412-413`, inside
  `initialize_llvm_runtime_snodes`, guarded by
  `config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense`.

`runtime_initialize_memory` (`runtime.cpp:962-971`) is the only writer of
`runtime_memory_chunk`, and `preallocate_runtime_memory` is its only caller
(`llvm_runtime_executor.cpp:629-631`). On x64 or arm64 neither guard passes,
`preallocated_size` stays 0, and `allocate_aligned` takes the
`host_allocator` branch at `runtime.cpp:834`.

That path has no ceiling. `host_allocate_aligned`
(`llvm_runtime_executor.cpp:28-32`) reaches `HostMemoryPool::allocate` →
`UnifiedAllocator::allocate`, which at
`taichi/rhi/common/unified_allocator.cpp:64-85` mmaps a fresh chunk sized
`std::max(size, default_allocator_size)` (line 68) whenever the current chunk
cannot serve the request.

**Consequence, and it is the one thing pass B gets wrong.** On CPU the pool does
not exist, so `taichi_max_num_snodes` *is* the binding constraint and raising it
alone *is* effective, bounded only by host RAM. Report B's escalation E5 says,
unqualified:

> The first two are already matched to within 0.4 %: the ceiling is 1024 and the
> default pool holds 1020 element-list headers. Raising either alone is inert.

That is false on CPU. Report B's body text is scoped correctly — §2.6 says "on
every CUDA card", §2.9 says "every tier", and every tier in brief 5.2 is a GPU —
so this is an escalation that outruns its own evidence rather than a wrong
measurement. But escalations are what the planner reads as the answer to open
question 8.1.2, and brief section 2 puts CPU-only operation in the first class.
It needs the qualifier.

Pass A's §3.2 wording has one imprecision of its own: "on CUDA the pool is
created either way; only *when* differs." When `use_device_memory_pool()` is
true and `all_dense` is true, `:412` does not fire and `:720` does not fire, so
the pool is created **neither** way. A's very next sentence supplies the
correct fact — "`all_dense` gates whether `runtime_memory_chunk` exists at all"
— so the paragraph is self-correcting, but the first clause is wrong as written.

### 5.2 The page-aligned base assumption. Honestly declared, and closable.

Covered in section 2 above. A declared it rather than hiding it, which is
correct conduct under standing instruction 2, but the answer is 1020 at every
base offset and `device_memory_pool.cpp:35-41` shows why nothing in the tree
aligns the base. B closed it and A did not.

### 5.3 Closing the gfx tree-id question rather than escalating it. Correct.

Pass A §7.2 and pass B §3.4 both close it, and both are right. I re-derived it
from the four functions:

1. `materialize_snode_tree` (`gfx/snode_tree_manager.cpp:11-16`) only
   `push_back`s — `add_root_buffer` at `:14` pushing at
   `gfx/runtime.cpp:747`, and `compiled_snode_structs_.push_back` at `:15`.
2. `destroy_snode_tree` (`:18-29`) finds `root_id` by linear scan on the root
   pointer (`:20-24`) and resets `root_buffers_[root_id]` at `:28`. Neither
   vector shrinks.
3. `Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
   (`program.cpp:235`); `allocate_snode_tree_id` (`:559-567`) pops it.
4. `get_snode_tree_device_ptr(int tree_id)` (`:49-51`) returns
   `runtime_->root_buffers_[tree_id]->get_ptr()`.

Create trees 0 and 1, destroy 1, create a third: the third gets `tree_id == 1`,
its buffer lands at `root_buffers_[2]`, and `root_buffers_[1]` is the reset
`unique_ptr`. `get_snode_tree_device_ptr(1)` dereferences null;
`get_field_in_tree_offset(1, …)` (`:31-47`) reads the destroyed tree's
descriptors and trips its own `TI_ASSERT_INFO` at `:34-38`.

**Divergence after one destroy-then-add cycle. Confirmed.** This is a
determinable fact, not a decision, so closing it was right; both reports keep
the remedy escalated, which is also right. Pass A additionally ties it to the
reachability finding in its §6 and calls it latent today. Pass B does not make
that tie in §3.4, though its E7 covers the same ground from the other side.

---

## 6. Testing the citation-regeneration claim

Pass A claims in `notes-03-runtime-struct.md` entry 28 that it re-extracted
every citation, bounds-checked 219 of them, and printed source lines for 47.

**Tested two ways, and the claim holds.**

**Bounds check.** I wrote my own extractor and ran it over both reports,
resolving each `file:line` and `file:a-b` against the real file length. Report A:
156 resolvable citations, **0 out of bounds**. Report B: 170 resolvable, **0 out
of bounds**. My regex is narrower than A's — it does not resolve the bare `:NNN`
continuation form — which is why my count is lower than 219. The +54 / +43
fault, which put several of A's round-one citations past end of file in a
41-line and a 54-line file, is gone.

**Content check.** Bounds checking cannot catch an in-range citation pointing at
the wrong line, so I printed the source line for every single-line citation in
both reports and read the full text of the two previously faulty files. Roughly
115 distinct citations. **One error found, in pass A:**

- Report A §4.5 says `snode_metas[i].id` is "populated from `snodes[i]->id` at
  `llvm_program.cpp:112`". Line 112 is the declaration
  `LlvmOfflineCache::FieldCacheData::SNodeCacheData snode_cache_data;`. The
  assignment is at **line 113**. Off by one. Changes nothing.

Everything else lands, across `runtime.cpp`, `llvm_runtime_executor.{cpp,h}`,
`llvm_context.{cpp,h}`, `llvm_program.{cpp,h}`, `program.cpp`, `snode.{cpp,h}`,
`snode_types.cpp`, `struct.{cpp,h}`, `snode_tree.{cpp,h}`,
`gfx/snode_tree_manager.{cpp,h}`, `gfx/runtime.{cpp,h}`, `node_pointer.h`,
`node_dynamic.h`, `ndarray.{cpp,h}`, `launch_context_builder.cpp`,
`llvm_offline_cache.h`, `snode_tree_buffer_manager.{cpp,h}`,
`unified_allocator.cpp`, `cuda_context.cpp`, `constants.h`, `export_lang.cpp`
and `runtime_module/CMakeLists.txt`.

**The measurements also hold.** I recompiled the real `runtime.cpp` with
`static_assert`s for `sizeof(LLVMRuntime) == 35256`,
`sizeof(ListManager) == 1048616`, `sizeof(NodeManager) == 56`,
`sizeof(Element) == 64`, `sizeof(PhysicalCoordinates) == 48`, and
`offsetof` of `roots` 88, `element_lists` 8296, `node_allocators` 16488,
`ambient_elements` 24680, `temporaries` 32872. It compiled clean. Both reports'
tables are correct, and the closed form `2488 + 24N + 16T` reproduces at every
row of both scaling tables I checked.

The eight-occurrence grep also holds: `taichi_max_num_snodes` at
`constants.h:12`, `struct_llvm.cpp:266`, `runtime.cpp:567,568,569`, and
`kMaxNumSnodeTreesLlvm` at `constants.h:13`, `runtime.cpp:562,563`. My own grep
over `taichi/`, `tests/`, `c_api/`, `cmake/`, `python/` and `misc/` returns
exactly those and nothing else. The four generated setters
`LLVMRuntime_set_element_lists`, `_set_node_allocators`, `_set_roots`,
`_set_root_mem_sizes` return **zero hits** outside the macro at
`runtime.cpp:69-77`. `taichi_max_num_mem_requests` returns only its definition
at `constants.h:16`.

---

## 7. Newly wrong or still missing

Beyond section 3 and section 5.1, these are what I found. None changes an
arithmetic conclusion.

**7.1 Report B §4.10's heading overclaims by two entries.** The heading is
"Every fixed-size array sized by an `inc/constants.h` value", and the section
lists five. The grep it discloses, `\[taichi_max_num\|\[kMaxNum`, cannot match
`char error_message_template[taichi_error_message_max_length]`
(`runtime.cpp:587`) or
`uint64 error_message_arguments[taichi_error_message_max_num_arguments]`
(`runtime.cpp:588`), and both are `inc/constants.h` values (`:18` and `:19`).
This is the same class of fault B corrected in that very section for
`taichi_listgen_max_element_size`. Report A has both arrays, in §2.2 and §9.
Neither scales with SNode count, so nothing downstream moves.

**7.2 Report A §9's fixed-array sweep misses two.** `alignas(8) char
tls_buffer[64]` at `runtime.cpp:1552` and `:1636`, in `gpu_parallel_range_for`
and `gpu_parallel_mesh_for`. Neither scales with SNode count, so A's conclusion
"none scale with SNode count" survives; only the word "exhaustive" does not.

**7.3 Report A §4.6 and §9 cite `element_listgen_root` as ending at 1329 while
report B ends it at 1328.** The closing brace is at 1329. Same for
`element_listgen_nonroot`: A says 1331-1383, B says 1331-1380, the closing brace
is at 1383. Trivial, recorded only because both reports were charged with
citation accuracy.

**7.4 Neither report states that the `NodeManager::data_list` chunk can be
64 MiB.** Report A §2.4 gives the formula `16384 × node_size` and prices a
`node_size` of 256 in §2.5; report B §2.5 gives the 1 KiB and 4 KiB cases
explicitly, "a 4 KiB cell gives 64 MiB". So B has it and A does not. Since brief
4.5 puts "a few hundred rows" of payload on each particle, cell sizes in the
kilobytes are plausible, and a single 64 MiB chunk is 6% of the default pool.
A should carry the same example B does.

**7.5 One arithmetic caveat both reports share, harmlessly.** Both compute the
multi-allocation rows as `k × (per-SNode charge)`. The true cumulative is that
minus the final allocation's unclaimed trailing pad. Every charge in play is a
multiple of 4096, so the difference is at most 4056 bytes and no row moves. I
confirmed by simulating the real allocation sequence rather than multiplying:
204, 254, 125 and 85 all reproduce.

**Nothing else.** I looked for a third pool, a second bound on the SNode arrays,
and any SNode-scaled storage outside `LLVMRuntime` and `ListManager::chunks`,
and found none. The `runtime_objects_chunk` (`runtime.cpp:936-941`) is a second
bump region but everything drawn from it — `temporaries` at `:952-954` and
`rand_states` at `:957-959` — is sized into the request at `:891-906`, so it
cannot be exhausted by SNode count. Both reports account for it.

---

## 8. What remains before consensus

Two corrections. No new investigation.

1. **Pass A, report §8.3 and escalation E14.** Withdraw "it terminates at the
   first untouched chunk and is O(chunks touched)". Replace with the arch split
   pass B states: early exit on success on every arch; thread death at the first
   null chunk under `ARCH_cuda` and `ARCH_amdgpu` only; a full 131072-iteration
   run on the CPU build, because `taichi_assert_format` (`runtime.cpp:766-815`)
   emits its kill only at `:800` and `:802`. Source is section 3 above.

2. **Pass B, escalation E5.** Qualify "raising either alone is inert" as a CUDA
   and AMDGPU statement. On the CPU path neither call site of
   `preallocate_runtime_memory` fires (`llvm_runtime_executor.cpp:412-413` and
   `:719-723`), `preallocated_size` stays 0, `allocate_aligned` falls through to
   `host_allocator` (`runtime.cpp:830-834`), and
   `UnifiedAllocator::allocate` (`unified_allocator.cpp:64-85`) has no ceiling.
   On CPU the constant is the binding limit and raising it alone does work.
   Brief section 2 makes that case first class.

Optional, and I would not hold consensus for them: pass A's
`llvm_program.cpp:112` → `:113`; pass A adopting B's 64 MiB `data_list` example;
pass B's §4.10 heading narrowed to its grep, or the two error-message arrays
added; pass A's `:112`-class span ends at 1329 / 1383.

Once item 1 and item 2 are in, I consider this territory correct and complete,
and I will say so without qualification.

---

## 9. Escalations

Unresolved. Each needs a decision the project owner has not made. I am choosing
none of them, and per standing instruction 3 I call nothing surplus.

**Y1 — The answer to open question 8.1.2 is not one rule, because the
constraint is not uniform across the target set.** On CUDA and AMDGPU the pair
`(taichi_max_num_snodes, device_memory_GB)` binds, matched today to within 0.4%
(`constants.h:12` and `compile_config.cpp:63`, capacity 1020 against a ceiling
of 1024). On CPU there is no pool and the constant binds alone. Brief section 2
makes CPU-only a first-class target and brief 5.2 lists three GPU tiers, so a
single replacement value serves two different constraints. Whether the
install-time loader emits one value or a per-arch pair is the planner's call.
Both reports reach the CUDA half; only pass A holds the CPU half.

**Y2 — `device_memory_GB` is reachable from C++ only as a `CompileConfig`
default.** Its only external setter is the Python binding
(`export_lang.cpp:208-210`), which brief 1.2 puts out of scope. Brief section 5
makes configuration an install-time decision and this is exactly such a
decision, but whether it joins item 6.1's parameterisation or is separate work
is undecided. Both reports escalate this; I am recording that I agree it is
unresolved and did not resolve it.

**Y3 — `ListManager::max_num_chunks` (`runtime.cpp:427`) and the element-list
chunk count `1024 * 64` (`runtime.cpp:1006`).** Together they set the
1,052,672-byte header charge and the 4,194,304-byte first-touch chunk, and
together they make a populated sparse SNode 5,246,976 bytes against a pool that
holds 204. Whether either is in scope for item 6.1 is undecided.

**Y4 — `demote_dense_struct_fors` (`compile_config.cpp:18`, default true;
forced true for SPIR-V at `:72-74`) is a further input.** With it false,
`all_dense` is false unconditionally (`llvm_runtime_executor.cpp:402-410`) and
every tree pays a `ListManager` per SNode, dense trees included. On the CUDA
mem-pool branch it also gates whether `runtime_memory_chunk` is created at all
(`:412-413`). Settable only from Python (`export_lang.cpp:201-202`). Whether the
install-time loader may touch it is undecided.

**Y5 — Will the C++ core call `Program::destroy_snode_tree`?** My own grep over
the tree confirms exactly one caller, the pybind lambda at
`export_lang.cpp:569-570`, driven from `python/taichi/_snode/snode_tree.py:21`.
No C++ and no test caller. Until the fork adds a recompose path, the stale
device tables, the dangling `snode_tree_allocs_` entry and the gfx numbering
divergence are all latent. Brief 4.6's "materialised working set, not the store
itself" suggests the path will be built. Not mine to decide.

**Y6 — The per-tree bound versus the program-global index.**
`struct_llvm.cpp:266` bounds `StructCompiler::snodes` per tree while
`runtime.cpp:1005`, `:1029` and `:1038` index by `SNode::id`, a global counter
(`snode.cpp:12`, `:220`) reset only in the `Program` constructor
(`program.cpp:144`), with `SNode::reset_counter` (`snode.h:348-350`) having no
caller and copying forbidden (`snode.cpp:230-233`). What the correct invariant
should be changes what "raise 1024 to X" means. Both reports have this and both
correctly decline to choose.

**Y7 — `kMaxNumSnodeTreesLlvm = 512` (`constants.h:13`) is bounds-checked
nowhere.** `Program::allocate_snode_tree_id` (`program.cpp:559-567`) returns
`snode_trees_.size()` or a recycled id with no comparison. Whether it is
parameterised alongside 1024 is undecided.

**Y8 — `taichi_listgen_max_element_size = 1024` (`constants.h:28`), used at
`runtime.cpp:1316`, `:1369` and `codegen_llvm.cpp:2291`.** A third hard 1024, in
dispatch rather than footprint. Brief 6.4 makes dispatch throughput the sole
criterion. Scope undecided.

**Y9 — The install-rule variable fault at
`runtime_module/CMakeLists.txt:13`.** Line 13 reads `${arch}`, the caller's
`foreach` variable from `:29`, where line 8 reads the function parameter
`${rtm_arch}` declared at `:3`. It works today only through CMake's parent-scope
read. Agent 04's territory. Both reports now flag it; neither chases it, which
is correct.

**Y10 — Widening SNode and tree ids changes the offline-cache on-disk format.**
`llvm_offline_cache.h:68` with `TI_IO_DEF` at `:73`, and `:76-77` with
`TI_IO_DEF` at `:81`. A cache version bump and an invalidation decision are
implied. Not mine.

**Y11 — The 131072 bound on the deactivate path, corrected.**
`Pointer_deactivate` (`node_pointer.h:67-82`) → `recycle`
(`runtime.cpp:683-686`) → `locate` (`:679-681`) → `ptr2index` (`:502-512`).
Per section 3 this costs a full 131072 iterations only on the CPU
pointer-not-found path. Recorded as a throughput fact against brief 6.4, not
judged.

---

## 10. Divergence from adversary2-03-2

`/opt/project/taichi/modernization/investigation/adversary2-03-2.md` does not
exist at the time of writing. No divergence section can be recorded.
