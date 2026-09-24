# Adversary2 03-2 — runtime and struct layer, round two

Second adversary, second round, on territory 03. Judging the revised
`report-03-runtime-struct.md` (pass A) and `report-03b-runtime-struct.md`
(pass B) against the source at `/opt/project/taichi`.

I wrote `adversary-03-2.md` in round one. Everything below was re-derived from
the source or from my own simulation. Where I say something is wrong I give the
line I read. Where round one was mine and wrong, I say so plainly.

---

## 0. The two verdicts

**CORRECT: no, for both — but for reasons an order of magnitude smaller than
round one's.**

The citation apparatus is now sound in both reports. I bounds-checked every
numeric citation mechanically and content-checked roughly 180 of them by
printing the source line. Nothing is out of range and nothing I sampled points
at the wrong line. Pass A's systematic +54 / +43 fault is gone. Every load-
bearing figure in both reports reproduces against my own simulation of the bump
allocator.

Three content defects survive, and one of them is new:

1. **Both reports assert that the SNode tree root buffers sit outside the 1 GiB
   pool. They do not, on the path where `use_device_memory_pool()` is false.**
   Section 4.1. Pass A section 3.4, pass B section 2.7(c). This is the same
   class of miss round one faulted: one call frame short of the allocation.
2. **Both reports state that `device_memory_GB` is reachable only from Python
   and is "a hard-coded 1 at C++ level".** It is a public field of a
   `TI_DLL_EXPORT` global. Section 4.2. Pass B marks the claim **[V]**.
3. **Pass A's `ptr2index` correction is wrong on CPU.** Section 3.2. Pass A
   adopted adversary 03-1's narrowing without reading `taichi_assert_format`;
   pass B read it and got the right answer.

**COMPLETE: no, for both, but narrowly and for one shared reason.**

The pool is now in both reports and it is the right answer as far as it goes.
What is still missing is the pool's *other* consumers (4.1) and the positive
statement of what binds on CPU (4.3), which is the tier brief section 2 makes
first class. Everything else the plan needs for open question 8.1.2 is present
in both.

**Consensus: not yet reached.** What remains is in section 7 and is small,
bounded, and does not require another full pass.

---

## 1. Round one objections — addressed or merely acknowledged

My round one file raised eleven items. I checked each against the revised text
and against source.

| # | Round one objection (mine) | Status |
|---|---|---|
| 1 | Pass A's `snode_tree.cpp` and `gfx/snode_tree_manager.cpp` citations all wrong | **ADDRESSED.** All regenerated and correct. Section 2 below. |
| 2 | Pass A inverts the `all_dense` condition | **ADDRESSED.** Pass A section 2.6 now states the conjunction and names the line as 402. Pass B section 2.4 states it too, which round one said neither did. |
| 3 | Both quote the wrong `NodeManager` chunk count | **ADDRESSED.** Pass A 2.4 and pass B 4.4 both record the `1024 * 16` override at `runtime.cpp:1030` and that the halving loop at `:652-655` engages only above `node_size` 8192. Verified. |
| 4 | The gfx numbering question is resolvable and both declined to resolve it | **ADDRESSED.** Pass A 7.2 and pass B 3.4 both close it. Section 3.3 below. |
| 5 | Neither found the pool | **ADDRESSED, and both corrected my arithmetic.** Section 3.1. |
| 6 | Chunk sizes unquantified | **ADDRESSED.** Pass A 2.4, pass B 2.5. `65536 × 64 = 4,194,304` verified against `runtime.cpp:1006` and a measured `sizeof(Element)` of 64. |
| 7 | Failure mode is a grid abort, not degradation | **ADDRESSED.** Pass A 3.5, pass B 2.8, both with the corrected span `runtime.cpp:864-869` and the `:872` fallback. Verified. |
| 8 | The 131072 loop bound on the deactivate path | **ADDRESSED CORRECTLY by pass B (4.11). MIS-ADDRESSED by pass A (8.3).** Section 3.2. |
| 9 | Pass B's leak claim asserted reachability without establishing it | **ADDRESSED.** Pass B 3.2 and pass A section 6 both establish the single Python caller at `export_lang.cpp:569-570` and state the finding as latent. |
| 10 | Pass B must withdraw "the card is the binding limit" | **ADDRESSED.** Explicit withdrawal at pass B 2.6 and in its revision record. |
| 11 | The out-of-bounds write is reachable and neither said so | **ADDRESSED as fact.** Pass A 4.3 constructs the two-tree case explicitly and says it did not build a failing case. Pass B 3.1 states it. Both stop at fact, which is right per standing instruction 3. |

Adversary 03-1's objections, which I also checked because step 7 of the method
makes the whole round one record the input:

- Pool, `all_dense` default and pool gating, the `CMakeLists.txt:13`
  `${arch}` / `${rtm_arch}` fault, `taichi_listgen_max_element_size`, the dead
  `taichi_max_num_mem_requests`, the sm_60 gating, pass A's macro lines
  (`69-77`, `85-88`), the listgen multiplies (`1319`, `1322`, `1323`), and
  `set_arg_ndarray_impl` (`308-330`, truncation at `326`) — **all addressed in
  both reports, all verified by me against source.**
- Adversary 03-1's own `ptr2index` ruling was wrong. Pass A adopted it. Section
  3.2.

Nothing was merely acknowledged. Every objection that was accepted was acted
on in the text, not footnoted.

---

## 2. The citation claim — tested, and it holds

Pass A claims (notes entry 28) to have regenerated every citation and
bounds-checked 219 of them, after round one found every `.cpp` line in two
files offset by the length of the neighbouring header.

