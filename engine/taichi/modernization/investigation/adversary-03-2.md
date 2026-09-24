# Adversary 03-2 — Runtime and struct layer

Second adversary on territory 03. Judging
`report-03-runtime-struct.md` (pass A) and `report-03b-runtime-struct.md`
(pass B) against the source at `/opt/project/taichi`.

Everything below was re-derived from the source or from the compiler by me.
Where I say a report is wrong I give the line I read.

---

## 0. Verdicts up front

**CORRECT: qualified yes.** Every load-bearing measurement in both reports
reproduces. I compiled the real `runtime.cpp` myself and confirmed all ten
`sizeof`/`offsetof` figures both passes quote. The structural findings — the
per-tree assertion versus the global array index, the unbounded
`kMaxNumSnodeTreesLlvm`, the `ListManager` dominance — all hold. Two content
errors and one systematic citation fault survive, listed in sections 5 and 6.

**COMPLETE: no.** Both passes stopped at the allocation *call site* and never
followed the allocation into the pool it comes from. The pool is
`config_.device_memory_GB`, defaulting to **1 GB**
(`taichi/program/compile_config.cpp:63`). That, not the constant and not the
card, is what binds first on every CUDA target. Neither report mentions it.
Both also left the lazily-touched chunks unquantified, and those chunks are
four times the size of the tables both reports centre on. Section 7.

**MAJOR REVISION REQUIRED: yes, for pass A. Yes, narrower, for pass B.**
Pass A must have its `snode_tree.cpp` and `gfx/snode_tree_manager.cpp`
citations regenerated; every one of them is wrong. Pass B must withdraw the
sentence that the card is the binding limit. Both must absorb section 7.

---

## 1. Attack item 1 — the convergent claim against the plan. It holds.

I verified it independently rather than accepting the convergence.

`ListManager` is declared at
`taichi/runtime/llvm/runtime_module/runtime.cpp:426`, and opens with a flat
table:

```
427:  static constexpr std::size_t max_num_chunks = 128 * 1024;
428:  Ptr chunks[max_num_chunks];
```

131072 pointers is 1,048,576 bytes, present in every instance whether or not a
chunk is touched.

I measured the sizes myself by piping the real translation unit through the
compiler, creating no file:

```
(cat taichi/runtime/llvm/runtime_module/runtime.cpp; printf '<static_asserts>') \
  | clang++ -x c++ - -std=c++17 -fno-exceptions \
      -I /opt/project/taichi \
      -I /opt/project/taichi/taichi/runtime/llvm/runtime_module \
      -fsyntax-only -w
```

It compiled clean with every one of these asserted:

| Quantity | Bytes |
|---|---|
| `sizeof(LLVMRuntime)` | 35256 |
| `sizeof(ListManager)` | 1048616 |
| `sizeof(NodeManager)` | 56 |
| `sizeof(Element)` | 64 |
| `sizeof(StructMeta)` | 72 |
| `sizeof(PhysicalCoordinates)` | 48 |
| `sizeof(RandState)` | 20 |
| `offsetof(LLVMRuntime, element_lists)` | 8296 |
| `offsetof(LLVMRuntime, node_allocators)` | 16488 |
| `offsetof(LLVMRuntime, ambient_elements)` | 24680 |
| `offsetof(LLVMRuntime, temporaries)` | 32872 |

Both passes' tables are exactly right. Pass A's offset pair (8296, 32872) and
pass B's triple (8296, 16488, 24680) are the same 24576-byte span.

The allocation is real and eager. `runtime_initialize_snodes`
(`runtime.cpp:986-1017`) does, at `runtime.cpp:1003-1006`:

