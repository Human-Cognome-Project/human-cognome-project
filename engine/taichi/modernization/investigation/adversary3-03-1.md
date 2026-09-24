# Adversary 3, 03-1 — runtime and struct layer, round three

Adversary 1 of 2, round three, on territory 03. Judging the amended
`report-03-runtime-struct.md` (pass A, revision 3) and
`report-03b-runtime-struct.md` (pass B, revision 3) against the source at
`/opt/project/taichi`, and judging whether the amendment pass met the round-two
adversaries' objections.

I am a new adversary on this territory; I did not write `adversary-03-*.md` or
`adversary2-03-*.md`. I read all four of those files, both reports and both
notes files after doing my own source work, not before.

Everything below was derived by me from the source or from my own measurement.
Where I say a report is wrong I give the line that shows it. Where I say a
report is right I say how I tested it, not that I agree with it.

**One thing in this file is new evidence rather than re-reading: I measured the
development box.** Section 4. It answers half of escalation E15, which both
reports and both round-two adversaries left open.

---

## 0. The two verdicts

**CORRECT — pass A: yes. Pass B: yes, with one false sentence to strike.**

All seven claims the lead put to me are upheld. Every load-bearing number in
both reports reproduces against my own derivation, and every citation I sampled
lands on the line it claims. The defects I found are four, all textual, and none
of them changes a number, a capacity row, or a conclusion the planner would act
on. They are listed in section 6 and none required new investigation to find.

The one that is a plain error of fact is pass B's, stated twice: that
`CompileConfig::fit()` "does not touch this field" for
`demote_dense_struct_fors`. It does, at `compile_config.cpp:73`, which pass B
itself states correctly two sentences earlier. Section 3.3.

**COMPLETE — not quite, for both, on three items.**

1. **DX12.** Both reports state the pool exemption as a CPU fact. `arch_uses_llvm`
   (`taichi/rhi/arch.cpp:54-57`) carries five archs, and DX12 is the fifth. It is
   not CPU, it takes neither pool guard, and its device is a `CpuDevice`
   (`llvm_runtime_executor.cpp:159-164`). Neither report names it. Section 5.1.
2. **Pass A never states the positive CPU consequence.** It has the negative —
   the 1 GiB ceiling is not universal — and stops there. The sentence that the
   constant binds alone on CPU and that raising it alone works exists only in
   pass B. Plan section 6.1 now records it as fact on a single source in a paired
   method. Section 5.2.
3. **Half of escalation E15 was answerable and neither agent answered it.** Both
   declined to map the memory-pool branch onto the tiers in plan 5.2, correctly,
   because the tree does not record it. It is not in the tree; it is in the box,
   and the box is here. I measured it. Section 4.

**Consensus: reached on substance, not on text.** No claim in either report is
wrong in a way that moves a number. What remains is four corrections and one
escalation that my measurement narrows. Section 7 states each exactly. I would
not send this territory round again for them.

---

## 1. Method

I did the source work before reading any report or adversary file, so that what
I checked was not steered by what they claim. Specifically:

- I read the allocator, `taichi_assert_format`, `ptr2index`, both pool call
  sites and their guards, the eight-hop root-buffer chain, the `CompileConfig`
  global and its pybind bindings, the destroy path, and the CMake bitcode rule,
  each from the file rather than from a quotation.
- I re-derived the pool arithmetic from the allocator's own expression rather
  than reusing 1020 or 4,190,248 from anyone.
- I ran a mechanical bounds check over every backtick-quoted `path:line` and
  `path:a-b` reference in both reports, and hand-checked roughly seventy of them
  by printing the line.
- I measured the two GPUs in this machine through the CUDA driver API, because
  the one question both agents declared unanswerable is answerable that way.

I created three files, all under the session scratchpad, none in the project: a
citation checker, a C source file, and its binary. I modified no file except
this one. I read no other adversary's round-three file; none exists at the time
of writing.

---

## 2. Claims 1, 5, 6 and 7 — upheld, with the work shown

### 2.1 Claim 1. Pool capacity 1020, overrun 4,190,248, pad billed before the fit test

**UPHELD.** Derived independently.

`runtime.cpp:838-874` is `allocate_from_reserved_memory`. The three lines that
decide it, at 848-853:

```
848:    auto alignment_bytes =
849:        alignment - 1 - (preallocated_head + alignment - 1) % alignment;
850:    size += alignment_bytes;
851:    if (preallocated_head + size <= preallocated_tail) {
852:      ret = (Ptr)(preallocated_head + alignment_bytes);
853:      memory_chunk.preallocated_head += size;
```

Line 850 adds the pad to `size` before the fit test at 851 and before the head
advance at 853. With `A` the alignment and `r = head mod A`, line 849 gives
`A-1-(A-1) = 0` when `r = 0` and `A-1-(r-1) = A-r` otherwise. Ordinary round-up,
billed to the requester. So `head_after = align_up(head_before) + size`, which
is the recurrence everything else follows from.