**I tested it independently rather than accepting it.**

I extracted every ``​`path:line`​`` and ``​`path:a-b`​`` reference from both
finished reports with a regex, resolved bare basenames against the tree, and
compared each number to the real file length.

| Report | Numeric line references | Past end of file | Unresolvable paths |
|---|---|---|---|
| `report-03-runtime-struct.md` | 416 | 0 | 0 |
| `report-03b-runtime-struct.md` | 504 | 0 | 0 |

Pass A's "219 distinct citations" counts file-and-span pairs; mine counts
individual line numbers. Consistent.

Bounds checking cannot catch an in-range citation pointing at the wrong line,
so I printed the source line for roughly 180 references across both reports,
weighted toward the two files that carried the fault and toward everything
either round one adversary disputed. Every one lands, including:

- `taichi/struct/snode_tree.cpp` is 41 lines; constructor `17-20`,
  `check_tree_validity` `22-32`, `get_snodes_to_root_id` `34-39`. All correct
  in both reports.
- `taichi/runtime/gfx/snode_tree_manager.cpp` is 54 lines;
  `materialize_snode_tree` `11-16`, `destroy_snode_tree` `18-29`, the reset at
  `:28`, `get_field_in_tree_offset` `31-47` with its `TI_ASSERT_INFO` at
  `34-38`, `get_snode_tree_device_ptr` `49-51`. All correct in both.
- `runtime.cpp` throughout: `339` `enable_assert`, `424-425` the upstream TODO,
  `427-428`, `451-456`, `467-473`, `502-512`, `562-563`, `567-569`, `606-612`,
  `643-664` with the three lists at `658-659`/`660-661`/`662-663`, `823-835`,
  `838-874`, `848-853`, `864-869`, `870`, `872`, `891-906`, `933`, `962-971`,
  `986-994`, `996-997`, `1000-1007`, `1016`, `1026-1031`, `1033-1040`,
  `1270-1273`, `1316`, `1319`, `1322-1323`, `1369`, `1664-1679`, `1681-1689`,
  `1691-1692`, `1721`, `1738`, `1782`, `1831`.
- `llvm_runtime_executor.cpp`: `188-209`, `257-270`, `386-389`, `402-410`,
  `412-414`, `417-420`, `437-440`, `442-444`, `446-468`, `476-491`, `493-498`,
  `587-604`, `607-632`, `620`, `639`, `641-653`, `675-694`, `719-723`,
  `758-761`, and `49`.
- `llvm_program.cpp:45-56`, `:58-65`, `:61`, `:67-76`, `:97-122`, `:110-119`;
  `llvm_program.h:52-59`, `:97-103`, `:188-189`, `:199`. Pass B's round one
  drift into this file is corrected.
- `program.cpp:144`, `:214-236`, `:235`, `:238-255`, `:257-259`, `:559-567`;
  `constants.h:5,12,13,16,28`; `compile_config.cpp:18,63,64,72-74`;
  `struct_llvm.cpp:247,266,269`; `codegen_llvm.cpp:2291,2689-2692,2697`;
  `snode.cpp:12,220,230-233`; `snode.h:88,98,306-308,348-350`;
  `snode_types.cpp:21-23`; `node_pointer.h:10-12,44,45,55,67-82,69,70,76,86,92,96`;
  `CMakeLists.txt:3,8,9,10,13,17,24,26,29-31,36-42`;
  `lang_util.cpp:18-28,30-45`; `llvm_context.cpp:209-211,358-362,504-506,574-591,968-973,1004-1011,1036`;
  `cuda_context.cpp:35-53,88-92`; `export_lang.cpp:201-202,208-210,569-570,1222`;
  `gfx/runtime.cpp:730-750,752-765`; `unified_allocator.cpp:68`.

**Ruling: the claim is honest and the fault is repaired.** Two micro-slips
survive, neither material:

- Pass A escalation E12 gives the `int` accumulate init as `ndarray.cpp:38` and
  `:78`. The literal `1` is at `:37` and `:77`; lines 38 and 78 are
  `std::multiplies<>())),`. The spans quoted in section 8.1B (`35-38`, `75-78`)
  are right, and the clean `1LL` counterparts really are on the cited `:53` and
  `:96`, so only the two escalation line numbers are off by one.
- Pass B's charge table (report B lines 352-357) gives the `NodeManager`'s
  charge as 4,096 "from an aligned head". From an aligned head a 56-byte
  request at alignment 4096 pays no padding at all
  (`runtime.cpp:848-849` yields 0 when the head is aligned); the page is
  effectively consumed by the *next* request. B's own framing at 2.7 says
  exactly this, so the table contradicts the prose. The per-SNode totals it
  feeds are nevertheless right — I simulated them, section 3.1 — because the
  4,112 charged for the `NodeManager` and the 1,052,656 charged for the next
  list differ from the table's 4,096 and 1,052,672 by offsetting 16 bytes.

Both are cosmetic. Neither changes a number the planner would use.

---

## 3. Where the explore agents ruled against round one — adjudicated

### 3.1 Pool capacity: 1020, not 1023. **Both explore agents are right. I was wrong.**

I simulated the allocator rather than re-deriving by hand, from
`taichi/runtime/llvm/runtime_module/runtime.cpp:838-874`:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

Line 850 is the whole dispute. The padding is added to `size` *before* the fit
test at 851 and before the head advance at 853, so it is charged to the caller.
With `A = 4096` and `r = head mod A`, line 849 gives `A - 1 - (A - 1) = 0` when
`r = 0` and `A - 1 - (r - 1) = A - r` when `r > 0`. That is ordinary round-up,
billed to the requester.