```
for (int i = root_id; i < root_id + num_snodes; i++) {
  // TODO: some SNodes do not actually need an element list.
  runtime->element_lists[i] =
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

`LLVMRuntime::create<T>` (`runtime.cpp:606-612`) calls
`allocate_aligned(runtime_memory_chunk, sizeof(T), 4096, request=true)`, so the
full 1,048,616 bytes is committed at materialisation. `num_snodes` is
`(int)snode_metas.size()` (`llvm_runtime_executor.cpp:443-444`), and
`snode_metas` is built from `struct_compiler.snodes` with no filtering
(`taichi/runtime/program_impls/llvm/llvm_program.cpp:108-118`), which
`StructCompiler::collect_snodes` (`taichi/struct/struct.cpp:7-13`) fills by
full pre-order traversal. So `place` nodes are included, exactly as upstream's
own TODO at `runtime.cpp:1004` admits.

`runtime_NodeAllocator_initialize` (`runtime.cpp:1026-1031`) adds a
`NodeManager` per garbage-collectable SNode, and the `NodeManager` constructor
(`runtime.cpp:643-664`) creates three further `ListManager`s at
`runtime.cpp:658`, `661` and `663`. `is_gc_able` is pointer or dynamic only
(`taichi/ir/snode_types.cpp:21-23`), called at
`llvm_runtime_executor.cpp:447`.

**Ruling: the plan's section 6.1 finding is incomplete as written, and the two
passes are right about why.** Section 6.1 records the three arrays at
`runtime.cpp:567-569` as "a footprint". They are 24576 bytes. The footprint the
constant actually gates is between 1.0 and 4.0 MiB per SNode, three orders of
magnitude larger. The planner does have to correct the plan. But the correction
the plan needs is larger than either report supplies — see section 7.

---

## 2. Attack item 2 — the magnitude "disagreement". It is not one.

There is no arithmetic conflict. The two passes headline different scenarios
and both computed their own scenario correctly.

| Scenario | Per SNode | × 1024 | Source |
|---|---|---|---|
| Element list only | 1,048,616 | 1,073,782,784 = 1024.04 MiB | pass A headline |
| Element list + NodeManager + its 3 lists | 4,194,520 | 4,295,188,480 = 4.00 GiB | pass B headline |

Pass A's own section 2.4 states the 4,194,520-byte per-gc-able-SNode figure
before quoting the 1024 MiB total, so pass A already holds pass B's number and
simply chose the conservative headline. Pass B quotes both the 1.00 GiB and the
4.00 GiB rows. **Neither figure is wrong. Neither passes the other's.**

Two corrections to the framing:

**a. Pass B's 4 GiB upper bound is very nearly reachable, not comfortably so.**
`SNodeTree::check_tree_validity` (`taichi/struct/snode_tree.cpp:22-32`) errors
if a non-`place`, non-`root` node has no children. A tree of 1024 SNodes
therefore cannot be all-pointer: at minimum the root and one leaf `place` are
not. The structural maximum is a chain of 1022 pointers, giving
1022×4,194,520 + 2×1,048,616 = 4,288,918,472 bytes, 3.99 GiB. Pass B's figure
is high by 0.01 GiB, which does not matter.

**b. Both totals are unreachable for a reason neither report identified.** They
exceed the pool the allocations come from. See section 7.1.

**Ruling: the arithmetic is settled in both reports' favour. The lead's premise
that "both cannot be right" does not survive reading pass A section 2.4.** The
figure the planner should record for 8.1.2 is neither: it is section 7.1's.

---

## 3. Attack item 3 — pass B's conclusion on open question 8.1.2. It half-holds.

Pass B, report line 279-283:

> raising `taichi_max_num_snodes` alone is nearly free in device memory, and it
> does not unlock a raised ceiling on the baseline tier because
> `sizeof(ListManager)` is what consumes the 2 GB.

And escalation E5, report line 672-674:

> On the baseline GTX 750 the card runs out at roughly 512 pointer SNodes
> regardless of the constant.

**First half: SOUND.** The closed form `2488 + 24N + 16T` is correct by
construction and I checked it at five points, including pass B's own table
rows at N=8192/T=1024 (215480), N=65536/T=8192 (1706424) and
N=1048576/T=65536 (26216888). All three reproduce. The page rounding
(`taichi/math/arithmetic.h:13-17`, applied at `runtime.cpp:897`) reproduces at
27, 53, 105, 417 and 6401 pages. Going from N=1024 to N=65536 costs 1.59 MiB
of the separate runtime-objects allocation
(`llvm_runtime_executor.cpp:663-694`). That is genuinely negligible.

Pass A's parallel table at fixed T=512 also reproduces at every row.

**Second half: FAILS AS STATED.** The card is not the binding limit. The
`ListManager`s are not allocated from free device memory; they are bump
allocated out of a fixed preallocated chunk:

- `LLVMRuntime::allocate_aligned` (`runtime.cpp:822-834`) dispatches to
  `allocate_from_reserved_memory` whenever
  `memory_chunk.preallocated_size > 0`.
- That chunk is filled by `runtime_initialize_memory`
  (`runtime.cpp:961-970`), called from
  `LlvmRuntimeExecutor::preallocate_runtime_memory`
  (`llvm_runtime_executor.cpp:607-632`).
- Its size is fixed at `llvm_runtime_executor.cpp:613-619`:
  `config_.device_memory_GB * (1UL << 30)` when
  `device_memory_fraction == 0`, else `device_memory_fraction * total_mem`.
- `CompileConfig::CompileConfig` sets `device_memory_GB = 1` and
  `device_memory_fraction = 0.0` (`taichi/program/compile_config.cpp:63-64`).

So on **any** CUDA card, baseline or Ampere, the default pool is 1 GiB. A 2 GB
GTX 750 and a 24 GB card exhaust at the same SNode count unless the
configuration is changed. Pass B's "the card, not the constant, is the binding
limit" is wrong; the default configuration is. Its own recommendation that
`device_memory_fraction=0.9` be used is printed in the failure message at
`runtime.cpp:866-867`, four lines from code pass B cites, and it did not
follow it.

The failure mode is also not gradual. `allocate_from_reserved_memory`
(`runtime.cpp:837-873`) is a pure bump: on failure it calls `__assertfail` with
"Out of CUDA pre-allocated memory" (`runtime.cpp:860-868`) under `ARCH_cuda`,
then `taichi_assert_runtime` at `runtime.cpp:871`. The grid aborts.

**Ruling: pass B has answered the cheap half of 8.1.2 correctly and mislocated
the expensive half.** The conclusion may stand in the form "the tables are
nearly free"; it may not stand in the form "the card is the limit". Section 7.1
gives the replacement.

---

## 4. Attack item 4 — the claimed leak. Verified. Reachability is narrower than claimed.

`LlvmRuntimeExecutor::destroy_snode_tree` is exactly what pass B quotes,
at `taichi/runtime/llvm/llvm_runtime_executor.cpp:758-761`:

```
void LlvmRuntimeExecutor::destroy_snode_tree(SNodeTree *snode_tree) {
  get_llvm_context()->delete_snode_tree(snode_tree->id());
  snode_tree_buffer_manager_->destroy(snode_tree);
}
```

Every negative claim checks out:

- Nothing clears `element_lists[i]`, `node_allocators[i]` or
  `ambient_elements[i]`. My own exhaustive grep of `runtime.cpp` finds writes
  only at `runtime.cpp:1005`, `1029` and `1038`, all in the initialise path.
- Nothing clears `roots[tree_id]` or `root_mem_sizes[tree_id]`; the only writes
  are `runtime.cpp:996-997`.
- No free path exists. `allocate_from_reserved_memory`
  (`runtime.cpp:837-873`) only ever advances `preallocated_head`
  (`runtime.cpp:852`). There is no counterpart function anywhere in the file.
- `snode_tree_allocs_` is assigned at `llvm_runtime_executor.cpp:440`, read at
  `:387`, and never erased. Confirmed by grep over both the header and the
  source.
- The tree's own root buffer *is* freed:
  `SNodeTreeBufferManager::destroy`
  (`taichi/runtime/llvm/snode_tree_buffer_manager.cpp:20-24`) reaches
  `LlvmRuntimeExecutor::deallocate_memory_on_device`
  (`llvm_runtime_executor.cpp:493-498`). Neither report says otherwise, and
  pass A explicitly says it does. Correct.

**Reachability, which pass B asserted without establishing.** I traced every
caller of `Program::destroy_snode_tree`
(`taichi/program/program.cpp:214-236`). There is exactly one, and it is the
Python binding at `taichi/python/export_lang.cpp:569-570`. No C++ path in the
core reaches it. Section 1.2 of the plan puts the Python front end out of
scope.

So pass B's E4 as written — "repeated recomposition therefore accumulates 1 MiB
per SNode with no reclamation" — is **currently unreachable from the C++ core
alone**. Pass A was right to record it as a fact and decline to call it a
defect (report line 465-466).

It becomes reachable the moment the core grows any destroy path of its own,
which the plan's section 4.6 language about a "materialised working set"
composed for working scope makes likely. Both reports should say that rather
than either asserting the leak bites today or leaving it neutral. Pass B
overstates; pass A understates.

---

## 5. Pass B's self-audit — genuine, and it caught what pass A still has.

Pass B's audit is recorded at `notes-03b-runtime-struct.md:714-757`. I checked
it two ways.

**It was real.** I sampled 31 of pass B's citations across nine files and
printed the source line for each. Twenty-eight land exactly. The five
"concatenation" corrections it claims are all correct against the source:
`taichi/struct/struct.cpp` is 15 lines and `collect_snodes` is at 7-13;
`taichi/struct/snode_tree.cpp` is 41 lines, `check_tree_validity` at 22-32,
`get_snodes_to_root_id` at 34-39. Pass B's report carries the corrected
numbers, not the drafted ones.

**Residual drift remains, and it is small.** Three of the 31 are off by one or
two, all in files the audit did not sweep:

| Pass B cites | Actual | What is there |
|---|---|---|
| `llvm_runtime_executor.cpp:637` | `:639` | `int starting_rand_state = config_.random_seed * 1048391;` |
| `llvm_runtime_executor.cpp:642-653` | `:641-653` | `int num_rand_states = 0;` |
| `llvm_program.h:190` / `:191` | `:188` / `:189` | signature of `get_field_in_tree_offset`, and the `FIXME` |

Nothing in section 2 or section 3 of pass B depends on any of these. The audit
was thorough where it was applied and was not applied everywhere. That is a
fair, not a misleading, self-report.

**Pass A has the identical concatenation fault and did not catch it.** Pass A
also ran an audit (`notes-03-runtime-struct.md:700-760`), and it missed this.
Every `.cpp` line number pass A gives for two files is wrong by exactly the
length of the neighbouring header:

| Pass A cites | Actual | Offset |
|---|---|---|
| `snode_tree.cpp:71-74` (constructor) | `:17-20` | +54 |
| `snode_tree.cpp:76-86` (`check_tree_validity`) | `:22-32` | +54 |
| `snode_tree.cpp:88-93` (`get_snodes_to_root_id`) | `:34-39` | +54 |
| `gfx/snode_tree_manager.cpp:54-59` (`materialize_snode_tree`) | `:11-16` | +43 |
| `gfx/snode_tree_manager.cpp:61-72` (`destroy_snode_tree`) | `:18-29` | +43 |
| `gfx/snode_tree_manager.cpp:74-90` (`get_field_in_tree_offset`) | `:31-47` | +43 |
| `gfx/snode_tree_manager.cpp:92` (`get_snode_tree_device_ptr`) | `:49` | +43 |

`snode_tree.h` is 54 lines. `gfx/snode_tree_manager.h` is 43 lines. Pass A's
own report states the first of those two facts correctly at its line 368
("`taichi/struct/snode_tree.h` is 54 lines") while citing the concatenated
numbering three lines later. `snode_tree.cpp` has 41 lines total; pass A cites
line 93 of it.

The *content* attached to every one of those citations is correct. This is
citation fault, not factual fault. It is still disqualifying for a document
whose stated purpose is to give the planner file and line references, and
standing instruction 5 makes line numbers part of the deliverable.

Everything else of pass A's that I sampled — `runtime.cpp` throughout,
`node_pointer.h:44,45,69,70,86,92,55,76,96`, `node_dynamic.h:5,11,21,22,30,51,
65,67,74,81,99,105,112`, `node_dense.h:10-12,22-23`, `program.cpp:144,214-236,
238-255,559-566`, `llvm_runtime_executor.cpp:391,402-410,417,437-440,442-444,
447,460-468,675-694,758-761`, `llvm_context.cpp:209-211,358-362,968-973,
1004-1011`, `codegen_llvm.cpp:2689-2692,2697`, `ndarray.cpp:35-38,53,75-78,96`,
`arithmetic.h:13-17`, `struct_llvm.cpp:266,269-270` — lands correctly.

---

## 6. Content errors, as distinct from citation errors

### 6.1 Pass A inverts the `all_dense` condition. Pass A section 2.4, last paragraph.

Pass A writes that element lists are skipped when the tree is `all_dense`,
"which the host computes at `llvm_runtime_executor.cpp:402-410` as 'every SNode
in the tree is dense, place or root', **also forced on by**
`config_.demote_dense_struct_fors` (line 401)."

The actual code, `llvm_runtime_executor.cpp:402-410`:

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

The flag is the seed value and the loop can only clear it. So
`all_dense == demote_dense_struct_fors AND (every node is dense/place/root)`.
It is a **necessary precondition for the skip**, not something that forces the
skip on. With the flag false, `all_dense` is false unconditionally and **every
tree allocates a 1 MiB `ListManager` per SNode, dense trees included.** The
line is 402, not 401.

The default is `demote_dense_struct_fors = true`
(`taichi/program/compile_config.cpp:18`), and `CompileConfig::fit` forces it
true again for SPIR-V archs (`compile_config.cpp:72-74`), so pass A's
conclusion — "this is a per-*sparse*-SNode cost, not flat" — is right under the
default and wrong under a configuration that is exposed and settable
(`taichi/python/export_lang.cpp:201-202`).

Pass B's version ("all_dense is false as soon as any SNode is not dense, place
or root", report line 225-226) is true but omits the same half. Neither report
states the conjunction.

This matters for 8.1.2 because it is a third dial: turning
`demote_dense_struct_fors` off multiplies the memory cost of a purely dense
configuration from zero to 1 MiB per SNode.

### 6.2 Both reports quote the wrong `NodeManager` chunk count.

Both describe the `NodeManager` constructor's default of `128 * 1024` elements
per chunk (`runtime.cpp:646-649`). Neither notes that the only call site
overrides it:

```
1029:  runtime->node_allocators[snode_id] =
1030:      runtime->create<NodeManager>(runtime, node_size, 1024 * 16);
```

`chunk_num_elements` is 16384, not 131072. The 128 MB halving loop
(`runtime.cpp:651-655`) therefore only engages when `node_size > 8192`.

This does not change `sizeof(NodeManager)` or `sizeof(ListManager)` — both are
fixed by the `constexpr max_num_chunks`, so the headline 4,194,520 figure both
reports give survives intact. It changes the chunk sizes in section 7.2.

### 6.3 Pass A's escalation 8 and pass B's E7 leave a resolvable question open.

Both flag that the gfx path indexes `compiled_snode_structs_` and
`root_buffers_` by materialisation order while receiving a `Program` tree id
that is recycled, and both decline to determine whether they diverge. It is
determinable from four functions and it does diverge:

1. `SNodeTreeManager::materialize_snode_tree`
   (`taichi/runtime/gfx/snode_tree_manager.cpp:11-16`) only `push_back`s.
2. `SNodeTreeManager::destroy_snode_tree` (`:18-29`) finds `root_id` by
   linear scan on the root pointer, resets `root_buffers_[root_id]`, and leaves
   the `compiled_snode_structs_` entry in place. Neither vector shrinks.
3. `Program::destroy_snode_tree` pushes the id onto `free_snode_tree_ids_`
   (`taichi/program/program.cpp:235`), and
   `Program::allocate_snode_tree_id` (`:559-566`) pops it back.
4. `get_snode_tree_device_ptr(int tree_id)` (`snode_tree_manager.cpp:49-51`)
   returns `runtime_->root_buffers_[tree_id]->get_ptr()`.

Create trees 0 and 1; destroy tree 1; create a third. The third receives
`tree_id == 1`, but its buffer is pushed at index 2, and `root_buffers_[1]` is
the reset `unique_ptr` from step 2. `get_snode_tree_device_ptr(1)` then
dereferences null, and `get_field_in_tree_offset(1, ...)`
(`snode_tree_manager.cpp:31-47`) reads the destroyed tree's descriptors and
fails its own `TI_ASSERT_INFO` at `:34-38`. Divergence after one
destroy-then-add cycle. Both reports could have closed this and did not.

The LLVM path does not have the equivalent problem:
`snode_tree_allocs_` is an `unordered_map` keyed by tree id
(`llvm_runtime_executor.h:152`), so a recycled id overwrites rather than
shifts.

---

## 7. What both reports missed

These are the gaps that make the work incomplete. All four bear directly on
open question 8.1.2.

### 7.1 The pool. `device_memory_GB = 1`, and it is what actually binds.

Established in section 3. Neither `notes-03` nor `notes-03b` contains the
strings `device_memory_GB`, `device_memory_fraction` or
`preallocate_runtime_memory`; I grepped both. Neither report followed
`allocate_aligned` past `runtime.cpp:822` into the chunk it draws from.

The consequence for the sizing rule is not a footnote. Against the default
1 GiB pool (`compile_config.cpp:63`, spent at
`llvm_runtime_executor.cpp:613-615`):

| Configuration | Bytes per SNode | Fit in 1 GiB pool |
|---|---|---|
| Plain sparse SNode, eager only | 1,048,616 | 1023 |
| Plain sparse SNode, after first listgen touch | 5,242,920 | 204 |
| Pointer SNode, eager only | 4,194,520 | 255 |
| Pointer SNode, after first touches, excl. data list | 8,519,896 | 126 |

The first row is the sharpest single fact in this territory. **The current
ceiling of 1024 element lists costs 1,073,782,784 bytes. The default pool is
1,073,741,824 bytes. The existing constant overruns the existing default
by 40,960 bytes** — under one page over. Whether that is coincidence or
upstream sizing intent is not mine to say, but it means the constant and the
pool are already matched to each other, and raising one without the other
buys nothing.

Also unrecorded by either report: `TI_ASSERT(total_prealloc_size <= total_mem)`
at `llvm_runtime_executor.cpp:620` means a configuration cannot ask for more
pool than the card has. On a 2 GB GTX 750, `device_memory_fraction = 0.9`
caps the pool near 1.8 GiB, which is 351 plain SNodes at first-touch cost or
211 pointer SNodes. That is the real baseline-tier number, and it is roughly
half of pass B's 512.

### 7.2 The lazily-touched chunks, which are four times the tables.

Both reports name `ListManager::touch_chunk` and both describe the chunks as
"lazy". Neither says what one costs.

`touch_chunk` (`runtime.cpp:1664-1679`) allocates
`max_num_elements_per_chunk * element_size`:

- **Element lists.** Created with `sizeof(Element)` and `1024 * 64`
  (`runtime.cpp:1005-1006`), and `sizeof(Element)` is 64 (measured). One chunk
  is 65536 × 64 = **4,194,304 bytes, 4 MiB**. Four times the 1 MiB table both
  reports centre on.
- **`NodeManager::free_list` and `recycled_list`.** `sizeof(i32)` elements,
  16384 per chunk (section 6.2). One chunk each is 65,536 bytes.
- **`NodeManager::data_list`.** `node_size` elements, 16384 per chunk. One
  chunk is 16384 × `node_size`, where `node_size` is the SNode's
  `cell_size_bytes` for pointer, or `sizeof(void*) + cell_size * chunk_size`
  for dynamic (`llvm_runtime_executor.cpp:448-456`). For a 1 KiB cell that is a
  single 16 MiB allocation.

The root's element list touches chunk 0 immediately, at
`runtime.cpp:1016`. Every other SNode's touches on its first
`element_listgen_root`/`element_listgen_nonroot` append
(`runtime.cpp:1282-1328`, `1331-1380`, both reaching `ListManager::append` at
`runtime.cpp:1681-1684`). `clear_list` (`runtime.cpp:1270-1273`) only zeroes
`num_elements`; chunks are never returned. So the touched set is monotonic over
the process lifetime, and any SNode a struct-for has ever visited holds its
4 MiB permanently.

**The steady-state cost of a live sparse SNode under struct-for is therefore
about 5 MiB, not 1 MiB.** Pass A alludes to this with "plus the data chunks
that are actually touched" (report line 168) without a number. Pass B does not
mention it.

### 7.3 The failure mode is a grid abort, not degradation.

`runtime.cpp:857-872`. On CUDA the bump allocator calls `__assertfail` with
"Out of CUDA pre-allocated memory", with a comment at `:858-860` explaining
that a `taichi_assert_runtime` would not halt the grid fast enough. Raising the
constant without raising the pool does not produce slow behaviour; it produces
a hard kernel abort at materialisation or at first listgen. Neither report says
what happens at the limit.

### 7.4 Two O(131072) linear scans on the sparse hot path.

`ListManager::ptr2index` (`runtime.cpp:502-512`) walks all `max_num_chunks`
entries and `taichi_assert_runtime`s on the first null, and
`get_num_active_chunks` (`runtime.cpp:467-473`) walks all of them
unconditionally. `ptr2index` is reached from `NodeManager::locate`
(`runtime.cpp:679-681`) from `NodeManager::recycle` (`:683-686`) from
`Pointer_deactivate`
(`taichi/runtime/llvm/runtime_module/node_pointer.h:67-82`) and
`Dynamic_deactivate` (`node_dynamic.h:44-58`).

Both reports list both functions in their width tables. Neither observes that
the loop bound is the same 131072 constant that produces the 1 MiB table, and
that it sits on the deactivate path of exactly the sparse structures the plan's
section 4.1 says are required. Against section 6.4 — "the sole issue is how
many calculations can be packed in" — a per-deactivate scan bounded by a
constant unrelated to the live data is a throughput fact worth recording.

I am recording it, not proposing anything about it.

---

## 8. Every other place the two reports disagree, resolved

| # | Disagreement | Resolution against source |
|---|---|---|
| 1 | Use-site count. A says "exactly eight occurrences" of both constants; B says "exactly five" of `taichi_max_num_snodes`. | **Both right, counting different things.** My unrestricted grep of the whole tree finds `taichi_max_num_snodes` at `constants.h:12`, `struct_llvm.cpp:266`, `runtime.cpp:567,568,569` — five. Plus `kMaxNumSnodeTreesLlvm` at `constants.h:13`, `runtime.cpp:562,563` — three. Eight together. No occurrence anywhere else, including tests, `c_api/` and CMake. |
| 2 | Territory ownership of `struct_llvm.cpp:266`. A treats it as in scope; B assigns it to agent 02. | Cosmetic. B is right on the letter of the assignment table in plan 9.3; A is right that the consequence lands on its arrays. No factual conflict. |
| 3 | `check_tree_validity` scope. A: "only enforces that non-place, non-root nodes have at least one child". B: same. | Agree, both correct (`snode_tree.cpp:22-32`). Only A's line number is wrong. |
| 4 | What destroy leaves behind. A lists `snode_trees_` not reset (`program.cpp:249,252`). B lists `snode_tree_allocs_` never erased (`llvm_runtime_executor.cpp:440`). | **Both correct, and complementary rather than conflicting.** Each found something the other did not. Combined, the list in section 4 above is the complete one. |
| 5 | Whether `1024` is a live or cumulative ceiling. A says the ids are "program-lifetime global"; B states outright it is "a ceiling on the cumulative number of SNodes created over a `Program`'s lifetime". | **B's phrasing is the accurate one.** `SNode::id = counter++` at `taichi/ir/snode.cpp:220` from the `std::atomic<int>` at `:12`; reset only at `taichi/program/program.cpp:144`; `SNode::reset_counter` (`taichi/ir/snode.h:348-350`) has no caller — the only `reset_counter` call in the tree is `Stmt::reset_counter()` at `program.cpp:347`, a different class. Copying is forbidden (`snode.cpp:230-233`), so ids are consumed one per constructed node and never returned. |
| 6 | The `2^33` vs `2^31` capacity mismatch. A section 5.3 and B section 4.3. | Identical claim, identical numbers, both correct. `max_num_chunks` 131072 × `1024*64` per-chunk elements from `runtime.cpp:1006` is 2^33 addressable slots against an `i32 num_elements` at `runtime.cpp:433`. |
| 7 | Where the build parameter goes. A escalates the two-artifact consistency risk; B escalates it as E9/E10 and adds that the `.bc` output lands in the source tree. | Agree. Both correct on `runtime_module/CMakeLists.txt:8-13` and `llvm_context.cpp:209-211,358-362`. B's addition that `struct_llvm.cpp:266` cannot detect a mismatch (because per section 3.1 it is not checking that bound) is the sharper form. |
| 8 | gfx numbering. Both flag, neither resolves. | Resolved in section 6.3 above. It diverges. |

I found no disagreement between the two reports where one is right and the
other wrong on a matter of fact. The divergence is entirely in emphasis,
scenario choice, and citation accuracy.

---

## 9. The two verdicts, separately

**CORRECT.** Pass B: yes, with the one withdrawal in section 3 and the
`all_dense` conjunction in 6.1. Pass A: yes on content, no on citation. Pass
A's factual error rate is low; its citation error rate in two files is 100%.
Under plan standing instruction 5, that is a correctness failure of the
deliverable even though the underlying reading was right.

**COMPLETE.** Neither. The territory's stated ownership is "the ceiling the
whole project is trying to raise" and both passes stopped one call frame short
of the thing that enforces it. Section 7.1 is not a refinement of their answer
to 8.1.2; it replaces the denominator. Section 7.2 multiplies the numerator by
five. A sizing rule built on either report as it stands would be wrong by
roughly an order of magnitude in the direction that matters.

**MAJOR REVISION REQUIRED: yes.** Specifically:

1. Pass A regenerates every `snode_tree.cpp` and `gfx/snode_tree_manager.cpp`
   citation.
2. Pass A corrects the `all_dense` conjunction and the line 401/402 slip.
3. Pass B withdraws "the card, not the constant, is the binding limit" and the
   512-pointer-SNode figure in E5.
4. Both incorporate section 7.1 (the pool), 7.2 (chunk sizes) and 7.3 (the
   abort). Both correct the `1024 * 16` at `runtime.cpp:1030`.
5. Both close the gfx numbering question per section 6.3 rather than
   escalating it.
6. Pass B qualifies E4 with the reachability finding in section 4.

The convergent claim in attack item 1 survives all of this and should be
recorded in the plan. Section 6.1's "It is a footprint, not only a bounds
check" is right in kind and wrong in scale by roughly 43,000 to one.

---

## 10. Escalations

Decisions the project owner has not made. I am not resolving any of these.

**A1 — The sizing rule for 8.1.2 has three dials, not one.**
`taichi_max_num_snodes` (`taichi/inc/constants.h:12`),
`ListManager::max_num_chunks` (`runtime.cpp:427`) and
`CompileConfig::device_memory_GB` (`taichi/program/compile_config.cpp:63`).
Section 7.1 shows the first is already matched to the third to within one page.
Any answer to 8.1.2 that moves one and not the others is arithmetic that does
not close. Which of the three is an install-time factor is the planner's call.

**A2 — `demote_dense_struct_fors` is a fourth input and is settable.**
Section 6.1. Default true (`compile_config.cpp:18`), forced true for SPIR-V
(`compile_config.cpp:72-74`), exposed at `export_lang.cpp:201-202`. With it
false, a purely dense configuration pays the full per-SNode `ListManager` cost.
Whether the install-time loader is permitted to touch it is undecided.

**A3 — The out-of-bounds write in section 3.1 of both reports is reachable and
neither report says so.** Two sparse trees whose combined SNode count exceeds
1024 write `element_lists[i]` past the array (`runtime.cpp:1005`) while each
passes `struct_llvm.cpp:266` independently. I did not construct a failing case
and am not proposing a fix. What the correct invariant is remains a design
decision, as both reports say.

**A4 — `runtime_initialize_snodes` fills element lists over the arithmetic
range `[root_id, root_id + num_snodes)` (`runtime.cpp:1003`) while the node
allocator and ambient loops use actual ids `snode_metas[i].id`
(`llvm_runtime_executor.cpp:449`).** Pass A raises this (its escalation 6);
pass B does not. Ids are issued at construction (`snode.cpp:220`) and collected
pre-order (`struct.cpp:7-13`), which need not be the same order. Whether the
two can diverge is an IR-layer question for territory 01. I did not chase it.

**A5 — The per-deactivate scans in section 7.4.** Recorded, not judged.

**A6 — The leak's reachability changes with the plan's own section 4.6.** A
core-owned recompose path would make section 4's findings live. No such path
exists today.

---

## 11. Cross-check against adversary 03-1

`/opt/project/taichi/modernization/investigation/adversary-03-1.md` does not
exist at the time of writing. No divergence section can be recorded.