`sizeof(ListManager)` is 1048616 and `1048616 mod 4096 = 40`, so from an aligned
base every request after the first is charged 4056 bytes of pad and advances the
head by 1052672, exactly 257 pages, returning the residue to 40.

| Quantity | Value |
|---|---|
| head after 1020 headers | 1,073,721,384 |
| pool, `device_memory_GB = 1` | 1,073,741,824 |
| slack at 1020 | 20,440 |
| head after 1021 | 1,074,774,056 |
| head after 1024 | 1,077,932,072 |
| overrun at the constant's value | **4,190,248** |

**Capacity 1020. Overrun 4,190,248.** Both figures reproduce exactly.

The base-alignment question is closed and both reports close it correctly. The
worst possible extra first-request pad is 4095 and the slack at 1020 is 20,440,
so the answer is 1020 at every base offset. The source reason is also right in
both: `DeviceMemoryPool::allocate` (`device_memory_pool.cpp:35-41`) takes an
`alignment` parameter at `:36` and never reads it, forwarding only
`allocate_raw_memory(size, managed)` at `:40`, which is a bare `CUDADriver::malloc`
at `:67`. I read all four functions. Nothing in this tree aligns the pool base.

I also re-derived the derived rows by stepping the allocator through the real
allocation order rather than multiplying:

| Case | Per-SNode charge | Fits in 1 GiB |
|---|---|---|
| element-list header only | 1,052,672 | 1020 |
| header + first 4 MiB chunk | 5,246,976 | 204 |
| pointer SNode, four headers + `NodeManager` | 4,214,784 | 254 |

The pointer figure is the one worth showing, because it is the one a reader
cannot check by inspection. Stepping `create<ListManager>` (`runtime.cpp:1005-1006`),
`create<NodeManager>` (`:1029-1030`) and the three lists in the `NodeManager`
constructor (`:658-663`) through `head_after = align_up(head_before) + size`
gives heads of 1048616, 1052728, 2105384, 3158056, 4210728, and the next group's
first request starts at 4214784. Steady-state charge 4,214,784. Both reports'
number.

### 2.2 Claim 5. `ptr2index` kills the thread only under CUDA and AMDGPU

**UPHELD, exactly as both reports now state it.**

`ptr2index` is `runtime.cpp:502-512`. The loop is at 504, the assert at 505
inside the loop and before the range test at 506, the fall-through return of -1
at 511.

`taichi_assert_runtime` (`:817-819`) forwards to `taichi_assert_format`
(`:766-815`). I read the whole function:

- `:780-781` — `if (!enable_assert || test != 0) return;`. The early return fires
  when the test **passes**. `enable_assert` is `constexpr bool ... = true` at
  `:339`.
- `:782` — on failure the error is recorded once under `if (!runtime->error_code)`.
- `:798-800` — `#if ARCH_cuda`, then `asm("exit;")` at `:800`.
- `:801-802` — `#elif ARCH_amdgpu`, then `asm("S_ENDPGM")` at `:802`.
- `:814` — `#endif`. The function then returns normally.

There is no CPU arm. On the host bitcode, built with `-D ARCH_x64`
(`runtime_module/CMakeLists.txt:8`, driven over `HOST_ARCH` at `:29`), neither
preprocessor branch is compiled in. Control returns to the loop, and line 506
evaluates `chunks[i] <= ptr` against `nullptr`, which holds, and
`ptr < chunks[i] + chunk_size`, which does not. No iteration matches, none
breaks, and the loop runs all 131072 before returning -1.

The assert is re-entered on every one of those iterations; after the first,
`runtime->error_code` is set, so the guard at `:782` skips the locked task and
each call is a return.

Both reports state the three-way split correctly. Pass A's revision 3 corrected
its own round-two adjudication, which was the one substantive error round two
found. It is fixed in section 8.3 and in E14.

### 2.3 Claim 6. The destroy path

**UPHELD in all four parts.**

**Four generated setters, zero call sites.** `STRUCT_FIELD_ARRAY`
(`runtime.cpp:69-77`) emits `S##_get_##F` and `S##_set_##F`, both taking `int i`.
It is applied to `element_lists`, `node_allocators`, `roots` and
`root_mem_sizes` at `:616-619`. My own grep for `LLVMRuntime_set_element_lists`,
`LLVMRuntime_set_node_allocators`, `LLVMRuntime_set_roots`,
`LLVMRuntime_set_root_mem_sizes`, and the bare `set_element_lists`,
`set_node_allocators`, `set_roots`, `set_root_mem_sizes`, over `.cpp`, `.h` and
`.py` with the build tree excluded, returns **nothing**. A JIT call by name would
show as a string literal and would have matched. `ambient_elements` gets no
accessor pair at all; it is absent from the `STRUCT_FIELD_ARRAY` list at
`:616-619`. Pass B's phrasing — no mechanism, not merely no call — is the right
one and pass A adopts it.