`create<T>` (`runtime.cpp:606-612`) requests `sizeof(ListManager)` at alignment
4096. `1048616 mod 4096 = 40`, so from an aligned head the first list advances
the head to residue 40 and every subsequent one is charged
`1048616 + 4056 = 1052672`, exactly 257 pages, leaving the residue at 40.

```
cum(k)    = 1,048,616 + (k-1) x 1,052,672
cum(1020) = 1,073,721,384   <=  2^30 = 1,073,741,824    fits, slack 20,440
cum(1021) = 1,074,774,056   >   2^30                    does not
cum(1024) = 1,077,932,072   -> over the pool by 4,190,248
```

**Capacity 1020. Overrun 4,190,248 bytes.** My round one figure of 1023 and
"40,960 bytes, under one page over" dropped the alignment charge and is wrong.
Both explore agents reached 1020 independently and both are right. Adversary
03-1 was right in round one and I was not. The qualitative conclusion we all
share — that the constant and the default pool are matched to under half a
percent, so moving one alone buys nothing — is unaffected.

The derived rows also reproduce: 204 populated plain sparse SNodes at
5,246,976 each, 254 pointer SNodes at 4,214,784 each. I re-derived the pointer
figure by stepping the allocator through the real order
(`create<ListManager>` at `runtime.cpp:1005-1006`, then `create<NodeManager>`
at `:1029-1030`, then the three lists inside the constructor at `:658-663`) and
the steady-state charge is 4,214,784 exactly. Both reports' number is right.

**Page-aligned pool base.** Pass A flags this as an unverified assumption
(report A section 3.3) and says capacity "drops by at most one" if the base is
not page-aligned. The assumption is real — `runtime_initialize_memory`
(`runtime.cpp:962-970`) takes the raw buffer from
`preallocate_memory` (`llvm_runtime_executor.cpp:587-604`) as the head, with no
rounding. But it is **immaterial**: any base misalignment costs at most 4095
bytes of extra first pad, and the slack at k=1020 is 20,440. Capacity is 1020
either way. Pass B states this explicitly (report B line 439-440) and is right
to; pass A's honest caveat is correct but leaves a settled question sounding
open.

### 3.2 `ptr2index`: **pass B is right, pass A is wrong, and both round one adversaries were partly wrong.**

`runtime.cpp:502-512`:

```
504:    for (int i = 0; i < max_num_chunks; i++) {
505:      taichi_assert_runtime(runtime, chunks[i] != nullptr, "ptr not found.");
506:      if (chunks[i] <= ptr && ptr < chunks[i] + chunk_size) {
```

The question is what line 505 does when the test fails.
`taichi_assert_runtime` (`runtime.cpp:817-819`) forwards to
`taichi_assert_format` (`runtime.cpp:766-815`), which:

- returns immediately at `:780-781` when the test **passes**;
- on failure records the error under the `if (!runtime->error_code)` guard at
  `:782`, then kills the thread only under `#if ARCH_cuda`
  (`asm("exit;")`, `:798-800`) or `#elif ARCH_amdgpu` (`asm("S_ENDPGM")`,
  `:801-802`). The `#endif` is at `:814`. **There is no CPU branch.**

So on the host bitcode the assert sets a flag and returns, the range test at
506 fails for a null chunk against any real pointer, and the loop runs to
131071 and returns `-1` at `:511`.

- **Pass B, section 4.11 and its adjudication table, is correct**: success case
  is O(index of the containing chunk) on every arch; not-found kills the thread
  on CUDA and AMDGPU; not-found on CPU runs all 131072 iterations.
- **Pass A, section 8.3, is wrong.** It writes "it terminates at the first
  untouched chunk and is O(chunks touched)" with no arch qualification, and
  repeats the unqualified form in escalation E14. Its notes entry 24 shows why:
  it read line 505 and `enable_assert` at `runtime.cpp:339` and stopped, never
  opening `taichi_assert_format`.
- **Adversary 03-1's round one ruling was wrong** in exactly the way pass A
  then inherited.
- **My round one ruling was half wrong.** Calling both functions "O(131072)
  linear scans on the sparse hot path" is wrong for the success case, which is
  the normal case. It is right for the CPU not-found case, which is what pass B
  established.

This is not a trivia point. Brief section 2 makes CPU-only a first-class
target, and pass A's uncorrected version tells the planner the CPU deactivate
path is bounded by live data when it is bounded by 131072.

`get_num_active_chunks` (`runtime.cpp:467-473`) genuinely is unconditional over
131072, and its only reachable caller is the host debug printer
(`runtime.cpp:743-747` → `llvm_runtime_executor.cpp:200-201`). Both reports say
so and both are right.

### 3.3 The 2 GB card figures: **pass B is right, my round one numbers do not reproduce.**

Pass B's ruling is that my "351 plain SNodes or 211 pointer SNodes" neither
reproduce nor agree with each other. I tested it.

With `device_memory_fraction = 0.9` on a nominal 2 GiB card,
`llvm_runtime_executor.cpp:617-618` gives `0.9 x 2,147,483,648 =
1,932,735,283`. Against that pool:

| Case | Charge per SNode | Fits |
|---|---|---|
| element-list header | 1,052,672 | 1836 |
| populated plain sparse | 5,246,976 | 368 |
| pointer SNode, headers | 4,214,784 | 458 |