**No free path.** `allocate_from_reserved_memory` (`:838-874`) only ever
advances `preallocated_head` at `:853`. There is no release, no free list and no
head rewind anywhere in the function. Both reports' qualification is also right:
the pool as a whole is a `DeviceAllocationGuard` (`llvm_runtime_executor.cpp:587-604`),
so it goes at executor teardown.

**Writes and reads.** I enumerated them myself with a grep for the five array
names indexed, over `runtime_module/`. Writes: `:996`, `:997`, `:1005`, `:1029`,
`:1038`, all on the create path. Reads: `:1011`, `:1016`, `:1271`, `:1287`,
`:1288`, `:1334`, `:1336`, `:1429`, `:1692`, `:1723`, `:1740`, `:1784`,
`node_pointer.h:55, 76, 96`, `node_dynamic.h:30, 51, 74, 112`. That is exactly
pass A's section 4.2 list and exactly pass B's section 3.2 statement, with no
addition and no omission on either side.

**The dangling allocation.** `snode_tree_allocs_` has exactly three occurrences
in the tree: declared `llvm_runtime_executor.h:152`, written `:440`, read
`:387`. Nothing erases it. `SNodeTreeBufferManager::destroy`
(`snode_tree_buffer_manager.cpp:20-24`) does free the underlying allocation at
`:22` and erase its own map entry at `:23`. So the entry left in
`snode_tree_allocs_` is dangling from the moment of destroy. Confirmed.

One precision fault, in **both** reports, on the words "on a recycled tree id".
See section 6.4. It does not disturb the finding.

### 2.4 Claim 7. Citation repair and self-audit

**UPHELD.** Tested two ways.

**Mechanically.** I wrote my own extractor for backtick-quoted `path:line` and
`path:a-b` forms, resolved each path against the tree, and compared every number
to the real file length.

| Report | Resolvable citations | Past end of file | Unresolvable paths |
|---|---|---|---|
| `report-03-runtime-struct.md` | 256 | 0 | 0 |
| `report-03b-runtime-struct.md` | 342 | 0 | 0 |

My counts differ from both round-two adversaries' because our regexes differ; my
form does not resolve the bare `:NNN` continuation. The result is the same:
nothing out of range. Pass A's systematic +54 / +43 fault is gone.

**By hand.** Bounds checking cannot catch an in-range citation pointing at the
wrong line, so I printed source for roughly seventy references, weighted to the
two files that carried the fault and to everything either round-two adversary
disputed. Every one landed. In particular:

- `taichi/struct/snode_tree.cpp` is 41 lines. `get_snodes_to_root_id_impl` 6-13,
  constructor 17-20, `check_tree_validity` 22-32, `get_snodes_to_root_id` 34-39.
  Pass A's section 5.1 table is exact.
- `taichi/runtime/gfx/snode_tree_manager.cpp` is 54 lines.
  `materialize_snode_tree` 11-16 with `add_root_buffer` at 14 and the
  `push_back` at 15; `destroy_snode_tree` 18-29 with the scan at 20-24, the
  `TI_ERROR` at 25-27 and the reset at 28; `get_field_in_tree_offset` 31-47 with
  its `TI_ASSERT_INFO` at 34-38; `get_snode_tree_device_ptr` 49-51. Both reports
  are exact.
- `runtime.cpp` throughout: `69-77`, `85-88`, `339`, `424-425`, `427-428`,
  `502-512`, `562-563`, `567-569`, `616-619`, `743-747`, `749-750`, `766-815`,
  `817-819`, `823-835`, `838-874`, `848-853`, `864-869`, `872`, `925-941`,
  `962-971`, `996-997`, `1005-1006`, `1029`, `1038`.
- `llvm_runtime_executor.cpp`: `28-32`, `49`, `386-389`, `391`, `399-400`,
  `402-410`, `412-413`, `417`, `419-420`, `440`, `442-444`, `476-491`,
  `607-632`, `620`, `719-721`, `758-761`.
- `llvm_program.cpp:45-56`, `:58-65`, `:61`, `:67-76`; `program.cpp:75`, `:77`,
  `:144`, `:214-236`, `:232`, `:234`, `:235`, `:238-255`, `:249`, `:252`,
  `:257-259`, `:559-567`; `snode.cpp:12`, `:220`, `:230-233`;
  `snode.h:348-350`; `struct_llvm.cpp:247`, `:266`; `constants.h:12`, `:13`,
  `:16`, `:28`; `compile_config.h:8`, `:28`, `:71-72`, `:110`;
  `compile_config.cpp:63`, `:64`, `:67-76`; `lang_util.cpp:14`;
  `export_lang.cpp:201-202`, `:208-210`, `:264-267`;
  `cuda_device.cpp:21-48`, `:28-29`, `:50-78`, `:56`, `:57-58`, `:60-61`, `:68`,
  `:80-90`, `:82-84`; `amdgpu_device.cpp:55-78`, `:80-90`;
  `cpu_device.cpp:44-51`; `device_memory_pool.cpp:27-33`, `:35-41`, `:43-51`,
  `:53-92`, `:67`; `allocator.cpp:33-58`, `:54-55`, `:60-68`;
  `cuda_context.cpp:35-50`, `:52-53`; `unified_allocator.cpp:68`;
  `snode_tree_buffer_manager.cpp:12-18`, `:20-24`;
  `runtime_module/CMakeLists.txt:3`, `:8`, `:9`, `:10`, `:13`, `:24`, `:29-31`,
  `:36-42`.