All three of pass B's figures reproduce exactly under my simulation. Mine do
not: 351 at my round one per-SNode figure of 5,242,920 implies a pool between
1,840,264,920 and 1,845,507,839, while 211 at my figure of 8,519,896 implies a
pool between 1,797,698,056 and 1,806,217,952. Those ranges are disjoint, so the
two figures cannot both come from one pool value. **Pass B's ruling is correct
and I withdraw both numbers.**

### 3.4 The gfx tree-id numbering: **closing it was right, and the construction holds.**

Pass A closed it in 7.2 rather than escalating, as instructed. I re-derived it
from source rather than accepting either report:

1. `materialize_snode_tree` (`snode_tree_manager.cpp:11-16`) only `push_back`s
   — `add_root_buffer` at `:14` pushing at `gfx/runtime.cpp:747`, and
   `compiled_snode_structs_.push_back` at `:15`. Index is materialisation order.
2. `destroy_snode_tree` (`:18-29`) finds `root_id` by linear scan on the root
   pointer at `:20-24`, `TI_ERROR`s if absent at `:25-27`, and resets
   `runtime_->root_buffers_[root_id]` at `:28`. Neither vector shrinks.
3. `Program::destroy_snode_tree` pushes the id at `program.cpp:235`;
   `allocate_snode_tree_id` (`program.cpp:559-567`) pops it at `:563-565`.
4. `get_snode_tree_device_ptr(int tree_id)` (`:49-51`) returns
   `root_buffers_[tree_id]->get_ptr()`.

Create 0 and 1, destroy 1, create a third: the third gets `tree_id == 1` from
the free stack, its buffer lands at `root_buffers_[2]`, and `root_buffers_[1]`
is the `unique_ptr` reset at `:28`. `get_snode_tree_device_ptr(1)` dereferences
null; `get_field_in_tree_offset(1, ...)` reads
`compiled_snode_structs_[1]`, the destroyed tree's descriptors, and trips its
own `TI_ASSERT_INFO` at `:34-38`. **Diverges after one destroy-then-add cycle.**

Both reports state it identically and correctly, both keep the remedy
escalated, and both correctly note that the LLVM path does not have the
analogue because `snode_tree_allocs_` is an `unordered_map` keyed by tree id
(`llvm_runtime_executor.h:152`). Nothing left open here.

### 3.5 Pass A's two added facts

**(a) The 1 GiB pool ceiling is a CUDA and AMDGPU fact. VERIFIED, and it is the
most useful thing added this round.**

`preallocate_runtime_memory` has exactly two call sites, and I confirmed the
count by grep:

- `llvm_runtime_executor.cpp:719-723`, in `materialize_runtime`, guarded on
  `config_.arch == Arch::cuda || config_.arch == Arch::amdgpu` at `:719` and
  `!use_device_memory_pool()` at `:720`.
- `llvm_runtime_executor.cpp:412-414`, in `initialize_llvm_runtime_snodes`,
  guarded on `config_.arch == Arch::cuda && use_device_memory_pool() &&
  !all_dense` at `:412`.

`runtime_initialize_memory` (`runtime.cpp:962-970`) has exactly one caller in
the tree, `llvm_runtime_executor.cpp:629-631`, inside that function. So on CPU
neither guard passes, `runtime_memory_chunk.preallocated_size` stays 0, and
`allocate_aligned` takes the `host_allocator` branch at `runtime.cpp:834`.
`UnifiedAllocator::allocate` then gives an oversized request its own chunk
(`taichi/rhi/common/unified_allocator.cpp:64-68`), so there is no ceiling.
**Pass A's scope limit is correct.** DX12, which also uses the LLVM path,
behaves like CPU here for the same reason; neither report names it, which is
fair since agent 04 owns dx12.

**Pass B does not have this.** I grepped: report B contains no statement of the
CPU allocation path for the pool. Its 2.6 says "the default pool is 1 GiB on
every CUDA card", which is literally true, and its 2.9 and E5 then say raising
the constant alone "is inert on every tier", which is true for the three tiers
in brief 5.2 because all three are NVIDIA parts. But the report never
establishes what binds when there is no card, and brief section 2 puts that
case first. Gap, not error.

**(b) The page-alignment assumption. VERIFIED as a real assumption and settled
as immaterial.** Section 3.1 above. Pass A was right to declare it rather than
hide it, and right that it did not verify it. The 20,440-byte slack settles it,
and pass B settled it explicitly.

**One internal inconsistency in pass A's 3.2.** It writes "on CUDA the pool is
created either way; only *when* differs", then in the next sentence "On the
mem-pool branch, `all_dense` gates whether `runtime_memory_chunk` exists at
all". The second sentence is right and the first is wrong: with
`config_.arch == Arch::cuda`, `use_device_memory_pool()` true and `all_dense`
true, neither guard passes and no pool is created. `use_device_memory_pool_` is
set only for CUDA (`llvm_runtime_executor.cpp:39-50`, assignment at `:49`), so
on AMDGPU the first guard always fires. Pass B's 2.4 states the same mechanism
without the contradiction.

---

## 4. Newly wrong, or still missing

### 4.1 Both reports say the root buffers are outside the pool. On the non-mem-pool path they are inside it.

This is the one finding that keeps consensus out of reach, and it is the same
shape as round one's: both passes stopped one call frame short of the
allocator.

Pass A, section 3.4: *"The root buffers
(`snode_tree_buffer_manager.cpp:12-18`), the runtime-objects buffer
(`llvm_runtime_executor.cpp:687-690`) and the CUDA context all come out of the
same card **outside** this pool."*

Pass B, section 2.7(c): *"That pool leaves roughly 200 MB of the card for the
root buffers, the runtime objects buffer and the CUDA context, all of which are
separate allocations outside it. **[I]**"*