**The corrections claimed in the revision records are in the text, not
footnoted.** I checked each of the six items adversary2-03-2 listed as remaining
and each of the two adversary2-03-1 listed. Items 1 through 5 and item 7 of the
former, and both of the latter, are done. Item 6 — that **both** reports state
the CPU consequence for open question 8.1.2 — is done in pass B and not in pass
A. Section 5.2.

Two span-end slips survive, both in pass A and both trivial. `struct_llvm.cpp:269-270`
for the `root_size` assignment: 269 is the assignment, 270 is blank. And
`amdgpu_device.cpp` "does not branch on `use_memory_pool` at all … calls
`allocate_with_cache` unconditionally at `:62-63`": the first clause is exactly
true, but there is a branch, on `params.host_read || params.host_write` at `:59`,
whose other arm is `TI_NOT_IMPLEMENTED` at `:60`. The sole caller passes both
false (`llvm_runtime_executor.cpp:480`), so the conclusion holds and the word
"unconditionally" is the only thing overreaching.

---

## 3. Claims 2 and 3 — the two that the plan now states as fact

### 3.1 Claim 2, the CPU exemption. UPHELD, and it is correct as far as it goes

Every link verified from source.

**Two call sites, both arch-gated.** My own grep over the tree for
`preallocate_runtime_memory` returns the declaration at
`llvm_runtime_executor.h:105`, the definition at `:607`, and exactly two calls:

- `:413`, inside `initialize_llvm_runtime_snodes`, under
  `if (config_.arch == Arch::cuda && use_device_memory_pool() && !all_dense)` at
  `:412`.
- `:721`, inside `materialize_runtime`, under
  `if (config_.arch == Arch::cuda || config_.arch == Arch::amdgpu)` at `:719`
  and `if (!use_device_memory_pool())` at `:720`.

**The flag is assigned only on the CUDA branch.** My grep for
`use_device_memory_pool` returns the accessor at `llvm_runtime_executor.h:78-79`,
the initialiser `bool use_device_memory_pool_ = false;` at `:162`, exactly one
assignment at `llvm_runtime_executor.cpp:49`, and three reads at `:412`, `:485`
and `:720` plus one in `c_api/`. Line 49 sits inside `if (config.arch == Arch::cuda)`
at `:39`, in the arm reached only when the CUDA API and a device are both present.

**Fall-through on the host allocator, and no ceiling.** `runtime_initialize_memory`
(`runtime.cpp:962-971`) is the only writer of `runtime_memory_chunk` and is
called only from `preallocate_runtime_memory` (`llvm_runtime_executor.cpp:629-631`).
With neither guard passing, `preallocated_size` stays 0 and
`LLVMRuntime::allocate_aligned` takes the `host_allocator` branch at
`runtime.cpp:834`. That reaches `host_allocate_aligned`
(`llvm_runtime_executor.cpp:28-32`), `HostMemoryPool::allocate`, and
`UnifiedAllocator::allocate`, which at `unified_allocator.cpp:64-68` sizes a
fresh mmap'd chunk as `std::max(size, default_allocator_size)` whenever the
current chunk cannot serve. There is no ceiling and no pool.

**So the constant binds alone.** `element_lists` is `[taichi_max_num_snodes]`
(`runtime.cpp:567`) and the write at `:1005` is unchecked, so on the no-pool
path the array bound is the only limit and raising it alone is effective, at
whatever host memory will carry at the ~5.2 MiB per populated sparse SNode both
reports compute. Confirmed.

**Where it is under-inclusive.** The exemption is broader than "CPU". See
section 5.1.

### 3.2 Claim 3, `device_memory_GB` and the cost asymmetry. UPHELD

Every link verified.

- `struct CompileConfig` at `taichi/program/compile_config.h:8` is a plain struct
  with public fields. `demote_dense_struct_fors` is `bool` at `:28`;
  `device_memory_GB` and `device_memory_fraction` are `float64` at `:71-72`.
- `extern TI_DLL_EXPORT CompileConfig default_compile_config;` at
  `compile_config.h:110`; the definition is `taichi/util/lang_util.cpp:14`.
- `Program::Program` copies it wholesale. `taichi/program/program.cpp:74`
  binds `auto &config = compile_config_;`, `:75` is
  `config = default_compile_config;`, `:76` sets the arch, `:77` calls
  `config.fit()`.
- The pybind side is a client. `export_lang.cpp:208-210` is `def_readwrite` on
  the two memory fields, `:201-202` on `demote_dense_struct_fors`, and
  `:264-267` is `m.def("default_compile_config", [&]() -> CompileConfig & { return default_compile_config; }, py::return_value_policy::reference)` —
  a reference to the same global.