The runtime-objects buffer really is separate — it is its own
`preallocate_memory` call at `llvm_runtime_executor.cpp:687-690`, feeding
`runtime_objects_chunk` at `runtime.cpp:936-941`. The root buffers are not.
The chain, every hop read:

1. `initialize_llvm_runtime_snodes` allocates the tree's root buffer at
   `llvm_runtime_executor.cpp:419-420` through
   `snode_tree_buffer_manager_->allocate`.
2. `SNodeTreeBufferManager::allocate`
   (`taichi/runtime/llvm/snode_tree_buffer_manager.cpp:12-18`) calls
   `runtime_exec_->allocate_memory_on_device` at `:15`.
3. `LlvmRuntimeExecutor::allocate_memory_on_device`
   (`llvm_runtime_executor.cpp:476-491`) calls
   `llvm_device()->allocate_memory_runtime`, passing
   `use_device_memory_pool()` as the `use_memory_pool` field at `:485`.
4. `CudaDevice::allocate_memory_runtime`
   (`taichi/rhi/cuda/cuda_device.cpp:50-78`) branches at `:56`. With
   `use_memory_pool` **true** it takes `cuMemAllocAsync` at `:57-58`, genuinely
   outside the pool. With it **false** it takes
   `DeviceMemoryPool::get_instance().allocate_with_cache` at `:60-61`.
5. `DeviceMemoryPool::allocate_with_cache`
   (`taichi/rhi/llvm/device_memory_pool.cpp:27-33`) →
   `CachingAllocator::allocate` (`taichi/rhi/llvm/allocator.cpp:33-58`), which
   on a cache miss calls `device->allocate_llvm_runtime_memory_jit` at `:54-55`.
6. `CudaDevice::allocate_llvm_runtime_memory_jit`
   (`cuda_device.cpp:80-90`) JIT-calls the device function
   `runtime_memory_allocate_aligned` at `:82-84`.
7. `runtime_memory_allocate_aligned` (`runtime.cpp:879-886`) is
   `runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)`
   at `:884-885`. **The same 1 GiB bump allocator the `ListManager`s come
   from.**

`AmdgpuDevice::allocate_llvm_runtime_memory_jit`
(`taichi/rhi/amdgpu/amdgpu_device.cpp:80-90`) and
`CpuDevice::allocate_llvm_runtime_memory_jit`
(`taichi/rhi/cpu/cpu_device.cpp:53-59`) are the same shape. Since
`use_device_memory_pool_` is set only under CUDA
(`llvm_runtime_executor.cpp:49`), **on AMDGPU the root buffers always come out
of the pool**, and on CUDA they do whenever the driver is below 11.2 or the
device lacks `CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`
(`taichi/rhi/cuda/cuda_context.cpp:35-53`).

The same path carries `Ndarray` storage (`taichi/program/ndarray.cpp:61` via
`Program::allocate_memory_on_device`, `taichi/program/program.h:245-248`) and
argpack buffers (`taichi/program/argpack.cpp:18`).

**Why it matters for 8.1.2.** Every capacity row in both reports — 1020, 204,
254, 125, 85 — assumes the whole 1 GiB is available to `ListManager`s. On the
non-mem-pool path it is not: the root buffers, sized
`iroundup(root_size, taichi_page_size)` at `llvm_runtime_executor.cpp:417`, and
every ndarray, come out of the same denominator. The older-driver and AMDGPU
cases are exactly the edge hardware brief section 3.1 and section 5.2's
baseline tier describe. A sizing rule taken from either report's table would be
optimistic there by however much root and ndarray storage the workload holds.

Related precision point, same path: both reports say the tree's root buffer
"is freed" on destroy (pass A 5.5, pass B 3.2 by implication). On the mem-pool
path that is `mem_free_async` (`cuda_device.cpp:107-108`) and is true. On the
non-mem-pool path `dealloc_memory` takes the `info.use_cached` branch at
`:109-111` and returns the block to `CachingAllocator`'s free list
(`device_memory_pool.cpp:43-51` → `allocator.cpp:60-68`). It is reusable by
later runtime-memory allocations but is never returned to the bump head at
`runtime.cpp:853`. That is consistent with "no free path in the bump
allocator", which both reports state correctly, but "freed" is
path-conditional.

I am recording this, not proposing anything about it.

### 4.2 `device_memory_GB` is not Python-only, and not hard-coded at C++ level

Pass B, section 2.6: *"`device_memory_GB` and `device_memory_fraction` are
reachable only from Python (`taichi/python/export_lang.cpp:208-210`), out of
scope per brief 1.2, so at C++ level the pool is a hard-coded 1 GiB.* **[V]**"
Repeated in pass B's E5, and B makes the same claim for
`demote_dense_struct_fors` in 2.4 and E5c, also marked **[V]**.

Pass A, escalation E1: *"at C++ level it is currently a hard-coded 1."*

Against source:

- `struct CompileConfig` is a plain struct with public fields,
  `taichi/program/compile_config.h:8`. `device_memory_GB` and
  `device_memory_fraction` are `float64` at `:71-72`;
  `demote_dense_struct_fors` is at `:28`.
- `extern TI_DLL_EXPORT CompileConfig default_compile_config;` at
  `compile_config.h:110`, defined at `taichi/util/lang_util.cpp:14`.
- `Program::Program(Arch)` copies it wholesale:
  `taichi/program/program.cpp:74-77`, `config = default_compile_config;` at
  `:75`, then `config.fit()` at `:77`.
- The pybind binding at `export_lang.cpp:265-266` returns a reference to that
  same global. Python is one client of it, not the owner.

So a C++ driver sets `taichi::lang::default_compile_config.device_memory_GB`
before constructing a `Program`, with no new mechanism and no Python. That
inverts the relative cost of the two halves of the pair the plan's 8.1.2 now
records: `taichi_max_num_snodes` is a `constexpr` baked into per-arch bitcode
by `runtime_module/CMakeLists.txt:8` and needs a build-time mechanism, while
`device_memory_GB` is a runtime field on an exported global and needs none.
Both reports currently tell the planner the opposite, and pass B marks it
verified.

Note also that both are `float64`, so fractional values are legal and are
already used (`tests/python/test_offline_cache.py:54` uses 0.2). Neither report
records the type. That is a small thing on its own and a real one when the
question on the table is what value the knob should take.

### 4.3 Neither report states what binds on CPU

Pass A establishes the negative — the 1 GiB ceiling does not apply on CPU
(3.2) — and stops. Pass B does not have it at all. Neither states the positive
consequence, which is short and follows from facts pass A already has:

On CPU there is no pool, so nothing bounds the `ListManager`s except host
memory and the array itself. `element_lists[i]` is a 1024-entry array
(`runtime.cpp:567`) and the write at `runtime.cpp:1005` is unchecked, so **on
CPU `taichi_max_num_snodes` is the binding limit, and raising it alone is
effective rather than inert** — subject only to host RAM at the ~5.2 MiB per
populated sparse SNode both reports compute. That is the opposite of the
headline conclusion both reports give, and it is the answer for the tier brief
section 2 puts first.

I am not proposing a value. I am recording that 8.1.2 has a different shape on
CPU than on CUDA and neither report says so.

### 4.4 Minor, recorded for completeness

- Pass A 3.2's "the pool is created either way" contradicts its own next
  sentence. Section 3.5(b) above.
- The two reports' fourth capacity rows describe different scenarios — pass A
  includes the `data_list` chunk and omits the two `i32` chunks (12,603,392,
  85 fit); pass B includes the two `i32` chunks and omits `data_list`
  (8,540,160, 125 fit). Both are internally correct and both label their
  scenario; I verified both totals and both fits. A reader comparing the two
  tables side by side will think they disagree. They do not.
- Pass A escalation E12's `ndarray.cpp:38` / `:78`. Section 2 above.
- Pass B's charge table row for `NodeManager`. Section 2 above.

---

## 5. What both reports now get right, and it is most of it

Recorded because saying so is part of the job.

- The two constants have exactly eight occurrences in the tree. My own
  unrestricted grep over `taichi/`, `c_api/`, `tests/`, `python/`, `cmake/` and
  the top-level `CMakeLists.txt` returns precisely
  `constants.h:12,13`, `struct_llvm.cpp:266`, `runtime.cpp:562,563,567,568,569`
  and nothing else. Both inventories are exact.
- `taichi_listgen_max_element_size` at `constants.h:28` is used at
  `runtime.cpp:1316`, `:1369` and `codegen_llvm.cpp:2291`, and
  `taichi_max_num_mem_requests` at `constants.h:16` has no use anywhere. Both
  reports now carry both, correctly.
- The per-tree assertion versus the program-global index. `SNode::id` from the
  atomic at `snode.cpp:12` and `:220`, reset only at `program.cpp:144`,
  `SNode::reset_counter` (`snode.h:348-350`) with no caller, copying forbidden
  at `snode.cpp:230-233`. Both correct, and it remains the sharpest structural
  finding in this territory.
- `kMaxNumSnodeTreesLlvm` bounds-checked nowhere;
  `Program::allocate_snode_tree_id` (`program.cpp:559-567`) compares against
  nothing. Both correct.
- Every `sizeof` and `offsetof` figure. I confirmed these with the compiler in
  round one and both reports still carry the same values.
- The `all_dense` conjunction, its default, and the SPIR-V forcing at
  `compile_config.cpp:72-74`. Both correct now.
- The lifecycle findings and their reachability: the generated setters at
  `runtime.cpp:616-619` with zero call sites, `snode_tree_allocs_` never erased
  (`llvm_runtime_executor.h:152`, written `:440`, read `:387`), the pool as a
  `DeviceAllocationGuard` released at teardown, the single Python caller at
  `export_lang.cpp:569-570`, and the statement that all of it is latent today.
  Both correct.
- The build seam and the `${arch}` / `${rtm_arch}` fault at
  `runtime_module/CMakeLists.txt:13` against `:8`, with `foreach(arch ...)` at
  `:29`. Both correct.
- The sm_60 downgrade. `llvm_context.cpp:504-506` gates on compute capability
  >= 60, `link_module_with_custom_cuda_library` (`:574-591`) gates on
  `get_custom_cuda_library_path` (`lang_util.cpp:18-28`) finding the file, and
  the only `install()` rule (`CMakeLists.txt:24`) sits in a function whose sole
  call site is commented out at `:36-42`. Both correct.
- The 32-bit inventories. I sampled across both and found no fault.

---

## 6. Escalations

Unresolved. Each needs a decision the project owner has not made. I am not
choosing any of them.

**Y1 — The pool's denominator is shared, and how much of it the root buffers
and ndarrays take is workload-dependent.** Section 4.1. On the non-mem-pool
path the capacity figures in both reports are upper bounds, not budgets. What
headroom the sizing rule should reserve is a design decision.

**Y2 — Which of the pair is an install-time factor, given that they are not
equally expensive to move.** Section 4.2. `taichi_max_num_snodes` needs a
build-time mechanism through `runtime_module/CMakeLists.txt:8`;
`device_memory_GB` needs only a write to `default_compile_config`
(`compile_config.h:110`) before `Program` construction. The plan's 8.1.2 treats
them as a pair; whether they are parameterised by the same mechanism is the
planner's call.