So a C++ driver writes `taichi::lang::default_compile_config.device_memory_GB`
before constructing a `Program`, with no new mechanism and no Python. Both
fields are `float64`, so fractional values are legal. Both reports now say this
and both are right.

**The asymmetry is real and the expensive half is the constant.**
`taichi_max_num_snodes` is a bare `constexpr int` at `taichi/inc/constants.h:12`
with no `#ifndef`, no `#cmakedefine` and no override of any kind — I read the
whole header. `runtime.cpp` includes it at `:25`, and the bitcode is produced by
a standalone clang command at `runtime_module/CMakeLists.txt:8` whose only flags
are `-D "ARCH_${rtm_arch}"` and `-I ${PROJECT_SOURCE_DIR}`. It is an
`add_custom_target` (`:6-11`), so it inherits nothing from the project's compile
definitions. There is no build-time mechanism to hang a parameter on. The pool
knob needs none. Confirmed in both directions.

The `${arch}` / `${rtm_arch}` fault is also real: `:8` reads the function
parameter declared at `:3`, `:13` reads `arch`, which is the caller's `foreach`
variable at `:29`. Both reports flag it, neither chases it, which is right.

### 3.3 One false sentence in pass B, in this same area

Pass B, section 2.4, on `demote_dense_struct_fors`:

> `Program::Program` copies that global wholesale at
> `taichi/program/program.cpp:75` before calling `config.fit()` at `:77`, **which
> does not touch this field** (`compile_config.cpp:67-76`).

**It does.** `CompileConfig::fit()` is `compile_config.cpp:67-76`, and `:72-73`
is

```
72:  if (arch_uses_spirv(arch)) {
73:    demote_dense_struct_fors = true;
```

Pass B states that forcing correctly two sentences earlier in the same
paragraph, and again in E5c. So the report contradicts itself within one
paragraph, and repeats the false half verbatim in its revision record at section
6.1. The sentence is true of `device_memory_GB`, which `fit()` genuinely does
not touch, and false of the field it is attached to.

Consequence: none downstream. No capacity row, no escalation and no conclusion
in pass B depends on it, and a reader is given the correct fact twice elsewhere.
But it is an error of fact in an amendment paragraph, and pass A does not have
it — pass A's E1 says only that `Program` copies the global and then calls
`fit()`, which is right. Strike the four words.

---

## 4. Claim 4 — the root-buffer finding, and testing the restraint

### 4.1 The chain. UPHELD, every hop read

I read all eight hops from the file rather than from either report.

| # | Site | What it does |
|---|---|---|
| 1 | `llvm_runtime_executor.cpp:419-420` | `snode_tree_buffer_manager_->allocate(rounded_size, tree_id, result_buffer)`, size from `iroundup(root_size, taichi_page_size)` at `:417` |
| 2 | `snode_tree_buffer_manager.cpp:15` | `runtime_exec_->allocate_memory_on_device(size, result_buffer)` |
| 3 | `llvm_runtime_executor.cpp:476-491` | `llvm_device()->allocate_memory_runtime(...)`, `use_device_memory_pool()` passed as the `use_memory_pool` field at `:485`, with `host_write` and `host_read` both false at `:480` |
| 4 | `cuda_device.cpp:50-78` | branches at `:56`. `use_memory_pool` true takes `malloc_async` at `:57-58`. False takes `allocate_with_cache` at `:60-61` |
| 5 | `device_memory_pool.cpp:27-33` | `allocator_->allocate(device, params)` at `:32` |
| 6 | `allocator.cpp:33-58` | on a cache miss, `device->allocate_llvm_runtime_memory_jit(params)` at `:54-55` |
| 7 | `cuda_device.cpp:80-90` | JIT-calls `runtime_memory_allocate_aligned` at `:82-84` |
| 8 | `runtime.cpp:879-886` | `runtime->allocate_aligned(runtime->runtime_memory_chunk, size, alignment)` at `:884-885` |

Step 8 is the same bump allocator the `ListManager`s come from. The finding is
correct.

The three arch statements are also correct.
`AmdgpuDevice::allocate_memory_runtime` (`amdgpu_device.cpp:55-78`) has no
`use_memory_pool` test and reaches `allocate_with_cache` at `:62-63` for any
request that does not ask for host access — and hop 3 asks for none.
`CpuDevice::allocate_memory_runtime` (`cpu_device.cpp:44-51`) calls
`allocate_memory` at `:47` and never reaches `allocate_with_cache`. And ndarrays
(`ndarray.cpp:61`) and argpacks (`argpack.cpp:17-18`) enter at hop 3 through
`Program::allocate_memory_on_device` (`program.h:245-248`).

### 4.2 The restraint. Correct about the tree, and answerable outside it

Both agents declined to say which tier lands on which branch. Both gave the same
reason: the gate is
`driver >= 11.2 AND CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`
(`cuda_context.cpp:35-50`, the flag set at `:52-53`), and neither is recorded in
the repository.