**Y3 — 8.1.2 has a different answer on CPU than on CUDA and AMDGPU.**
Section 4.3. On CPU the constant is the binding limit and raising it alone
works. Whether the replacement value is one number across the target set or one
per tier is undecided.

**Y4 — Everything already escalated by both reports stands**: the per-tree
bound versus the global index, `kMaxNumSnodeTreesLlvm`, the chunk constants,
`demote_dense_struct_fors`, the offline-cache format, the gfx numbering remedy,
whether the C++ core will call `Program::destroy_snode_tree`, the CMake install
variable, `taichi_listgen_max_element_size`, and the `.bc`/host value mismatch
with no detection path. I re-verified the source for each and add nothing.

---

## 7. Verdicts, and precisely what remains

**CORRECT — not yet, for either, and the gap is small.**

Pass B: correct except for the "reachable only from Python / hard-coded at C++
level" claim in 2.6, E5 and E5c, which is marked **[V]** and is false
(`compile_config.h:8, 71-72, 110`; `program.cpp:75`), and the "root buffers are
outside the pool" clause in 2.7(c) (section 4.1). Everything else reproduces.

Pass A: correct except for the `ptr2index` claim in 8.3 and E14, which is wrong
on CPU (`runtime.cpp:766-815`, kill only under `ARCH_cuda` / `ARCH_amdgpu` at
`:798-802`); the same "outside this pool" clause in 3.4; the "created either
way" sentence in 3.2 that its own next sentence contradicts; and the E1
phrasing that reads the same way as pass B's Python-only error. Its citation
apparatus is now clean and its round one fault is repaired.

**COMPLETE — not yet, for either, for one shared reason and two report-specific
ones.**

Shared: the pool's other consumers (4.1). Pass B alone: the CPU scope limit on
the pool, which pass A has and pass B does not (3.5a). Neither: the positive
statement of what binds on CPU (4.3).

**Consensus is not reached.** What remains, exhaustively:

1. Both correct "the root buffers are outside the pool" and record the chain
   `snode_tree_buffer_manager.cpp:15` → `llvm_runtime_executor.cpp:485` →
   `cuda_device.cpp:56-61` → `allocator.cpp:54-55` → `cuda_device.cpp:82-84` →
   `runtime.cpp:884-885`, and that it also carries ndarrays
   (`ndarray.cpp:61`) and argpacks (`argpack.cpp:18`).
2. Both correct the claim that `device_memory_GB` and
   `demote_dense_struct_fors` are Python-only, against `compile_config.h:8`,
   `:28`, `:71-72`, `:110` and `program.cpp:75`. Pass B must drop the **[V]**.
3. Pass A corrects `ptr2index` in 8.3 and E14 to pass B's arch-dependent form.
4. Pass A drops or fixes "on CUDA the pool is created either way" in 3.2.
5. Pass B adds the CPU scope limit on the pool that pass A has in 3.2.
6. Both state the CPU consequence for 8.1.2 in 4.3.
7. Cosmetic, at each report's discretion: pass A's E12 line numbers, pass B's
   `NodeManager` charge row.

None of that requires re-deriving anything either report has already
established. Items 1 and 2 are the only ones that change what the planner would
write down.

---

## 8. Cross-check against adversary2-03-1

`adversary2-03-1.md` did not exist when I wrote sections 0 to 7. It landed
before I finished and I read it then. Everything above was written blind to it.

### 8.1 Where we converge, independently

On every question the lead put to us we reached the same answer from separate
work.

- **Pool capacity 1020, overrun 4,190,248.** Both of us re-simulated
  `runtime.cpp:848-853` rather than reusing a report's number, and both of us
  hold that line 850 charges the pad to the caller. Both explore agents are
  right. My round one figures of 1023 and "40,960 bytes" are wrong and I
  withdraw them; 03-1's round one figure was right and it re-derived rather
  than reused it.
- **`ptr2index`.** Both of us went to `taichi_assert_format`
  (`runtime.cpp:766-815`), both found the thread kill only at `:800` under
  `ARCH_cuda` and `:802` under `ARCH_amdgpu`, and both concluded that pass B is
  right and pass A is wrong on the CPU build. 03-1 withdraws its own round one
  position, which pass A had adopted. I withdraw the "unconditional 131072 hot
  path" half of mine. Pass B's three-way split is the only reading the source
  carries.
- **The 2 GB card.** Both simulations give 1836 / 368 / 458 against
  `0.9 x 2,147,483,648`. My round one 351 and 211 do not reproduce and are not
  mutually consistent. Pass B's ruling stands.
- **The gfx closure.** Both of us re-derived the destroy-then-add cycle from
  `snode_tree_manager.cpp:11-16`, `:18-29`, `:49-51` and `program.cpp:235,
  559-567` and confirmed it diverges. Closing it rather than escalating it was
  right.
- **Pass A's CPU exemption on the pool.** Both verified, both call it material,
  both note that pass B lacks it.
- **Pass A's page-alignment caveat.** Both of us find it immaterial. 03-1 ran
  the simulation at five base offsets and got 1020 every time; I reached the
  same by comparing the 20,440-byte slack to the maximum 4,095-byte pad.
- **Pass A's "on CUDA the pool is created either way".** Both of us caught the
  same self-contradicting sentence in report A section 3.2.