**That reason is correct.** I confirmed it. Nothing in this tree records the
answer for any card, the attribute is queried at runtime, and the driver version
is an environment fact. Under plan section 10.7 this is ENVIRONMENTAL, and
declining to assert it from source was the right call. Adversary2-03-2's claim
that it lands on "exactly the baseline-tier hardware" did not follow from the
tree, and pass B was right to refuse it on that ground.

**But the question was answerable, and the answer was in the room.** Plan
section 8.2 records that both adversaries on territory 05 measured this same
development box. Nothing stopped territory 03 from doing the same. I did.

I queried the CUDA driver API directly, through `dlopen` on `libcuda.so.1`, for
attribute 115, which `taichi/rhi/cuda/cuda_driver.h:38` defines as
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`:

| Device | Card | Compute capability | Attribute 115 |
|---|---|---|---|
| 0 | NVIDIA GeForce GTX 1070 | sm_61 | **1** |
| 1 | NVIDIA GeForce GTX 750 Ti | sm_50 | **0** |

Driver CUDA version 12.2, `nvidia-smi` driver 535.309.01, both cards on the same
driver. I re-ran the 750 Ti under `CUDA_VISIBLE_DEVICES=1` so that it was device
index 0, because `CUDAContext::CUDAContext` hard-codes
`driver_.device_get(&device_, 0)` at `cuda_context.cpp:22` and therefore only
ever queries index 0. It reports 0 in both positions.

**What this settles:**

- The **driver half of the gate is satisfied on this box for both cards.** 12.2
  is far above 11.2, so the `else` arm at `cuda_context.cpp:41-50` that forces
  the flag to 0 is not the reason anything fails here.
- The **device half decides it, and it splits the two tiers.** On the GTX 1070,
  `supports_mem_pool()` is true, so `use_device_memory_pool()` is true, so root
  buffers, ndarrays and argpacks take `malloc_async` at `cuda_device.cpp:57-58`
  and are genuinely outside the pool. On the GTX 750 Ti it is false, so they
  come out of the same 1 GiB bump allocator as the `ListManager`s.
- **The baseline tier is on the shared-denominator path and the mid tier is
  not.** Adversary2-03-2's tier mapping was right about this box; its reasoning
  from the tree was not, and pass B was right to reject the reasoning. Both
  things are true.
- **This does not expire with a driver upgrade.** The driver is already CUDA
  12.2. Both reports frame the gate as "below driver 11.2 **or** the device lacks
  the attribute". On this box only the second disjunct is live, and it is a
  hardware-generation fact about Maxwell. Under plan 10.7 that is still
  ENVIRONMENTAL — it expires if the card is replaced — but the external thing
  that would have to change is the card, not the driver, and neither report says
  so.

**Consequence for escalation E15.** Two thirds of it is now answered and one
third is not:

- GTX 750 Ti, baseline tier: shared denominator. The 1020 / 204 / 254 / 85 / 125
  rows are upper bounds there, not budgets.
- GTX 1070, mid tier: exclusive denominator for `ListManager`s. Those rows are
  exact there, with the further note that on this branch the pool is created only
  at `llvm_runtime_executor.cpp:412-413`, so a fully `all_dense` configuration
  gets no pool at all.
- RTX 3060, upper tier: not in this machine, not measured, still open.

The remaining half of E15 — how much headroom a sizing rule must reserve for
root, ndarray and argpack storage on the baseline path — is workload-dependent
and stays open. I propose no figure and call nothing surplus.

I did not extend this to AMDGPU, which needs no measurement: the flag is never
assigned outside the CUDA branch, so `use_device_memory_pool()` is false there
by construction.

### 4.3 The related precision point, and pass A carries it

Pass A section 5.5 records that "freed" is path-conditional on the same split:
`CudaDevice::dealloc_memory` takes `mem_free_async` only when
`info.use_memory_pool` is set, and otherwise returns the block to
`CachingAllocator`'s free list, never to the bump head at `runtime.cpp:853`. I
verified `info.use_cached = true` is assigned unconditionally at
`cuda_device.cpp:68`, so the cached branch is the one taken. Pass A has this and
pass B does not, though pass B's "no free path in the bump allocator" is not
wrong. On the measurement above, the baseline tier is the one where that branch
is taken.

---

## 5. Still missing

### 5.1 DX12 is a fifth LLVM arch with no pool, and neither report names it

Both reports state the exemption as a CPU fact. Pass A: "On CPU neither call
site fires" (section 3.2). Pass B: "On the CPU path neither guard passes"
(section 2.6).

`arch_uses_llvm` (`taichi/rhi/arch.cpp:54-57`) is

```
54: bool arch_uses_llvm(Arch arch) {
55:   return (arch == Arch::x64 || arch == Arch::arm64 || arch == Arch::cuda ||
56:           arch == Arch::dx12 || arch == Arch::amdgpu);
57: }
```

Five archs. The pool guards admit two. `x64` and `arm64` are covered by the word
CPU. **DX12 is not**, and it is not a CPU arch: `arch_is_cpu(Arch::dx12)` is
false, `LlvmRuntimeExecutor`'s constructor gives it a `cpu::CpuDevice` at
`llvm_runtime_executor.cpp:159-164` with a `FIXME: add dx12 device`, and
`DX12_ARCH` is in the bitcode arch loop at `runtime_module/CMakeLists.txt:29`,
so a `runtime_dx12.bc` is built with its own baked constant.

So on DX12 the pool does not exist, allocation falls through to the host
allocator, and `taichi_max_num_snodes` binds alone — the same conclusion as CPU,
for the same reason, on a target that is not CPU.

The positive statements in both reports ("a CUDA and AMDGPU fact") are exact and
complete. Only the enumeration of the exception is short. Adversary2-03-2 flagged
this in round two (its section 3.5a) and excused it as agent 04's territory. It
was not added by either amendment. I note it now because plan section 5.2's
CORRECTION of 2026-09-09 explicitly puts DirectX 12 on the LLVM spine, which it
did not do when round two ran, and because plan section 6.1 currently records the
exemption as "the CPU path" alone.

This is one clause in each report, not new work.

### 5.2 Pass A never states the positive CPU consequence

Pass A has the negative. Section 3.2: "The 1 GiB ceiling is a CUDA and AMDGPU
fact, not a universal one." Its summary item 4 and its E1 are both correctly
arch-scoped to CUDA and AMDGPU.

It nowhere says that on the no-pool path the constant is the binding limit and
that raising it alone works. I grepped pass A for the phrase in every form: two
hits for "on CPU", neither of them the positive statement, and no hit for "binds
alone", "raising it alone" or "is effective". Pass B has it twice, at its lines
476 and 1176.

Adversary2-03-2's remaining item 6 was that **both** reports state it. Only one
does. Plan section 6.1 now records it as a verified fact, and it is
single-sourced in a method whose whole point (plan 9.1) is that two agents cover
the same ground independently. It is not wrong and it is not disputed — I
verified the mechanism myself in section 3.1 above — but the planner should know
that the pair did not both reach it.

### 5.3 Pass A section 3.2 carries a stale parenthetical

"**Scope limit, which neither adversary states.**" That was true of round one.
Both round-two adversaries state it, and adversary2-03-1 section 5.1 credits
pass A for it by name. Cosmetic, in a paragraph revision 3 otherwise rewrote.

---

## 6. Newly wrong

Four items. None changes a number.

**6.1 Pass B: `fit()` "does not touch this field".** Section 3.3 above. False at
`compile_config.cpp:72-73`, contradicted by pass B's own adjacent sentence, and
repeated in its section 6.1 revision record.

**6.2 Pass A: `amdgpu_device.cpp` "unconditionally".** Section 2.4 above. There
is a branch at `:59` on `host_read || host_write`; the sole caller passes both
false. The clause "does not branch on `use_memory_pool` at all" is exactly right
and is the one that matters.

**6.3 Pass A: `struct_llvm.cpp:269-270`.** The `root_size` assignment is `:269`
alone; `:270` is blank. Pass B cites `:269`.

**6.4 Both reports: "hand out on a recycled tree id".** Pass A section 5.5 and
pass B section 3.2 both say the dangling `snode_tree_allocs_` entry is handed out
by `get_snode_tree_device_ptr` **on a recycled tree id**. Both reports also say,
in their gfx sections (A 7.2, B 3.4), that the LLVM path has no equivalent to the
gfx divergence precisely because a recycled id **overwrites** the map entry. Both
statements cannot stand unqualified together.

The source resolves it, and both are right on different paths:

- `Program::add_snode_tree` (`program.cpp:238-255`) takes the id at `:240` and
  then branches at `:243`. With `compile_only == false` it calls
  `materialize_snode_tree`, which is `llvm_program.cpp:67-76` and reaches
  `initialize_llvm_runtime_snodes` at `:74-75`, which writes
  `snode_tree_allocs_[tree_id]` at `llvm_runtime_executor.cpp:440`. **The stale
  entry is overwritten.**
- With `compile_only == true` it calls `compile_snode_tree_types`
  (`llvm_program.cpp:58-65`), which ends at `cache_field` and never reaches
  `initialize_llvm_runtime_snodes`. **The id is recycled and the map is not
  written**, so the dangling entry survives under a live tree id. That path has
  real callers: `cpp_examples/aot_save.cpp:18` and
  `tests/cpp/aot/dx12/aot_save_load_test.cpp:28` both pass `compile_only=true`.

The accurate statement is that the entry dangles from the moment of destroy, is
overwritten if the id is re-materialised, and is not overwritten if the id is
re-allocated compile-only. Both reports' version is true on one of the two
branches and neither distinguishes them.

Related, and smaller: neither report names a caller of
`get_snode_tree_device_ptr`, so the "hand out" is asserted rather than
established. Standing instruction 6 is about exactly this. The claim survives —
there are two C++ callers, `taichi/program/field_info.cpp:26` and
`taichi/program/texture.cpp:235`, both reaching it through
`Program::get_snode_tree_device_ptr` (`program.h:232-233`) — but neither report
did that step.

---

## 7. What remains, exhaustively

Four textual corrections and one escalation narrowing. No new investigation.

1. **Pass B**, section 2.4 and section 6.1: strike "which does not touch this
   field" for `demote_dense_struct_fors`. `fit()` sets it true for SPIR-V archs
   at `compile_config.cpp:73`, as pass B states correctly elsewhere.
2. **Both**, in the pool-scope paragraph: name DX12. `arch_uses_llvm`
   (`taichi/rhi/arch.cpp:54-57`) carries five archs; the pool guards admit two;
   DX12 is neither CPU nor pooled, and gets a `CpuDevice` at
   `llvm_runtime_executor.cpp:159-164`.
3. **Pass A**: state the positive CPU consequence that pass B states — that
   where there is no pool the constant binds alone and raising it alone is
   effective. Pass A already holds every fact it needs.
4. **Both**: qualify "hand out on a recycled tree id" per section 6.4, and name
   a caller.
5. **Escalation E15 narrows.** Section 4.2. On this development box the baseline
   tier is on the shared-denominator path and the mid tier is not, measured, not
   inferred. What stays open is the RTX 3060, which is not in this machine, and
   the headroom question, which is workload-dependent.

Cosmetic, and I would not hold anything for them: pass A's
`struct_llvm.cpp:269-270`, pass A's "unconditionally" on `amdgpu_device.cpp`,
pass A's stale "which neither adversary states", and pass B's `NodeManager`
charge row, which adversary2-03-2 already showed to be off by offsetting 16 bytes
that cancel.

**My verdict on consensus.** Reached on substance. All seven claims put to me are
upheld and I found nothing that moves a number, a capacity row, a footprint, or
an escalation's direction. The four items above are one or two sentences each and
do not require either report to re-derive anything. Item 5 is mine, not theirs,
and it adds evidence rather than correcting them.

---

## 8. Escalations

Unresolved. Each needs a decision the project owner has not made. I choose none
of them and per standing instruction 3 I call nothing surplus.

**Z1 — The measured tier split on the memory-pool branch, and what a sizing rule
must reserve.** Section 4.2. On this box the GTX 750 Ti does not report
`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED` and the GTX 1070 does, on the same
CUDA 12.2 driver. So on the baseline tier the SNode tree root buffers, ndarrays
and argpacks share the 1 GiB bump allocator with the `ListManager`s and every
capacity row is an upper bound; on the mid tier they do not. How much headroom a
sizing rule for item 8.1.2 must reserve is workload-dependent. The RTX 3060 is
unmeasured. This narrows E15 in both reports rather than replacing it.

**Z2 — The gate is a hardware generation, not a driver version, on this
hardware.** Section 4.2. Both reports and both round-two adversaries describe the
branch as "driver below 11.2 or the device lacks the attribute". The driver here
is CUDA 12.2, so only the device attribute is live, and it is a Maxwell fact. If
the planner reads the finding as expiring on a driver upgrade, it does not. Under
plan 10.7 it remains ENVIRONMENTAL, but the external thing that must change is
the card.

**Z3 — DX12 sits on the no-pool side of item 8.1.2 and is unowned here.**
Section 5.1. Plan 5.2's correction puts DirectX 12 on the LLVM spine. It takes
neither pool guard, so the constant binds alone there as on CPU, and it has its
own `runtime_dx12.bc` with its own baked constant. Territory 04 owns the backend;
I am flagging the arithmetic consequence, not chasing the backend.

**Z4 — `CUDAContext` uses device index 0 only.** `cuda_context.cpp:22` is
`driver_.device_get(&device_, 0)`, with no configuration path to any other index
that I found in this territory. Plan 5.1a records that the owner has run both
cards concurrently, which outranks any code-reading conclusion and which I am not
disputing; the mechanism by which a second card is selected is outside my
territory and outside this file. I record the line because the tier split in Z1
depends on which device the attribute is read from, and because the
install-time configuration agent in plan 5.1a will have to select one.

**Z5 — Everything both reports already escalated stands.** I re-verified the
source for each and add nothing: the per-tree assertion against the
program-global `SNode::id`; `kMaxNumSnodeTreesLlvm = 512` bounds-checked nowhere;
`ListManager::max_num_chunks` and the element-list chunk count;
`demote_dense_struct_fors` as a further footprint input; the offline-cache format
under an id width change; the gfx numbering remedy; whether the C++ core will
call `Program::destroy_snode_tree`; the CMake install variable fault; the
`.bc`-versus-host constant mismatch with no detection path;
`taichi_listgen_max_element_size`; and the `ptr2index` bound on the CPU
deactivate path.

---

## 9. Divergence from adversary3-03-2

`/opt/project/taichi/modernization/investigation/adversary3-03-2.md` does not
exist at the time of writing. No divergence section can be recorded.