- **The citation repair.** Both tested it independently and both found it
  genuine. Our extractors differ — 03-1 counts 156 and 170 resolvable
  citations, I count 416 and 504 individual line numbers because I also resolve
  the bare `:NNN` continuation form — and both return zero out of bounds.

### 8.2 Two findings I have that 03-1 does not, and where the source lands

**A. Root buffers, ndarrays and argpacks come out of the same 1 GiB pool
whenever `use_device_memory_pool()` is false.** My section 4.1. 03-1's section
7 closes with "I looked for a third pool … and found none", which is true and
is not the question. There is one pool with two classes of consumer. The chain
is `snode_tree_buffer_manager.cpp:15` → `llvm_runtime_executor.cpp:485` →
`cuda_device.cpp:56-61` → `allocator.cpp:54-55` → `cuda_device.cpp:82-84` →
`runtime.cpp:884-885`, which is `allocate_aligned(runtime_memory_chunk, …)`.
Both reports state the opposite explicitly (pass A section 3.4, pass B section
2.7(c)) and 03-1 does not challenge it. **The source supports me.** It matters
because it is the same one-frame-short miss round one faulted, and because it
makes every capacity row an upper bound on AMDGPU and on any CUDA device below
driver 11.2 — the baseline-tier case.

**B. `device_memory_GB` is reachable from C++.** My section 4.2. 03-1's
escalation Y2 states the reports' position rather than testing it: *"reachable
from C++ only as a `CompileConfig` default. Its only external setter is the
Python binding."* Against source, `default_compile_config` is
`extern TI_DLL_EXPORT CompileConfig` at `taichi/program/compile_config.h:110`,
defined at `taichi/util/lang_util.cpp:14`, with `device_memory_GB` a public
`float64` field at `compile_config.h:71`, and `Program::Program` copies the
global wholesale at `taichi/program/program.cpp:75`. The pybind binding at
`export_lang.cpp:265-266` hands out a reference to that same global; it is a
client, not the owner. A C++ driver sets the field before constructing a
`Program` with no new mechanism. **The source supports me**, and 03-1's Y4
repeats the same error for `demote_dense_struct_fors`
(`compile_config.h:28`, exposed at `export_lang.cpp:201-202`). This inverts the
relative cost of the two halves of the pair the plan's 8.1.2 records: the
constant is baked into per-arch bitcode by `runtime_module/CMakeLists.txt:8`
and needs a build mechanism, the pool knob needs none.

### 8.3 Findings only in 03-1's analysis, which I verified and accept

- **`llvm_program.cpp:112` should be `:113`.** Report A section 4.5 attributes
  `snode_cache_data.id = snodes[i]->id;` to line 112; that line is the
  declaration and the assignment is at 113. Verified. My own content sample
  covered line 110 and 112 but did not test what A attributed to 112.
- **Report B section 4.10's "exhaustive" heading misses two arrays.**
  `error_message_template[taichi_error_message_max_length]`
  (`runtime.cpp:587`) and
  `error_message_arguments[taichi_error_message_max_num_arguments]`
  (`:588`) are both sized by `inc/constants.h` values, at `constants.h:18` and
  `:19`. Verified. Report A carries both in its sections 2.2 and 9.
- **Report A section 9's fixed-array sweep misses `tls_buffer`.** Verified:
  `alignas(8) char tls_buffer[64]` at `runtime.cpp:1552` and `:1636`, plus
  `char tls_buffer[1]` at `:1435`. None scales with SNode count, so A's
  conclusion survives and only the word "exhaustive" does not.
- **Report B's listgen spans end one and three lines early.** Verified: the
  closing brace of `element_listgen_root` is at `runtime.cpp:1329` and of
  `element_listgen_nonroot` at `:1383`. Report A's `1282-1329` and `1331-1383`
  are right; report B's `1282-1328` and `1331-1380` are not.
- **`DeviceMemoryPool::allocate` discards its alignment argument**
  (`taichi/rhi/llvm/device_memory_pool.cpp:35-41`, forwarding to
  `allocate_raw_memory` and a bare `cuMemAlloc` at `:67`), and
  `CudaDevice::allocate_memory` is what `preallocate_memory` reaches
  (`cuda_device.cpp:21-47`, the call at `:28-29`). Verified. It closes pass A's
  declared assumption: nothing in this tree aligns the pool base. It does not
  change the answer, which is 1020 either way.
- **Pass B has the 64 MiB `data_list` example and pass A does not.** Verified
  against `runtime.cpp:1030` (`1024 * 16`) and the halving loop at `:652-655`.
  A fair point.

### 8.4 Where our verdicts differ, and why

03-1 records **CORRECT: pass B yes**, **COMPLETE: as a pair yes**, and lists
two corrections before consensus. I record **CORRECT: no for both** and
**COMPLETE: no for both**, and list six.

The difference is entirely the two findings in 8.2. Strike those and I agree
with 03-1 line for line, including its two corrections and its judgement that
they are one paragraph of work each. Add them and pass B has a claim marked
**[V]** that is false, both reports have an explicit statement about the pool's
occupancy that is false on the older-hardware path, and 03-1's own Y2 and Y4
carry the same error forward into the escalation list the planner reads.

One framing point where 03-1 is sharper than I was and I adopt it: it localises
pass B's CPU problem to the single unqualified sentence in escalation E5
("raising either alone is inert"), and notes that B's body text is correctly
scoped to CUDA throughout. That is more precise than calling it a whole-report
completeness gap. We converge on the remedy: pass B must state the CPU case,
because escalations are what the planner reads as the answer to 8.1.2.

The two corrections in 03-1's section 8 are a strict subset of my section 7.
Items 3 and 5 of my list are its items 1 and 2.
