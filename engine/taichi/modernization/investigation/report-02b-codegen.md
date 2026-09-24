# Report 02B — Code generation, both paths

Agent 02B. Territory: `taichi/codegen/` in full — **51 files, of which 45 are
`.h`/`.cpp` and 6 are `CMakeLists.txt`**, both figures derived from
`find taichi/codegen -type f`. **Section 9 is the sweep list** and accounts for
all 45 by name: 21 cited as findings, 24 opened with what was in them.
Contemporaneous notes: `modernization/investigation/notes-02b-codegen.md`.

**Revision 4, 2026-09-09.** Worked against `PROJECT-PLAN.md` **dated
2026-09-09**, re-read in full at that stamp, including the priority statement at
the head of section 10 and item 6 of the standing instructions. Round three
adversary analyses (`adversary3-02-1.md`, `adversary3-02-2.md`) split. Both
uphold everything they tested; 3-2 additionally finds one substantive fault in
this report and one omission in the source that neither report and no prior
adversary had reached. Both are verified here at source and corrected. Section 6
and section 3.2's widening enumeration are rewritten: **of the four `u32_type()`
hardcodes in `bitmasked_activation`, only `:411` breaks under a widened pointer**
— `:412` is a Shift operand and `:414` is a buffer element type that must NOT be
widened. Section 6 gains two further widening-invalidity sites at `:392-394` and
`:395-397` from a second mechanism, an uncast index passed by the
`SNodeLookupStmt` caller at `:488-489`. Sections 3.2, 4.6's neighbours and
section 2 carry two spans this report's own ledger had already withdrawn.
Section 9.2 corrects one quantifier. The citation figure is corrected and given a
counting rule. Sections 7.13-7.14, escalation 21 and section 11 are new.

**Revision 3, 2026-09-09.** Round two adversary analyses
(`adversary2-02-1.md`, `adversary2-02-2.md`) judged revision 2 correct on
substance, not fully correct on method, and not complete. This revision is the
amendment pass. Section 4.4's attribution of two of the three live translator
call sites was **swapped** and is corrected. Section 4.5 is extended from one
capability to all five. Section 4.4 gains the chain that determines what
actually happens to an invalid module. Section 5.3's conclusion is withdrawn.
Section 6 gains a second widening-invalidity site. Section 9 is new and is the
territory sweep list. Sections 7 and 8 are extended.

**Revision 2, 2026-09-09.** Round one produced this report; two adversary
analyses (`adversary-02-1.md`, `adversary-02-2.md`) then judged it against
`report-02-codegen.md`. That revision was made under the planner's direction
after arbitration. Section 4 was rewritten because its central claim was false.
Section 5 was extended because it stopped one call site short. Sections 3 and 6
carried citation corrections. Section 7 was new and recorded the adjudications
I made where the two adversaries contradict each other. Section 8 was the
escalation list, extended.

Everything below is marked **[V]** verified by reading the named code, or
**[I]** inferred. Nothing is unmarked.

**On method, and this replaces a false claim.** Revision 2 said at this point
that "every **[V]** in this revision was re-opened at the named line during
this pass, not carried forward on trust." **That claim was false and is
withdrawn.** Both round-two adversaries demonstrated it, and I confirm it: rows
of the section 3 tables appear in neither list of my notes §28 and several of
them are wrong. Notes §32 records how the claim came to be made.

What revision 3 actually did, stated so it can be checked: every `file:line`
citation in this document was extracted mechanically and printed from the named
source file, and each printed extract was compared against the sentence it
supports. The bare `:NNNN` references, which name a line without repeating the
filename, were resolved to their governing file and opened by hand.

**The figure attached to that sweep is corrected in revision 4, and the counting
rule is now stated because its absence is what made the old figure untestable.**
Revision 3 wrote "**278** distinct file-qualified citations". That number is not
reproducible from this document under any rule I can state, and revision 4
withdraws it. **Rule:** a citation is a token matching
`<path>.<ext>:<line>[-<line>]` for `ext` in `cpp h mm cc hpp py md txt cmake inc`,
counted once per distinct `(path-as-written, start, end)` triple, over the whole
document including the ledgers' superseded "Was" columns. Under that rule this
document holds **322** distinct citations across **91** distinct
file paths, from **404** raw occurrences, and **309** distinct
`(path-as-written, start)` pairs. Both round-three adversaries ran the same
extraction independently against revision 3 and got 311 triples, 298 pairs and 87
files, and both report **zero** citations that fail to resolve to a real file and
**zero** whose line is out of range (`adversary3-02-1.md` §4.2,
`adversary3-02-2.md` §4.2). I re-ran that resolution test against this
revision, resolving each cited basename to a real path in the tree and checking
the named line against that file's length: **322 of 322 resolve, zero
failures.** What the sweep certifies is unchanged and is stated in the next
paragraph: resolvability, not attribution. The faults that pass found
are listed in section 10 and each is corrected in place. This is a claim about
citations, not about the sentences attached to them: per the standing
instructions §10.6, a verified citation certifies only the citation, and every
quantifier over the tree in this report is still marked for what it is.

---

## 0. What changed in this revision, and why

Stated first so the planner does not have to diff.

1. **Section 4's central claim was false and is withdrawn.** Revision 1 said
   SPIR-V Int64 is "checked at every use". It is checked on one type-lowering
   path and unchecked on the other, and the unchecked one is the live path for
   kernel argument, return and argpack structs. Both adversaries resolved this
   against me independently. I have now opened the code and they are right.
   Section 4 is rewritten around what the source actually says. **[V]**
2. **The defect is wider than Int64.** The same function bypasses the same
   guards for eight optional types under five capability flags, including
   `f16` and `f64`. **[V]**
3. **Section 5 understated the SNode-ceiling defect.** Exceeding the bound is
   an out-of-bounds *store* over a contiguous range keyed on the tree root's
   global id (`runtime.cpp:1003-1006`), not a bad lookup. **[V]**
4. **OpenGL ES is not the only in-tree SPIR-V target without Int64.** Metal
   gates it on `family_apple3`; Direct3D 11 never sets it at all; and the C API
   OpenGL runtime sets it with no guard, overwriting the GLES guard. **[V]**
5. **Revision 1's claim that `max_num_elements` is "the only piece of SNode
   geometry already 64-bit on both sides" is withdrawn.** The accessor
   truncates it back to i32. **[V]**
6. **Revision 1's claim that every 64-bit-pointer branch in the SPIR-V path is
   unreachable is corrected.** `at_buffer`'s 64-bit branch keys on the SType's
   data type, not on the physical-storage-buffer capability, so it is reachable
   the moment a u64-typed pointer value exists. **[V]**

Added in revision 3:

7. **Section 4.4's attribution of two of the three translator call sites was
   swapped, and this report supplied the spans that disproved it.**
   `spirv_codegen.cpp:2464` is inside `compile_argpack_struct` (`:2402-2481`)
   and `:2511` is inside `compile_ret_struct` (`:2483-2527`). Revision 2 had
   them the other way round while printing those very spans. **[V]** Section 4.4.
8. **The "silently" in revision 2's failure mode is withdrawn.** An id-0 module
   is caught inside Taichi's own process by the SPIRV-Tools binary parser,
   emitted as two `TI_WARN`s, and shipped anyway because the flag that records
   the failure is read only inside a compiled-out block. **[V]** Section 4.4.
9. **Section 4.5 covered one of the five bypassed capabilities. It now covers
   all five.** A grep of `taichi/` and `c_api/` finds no setter of
   `spirv_has_float64` on the Metal path at all, so an `f64` kernel argument
   produces type id 0 on every Metal device, current hardware included.
   **[V]** Section 4.5.1.
10. **Section 5.3's conclusion that both routes require sparse SNodes is
    withdrawn.** `all_dense` is seeded from `config_.demote_dense_struct_fors`;
    with that field false the range store fires for every tree, dense included.
    The report recorded the seed in revision 2 and failed to propagate it into
    the conclusion. **[V]** Section 5.3.
11. **A second widening-invalidity site inside `bitmasked_activation`**, at
    `spirv_codegen.cpp:398-399`, in the opposite direction from `:410-412`.
    **[V]** Section 6. **Revision 4 adds two more and narrows the first; see
    items 13 and 14 below.**
12. **The territory sweep list is now in the report** rather than only in the
    notes. Section 9. Revision 2's header claimed 51 files and named 21 of the
    45 `.h`/`.cpp` files.

Added in revision 4:

13. **Revision 3's widening enumeration was too wide by two, and one of the two
    would have been a regression.** Of the four `u32_type()` hardcodes in
    `bitmasked_activation`, **only `:411` breaks** under a widened pointer.
    `:412` is the Shift operand of the same instruction and is exempt by exactly
    the rule this report already applied to `:405`; `:414` is
    `struct_array_access`'s pointee type, the element type of a buffer that
    genuinely holds 32-bit words, and widening it would be a regression rather
    than a fix. Verified against the SPIRV-Tools validator checked out in this
    tree, not against the specification. **[V]** Sections 3.2, 6, 7.12, 7.13.
    `adversary3-02-2.md` §7 found it.
14. **`bitmasked_activation` holds four widening-invalidity sites on its
    `SNodeLookupStmt` path, not two**, from a second mechanism no report and no
    prior adversary had named: the caller at `spirv_codegen.cpp:488-489` passes
    an **uncast i32** index, while the same visitor casts the same value at
    `:504-505`. The two further sites are `:392-394` and `:395-397`. **[V]**
    Section 6, escalation 21. `adversary3-02-2.md` §8 found it.
15. **Two spans this report's own ledger withdrew were still standing
    elsewhere**, at `:121` and in section 2's list, the second inside a bullet
    list closed with a blanket "All **[V]**". Corrected in place. **[V]**
    `adversary3-02-1.md` §8.3 found it. Section 3.2's "not 32-bit-limited"
    bullet on `SNodeLookupStmt` is qualified in the same pass, because the
    finding at item 14 falsifies its "with no further change" as a statement
    about that visitor.
16. **The citation figure of 278 is withdrawn and replaced by a figure with a
    stated counting rule** (header, section 10). It was not reproducible. What
    the sweep certifies is unchanged, and both round-three adversaries confirm
    independently that every citation in this document resolves to a real file
    at a real line.
17. **One quantifier in section 9.2 is corrected**: `construct` in
    `snode_struct_compiler.cpp` has a second, recursive call at `:44`, so "its
    only call is at `:18`" was wrong as written. The conclusion — unreachable in
    the current tree — is unaffected, because `:44` is reachable only from
    `:18`. **[V]** `adversary3-02-2.md` §10 found it.

---

## 1. Executive summary

Five things dominate.

1. **Both paths linearise indices at 32 bits, in the same shape.**
   `codegen_llvm.cpp:1736-1744` and `spirv_codegen.cpp:532-541` are the same
   Horner loop over i32. This is the single site the two paths agree on, and
   it is the one that must change first. **[V]**

2. **The two paths diverge completely on address width.** LLVM computes
   addresses through LLVM pointers and GEPs, which are 64-bit on every target
   Taichi supports, and only the *index* is 32-bit. SPIR-V represents a
   pointer as a plain **u32 integer** and does its own byte arithmetic on it
   (`spirv_ir_builder.cpp:334-353`, `spirv_codegen.cpp:2194-2220`). The SPIR-V
   path therefore has a hard 4 GiB cap on the root buffer that the LLVM path
   does not have. **[V]**

3. **The SPIR-V 64-bit machinery already exists and is switched off in two
   independent places.** `use_64bit_pointers` is a hardcoded
   `const bool ... = false` (`spirv_codegen.cpp:82`), and
   `spirv_has_physical_storage_buffer` is set only inside an
   `#if !defined(__APPLE__) && false` block
   (`vulkan_device_creator.cpp:825-827`). Both switches must be turned on for
   64-bit SPIR-V addressing, the code behind them is incomplete by its own
   comment (`spirv_codegen.cpp:2319`), and flipping either one alone produces
   an invalid module rather than a wider one. Section 6 gives the mechanism.
   **[V]**

4. **The SPIR-V type system does not check Int64 on the path that compiles
   kernel signatures.** `Translate2Spirv::visit_int_type`
   (`spirv_types.cpp:393-419`) calls the bare accessors at
   `spirv_ir_builder.h:529-534`, which return a default-constructed `SType`
   whose `id` is 0 (`spirv_ir_builder.h:51`) when the capability is absent.
   Zero is not a valid SPIR-V id. That is the live path for the args, rets and
   argpack structs. **[V]** Section 4.

5. **The assertion at `struct_llvm.cpp:266` bounds a different quantity from
   the one the runtime arrays are indexed by, and exceeding the bound is a
   memory-corrupting store.** The assertion counts SNodes in **one tree**; the
   arrays are indexed by a **process-global monotonic** SNode id, and
   `runtime_initialize_snodes` writes a contiguous range starting at the tree
   root's global id. **[V]** Section 5.

On section 6.2's specific question — is SPIR-V Int64 declared, checked,
assumed or absent — the answer is **declared conditionally, checked on one
type-lowering path, unchecked on the other, and assumed outright by two device
backends**. Details in section 4.

**Three things revision 3 adds that do not displace the five above, and one it
withdraws.** The defect at point 4 is not confined to Int64 and not confined to
legacy hardware: no Metal device sets `spirv_has_float64`, so an `f64` kernel
argument emits type id 0 on all Metal hardware (section 4.5.1). The invalid
module is not silent — it is caught in-process by the SPIRV-Tools parser,
downgraded to a warning by `spriv_message_consumer`'s branch order, and shipped
anyway because the flag recording the failure is dead code (section 4.4). And
`bitmasked_activation` breaks under widening in two directions, not one
(section 6) — **revision 4 counts the sites in each direction and finds four
broken instructions on the live `SNodeLookupStmt` path, one in the first
direction and three in the second, against the two revision 3 named**.
Withdrawn: point 5's reachability is wider than revision 2 said —
the `element_lists` route needs no sparsity at all when
`demote_dense_struct_fors` is false (section 5.3).

---

## 2. Where section 6's proposed changes touch my territory

### 6.1 Parameterise the SNode ceiling

| Site | What | V/I |
|---|---|---|
| `taichi/codegen/llvm/struct_llvm.cpp:266` | `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);` The only use of the constant in all of codegen. Post-hoc check, runs after all types and accessors are generated. | **[V]** |
| `taichi/codegen/llvm/struct_llvm.cpp:142-143` | One dummy LLVM function `<name>_type_stubs_func` per non-bit-level SNode. | **[V]** |
| `taichi/codegen/llvm/struct_llvm.cpp:146-190, 199-201` | One `refine_coordinates` function per non-leaf SNode, body unrolled `taichi_max_num_indices` = 12 times. | **[V]** |
| `taichi/codegen/llvm/struct_llvm.cpp:203-233` | One `get_ch_from_parent` function per non-root SNode. | **[V]** |
| `taichi/codegen/llvm/codegen_llvm.cpp:278` | `snode_id` emitted into `StructMeta` as i32 via `get_constant(snode->id)`. `SNode::id` is `int`. | **[V]** |
| `taichi/codegen/llvm/codegen_llvm.cpp:2308-2311` | `leaf_block->id` passed to `parallel_struct_for` as i32 (the id at `:2308`). | **[V]** |
| `taichi/codegen/llvm/codegen_llvm.cpp:2689-2692` | `get_root(int snode_tree_id)` — i32 tree id into `roots[kMaxNumSnodeTreesLlvm]`. | **[V]** |
| SPIR-V path, all files | **No SNode-count bound of any kind.** `snode_descriptors` is an unbounded `unordered_map<int, SNodeDescriptor>` (`snode_struct_compiler.h:38`), and neither constant appears anywhere in `taichi/codegen/spirv/`. | **[V]** |

**Cost of raising the ceiling, in codegen:** struct-module size and LLVM
compile time are linear in SNode count, with the per-SNode constant set by
`taichi_max_num_indices` through the unrolled refine body. **[V]** for the
code shape; the linear-cost conclusion is **[I]**, though it follows directly.

An i32 SNode id is not itself a constraint at any plausible ceiling — 2^31 ids
is not the bottleneck. **[I]**

### 6.2 64-bit addressing

Full inventory in section 3. The sites that must change together:

**LLVM path**
- `codegen_llvm.cpp:1736-1744` `LinearizeStmt` — the accumulator and strides.
- `codegen_llvm.cpp:1169-1171, 1188, 1192` range-for loop variable and its
  **signed** comparisons.
- `codegen_llvm.cpp:2065-2087` `get_range_for_bounds`.
- `codegen_llvm.cpp:2157` struct-for `loop_index_ty`.
- `codegen_llvm.cpp:2336, 2339-2340` `LoopIndexStmt` — both loads hardcode i32.
- `codegen_llvm.cpp:1755` `TI_ASSERT(bit_offset->getType()->isIntegerTy(32))`
  in `create_bit_ptr` — a hard assertion that will fire on any widening.
- `codegen_llvm.cpp:1920-1945, 1988` `ExternalPtrStmt` i32 shape loads and
  linearisation.
- `struct_llvm.cpp:153, 171-185` `generate_refine_coordinates`.
- The `PhysicalCoordinates` struct itself, `runtime.cpp:288-290` — outside my
  territory but codegen is written against its exact shape
  (`codegen_llvm.cpp:2356-2381`).
- Every runtime signature in note 4 of my notes: `lookup_element`, `is_active`,
  `activate`, `refine_coordinates`, `RangeForTaskFunc`, `BlockTask`,
  `parallel_struct_for`, `cpu_parallel_range_for`, `gpu_parallel_range_for`.
- `cpu/codegen_cpu.cpp:57,59,103,110,126`; `cuda/codegen_cuda.cpp:480,482,522-523,549`;
  `amdgpu/codegen_amdgpu.cpp:258`; `dx12/codegen_dx12.cpp:42,82-83,112`.
All **[V]**.

**Upstream of my territory, and load-bearing.** `LinearizeStmt::strides` is
`std::vector<int>` (`taichi/ir/statements.h:1280`), and the values are produced
outside codegen at `int`: `std::array<int, taichi_max_num_indices> total_shape`
at `taichi/transforms/scalar_pointer_lowerer.cpp:33` and the identical
declaration at `taichi/transforms/demote_dense_struct_fors.cpp:19`. **[V]**
Widening the codegen consumer without those two producers is a half-change.
Those files are territory 01's; recording the seam because the consumer is the
site both passes named as the one that must change first.

**SPIR-V path**
- `spirv_codegen.cpp:82` `use_64bit_pointers`.
- `spirv_codegen.cpp:2317-2324` `make_pointer`, including the
  `uint32_t(offset)` narrowing cast at `:2322` and the bare `u64_type()` call
  at `:2320`.
- `spirv_codegen.cpp:532-541` `LinearizeStmt`.
- `spirv_codegen.cpp:2194-2220` `at_buffer`, the byte-pointer to SSBO-index
  shift.
- `spirv_codegen.cpp:383-435` `bitmasked_activation` — hardcodes `u32_type()`
  at lines 405, 411, 412, 414 regardless of `make_pointer`. **Of those four only
  `:411` must change under a widened pointer, and `:414` must not**; the sites
  that must change with it are `:392-394`, `:395-397` and `:398-399`. Section 6.
- `spirv_codegen.cpp:488-489` — the `SNodeLookupStmt` caller of
  `bitmasked_activation`, which passes an **uncast** i32 index where its own
  neighbour at `:504-505` casts the same value to the pointer type. Section 6.
- `spirv_codegen.cpp:707-712` `GlobalTemporaryStmt` narrows a `size_t` to i32.
- `spirv_codegen.cpp:737-781` `ExternalPtrStmt`, including the i32
  `OpShiftLeftLogical` byte-offset computation at 773-777.
- `spirv_codegen.cpp:2000-2004, 2017-2044, 2054, 2095-2097`
  `generate_range_for_kernel`.
- `spirv_codegen.cpp:2138-2185` `generate_struct_for_kernel`, u32 listgen.
- `spirv_ir_builder.cpp:334-353` `from_taichi_type` and
  `spirv_types.cpp:484-514` `translate_ti_type` — the pointer representation.
  **Both spans corrected in revision 4**: they are the forms this report's own
  section 10 ledger, rows 9 and 10, already established in revision 3, and the
  superseded `:334-341` and `:484-497` had survived here under the blanket
  "All **[V]**" that closes this list. `adversary3-02-1.md` §8.3 found it.
- `spirv_types.cpp:393-419` and `:421-431` — the unguarded type translation.
  Section 4.
All **[V]**.

### 6.3 Adaptive module loading

Only two things in my territory bear on this, both **[V]**:
- `codegen.cpp:37-74` `KernelCodeGen::create` — the LLVM-path dispatch. It
  keys on `compile_config.arch` alone, inside `#if defined(TI_WITH_CUDA)` /
  `TI_WITH_DX12` / `TI_WITH_AMDGPU` blocks. No capability or hardware-generation
  parameter reaches it.
- `llvm/kernel_compiler.cpp:30-48` — takes `const DeviceCapabilityConfig
  &device_caps` at line 32 and **never uses it**. It is not forwarded to
  `KernelCodeGen::create` at lines 36-37. The SPIR-V equivalent
  (`spirv/kernel_compiler.cpp:37`, `params.caps = device_caps;`) does forward
  it, and from there it reaches every `caps_->get(...)` site in the SPIR-V
  codegen.

So the capability-plumbing point that section 6.3 would extend exists on the
SPIR-V side and is a dead parameter on the LLVM side.

---

## 3. Complete inventory of 32-bit width assumptions in emitted code

Separated by path, as required. All **[V]** unless marked. Every line in these
tables was re-opened mechanically in revision 3, by extracting the citation and
printing the named line from source. Revision 2 claimed the same and had not
done it; four rows of this table and five of the next were wrong. Section 10
lists them.

### 3.1 LLVM path

**Index arithmetic**

| File:line | Assumption |
|---|---|
| `llvm/codegen_llvm.cpp:1737` | `LinearizeStmt` accumulator `get_constant(0)` is `APInt(32,...)` via `llvm_context.cpp:738-740`. |
| `llvm/codegen_llvm.cpp:1740` | Strides `get_constant(stmt->strides[i])`; `LinearizeStmt::strides` is `std::vector<int>` (`taichi/ir/statements.h:1280`). Producers at `transforms/scalar_pointer_lowerer.cpp:33` and `transforms/demote_dense_struct_fors.cpp:19`. |
| `llvm/codegen_llvm.cpp:1169` | `loop_var_ty = get_data_type(PrimitiveType::i32)`. |
| `llvm/codegen_llvm.cpp:1171` | `create_entry_block_alloca(PrimitiveType::i32)` for the range-for counter. |
| `llvm/codegen_llvm.cpp:1188, 1192` | `ICMP_SLT` / `ICMP_SGE` — **signed**, so the cap is 2^31-1, not 2^32-1. |
| `llvm/codegen_llvm.cpp:2072, 2074-2075, 2081, 2083-2084` | Range-for bounds forced through `PrimitiveType::i32` in `get_range_for_bounds` (`:2065-2087`). |
| `llvm/codegen_llvm.cpp:2069, 2078` | Constant bounds from `stmt->begin_value`/`end_value`, declared `int32` at `taichi/ir/statements.h:1415-1416`. |
| `llvm/codegen_llvm.cpp:2157` | `loop_index_ty = getInt32Ty` for the struct-for element index. |
| `llvm/codegen_llvm.cpp:2214-2216` | Struct-for loop test `ICMP_SLT` on i32. **Corrected from `:2213-2216` in revision 3**; `:2213` is `builder->SetInsertPoint(loop_test_bb);`. |
| `llvm/codegen_llvm.cpp:2273` | `create_increment(loop_index, block_dim)` on i32. **Corrected from `:2272` in revision 3**; `:2272` is blank. |
| `llvm/codegen_llvm.cpp:2336` | `LoopIndexStmt` struct-for branch loads `getInt32Ty`. |
| `llvm/codegen_llvm.cpp:2339-2340` | `LoopIndexStmt` range-for branch loads `getInt32Ty`. |
| `llvm/struct_llvm.cpp:153` | `generate_refine_coordinates` third parameter `l` is `getInt32Ty`. **Corrected from `:152` in revision 1.** |
| `llvm/struct_llvm.cpp:171` | Loop bound `taichi_max_num_indices`; up to 12 unrolled div/rem/mul/add chains per SNode. |
| `llvm/struct_llvm.cpp:172-176` | Extractor constants are i32; `AxisExtractor::num_elements_from_root`, `shape`, `acc_shape` are `int` (`taichi/ir/snode.h:41,45,49`). |
| `llvm/struct_llvm.cpp:174-175` | `acc_shape * shape` is computed in **host C++ `int`** before reaching LLVM. Overflows in the compiler, not just in emitted code. |
| `llvm/struct_llvm.cpp:179` | `CreateUDiv(CreateURem(l, prev), next)` at i32. **Corrected from `:177`.** The `prev` operand is the host `int` product from `:174-175`, so a product that wraps to exactly zero makes this a `urem` by zero. |
| `llvm/struct_llvm.cpp:181-185` | `CreateMul(in, get_constant(shape))` at `:184` then `CreateAdd` at `:185`, i32, no overflow guard. |
| `cpu/codegen_cpu.cpp:57` | Offloaded body signature third arg `get_data_type<int>()`. |
| `cpu/codegen_cpu.cpp:59` | Loop var `PrimitiveType::i32`. |
| `cpu/codegen_cpu.cpp:103, 110, 126` | Mesh-for loop index alloca and loads at `getInt32Ty`. |
| `cpu/codegen_cpu.cpp:111-112` | `ICMP_SLT` in the mesh-for loop test. |
| `cuda/codegen_cuda.cpp:480` | Body signature third arg `get_data_type<int>()`. |
| `cuda/codegen_cuda.cpp:482` | Loop var `PrimitiveType::i32`. |
| `cuda/codegen_cuda.cpp:522-523, 535, 549` | Mesh-for loop index i32, `ICMP_SLT`, i32 add. |
| `amdgpu/codegen_amdgpu.cpp:258` | Loop var `PrimitiveType::i32`. |
| `amdgpu/codegen_amdgpu.cpp:464, 466` | i32 workgroup dimension constants. |
| `dx12/codegen_dx12.cpp:42` | Loop var `PrimitiveType::i32`. |
| `dx12/codegen_dx12.cpp:82-83, 86, 89, 98, 112` | Mesh-for loop index, thread idx, block dim 64, `ICMP_SLT`, i32 add. |
| `dx12/dx12_lower_intrinsic.cpp:54, 83-84` | i32 constants for DXIL thread/group id intrinsics. **Externally fixed by the DirectX intrinsic definitions, not a Taichi choice.** **[I]** for the "externally fixed" characterisation. |

**Address / offset arithmetic**

| File:line | Assumption |
|---|---|
| `llvm/codegen_llvm.cpp:1755` | `TI_ASSERT(bit_offset->getType()->isIntegerTy(32))` — hard assertion. `TI_ASSERT` expands to `TI_ERROR` unconditionally (`taichi/common/logging.h:100-107`), with no `NDEBUG` guard, so it is present in release builds. |
| `llvm/codegen_llvm.cpp:1750-1753` | The bit-pointer struct is documented and built as `{ iX* byte_ptr; i32 bit_offset; }`. |
| `llvm/codegen_llvm.cpp:1820-1824` | quant_array: `get_constant(element_num_bits)` at `:1822`, i32, multiplied by the input index at `:1823`, into `create_bit_ptr` at `:1824`. **Span corrected from `:1823-1825` in revision 3**; `:1825` is `} else {`. |
| `llvm/codegen_llvm.cpp:1834-1839` | `GetChStmt` bit-struct branch: `get_constant(bit_offset)` i32 at `:1838`, into `create_bit_ptr` at `:1839`. **Misattribution corrected from `:1841-1843` in revision 3**: that span is the `else` branch, `call_struct_func`, and contains no `get_constant` at all. Found by adversary2-02-2 §4.2. |
| `llvm/codegen_llvm.cpp:1926-1927` | `ExternalPtrStmt` loads ndarray shapes as `PrimitiveType::i32`. |
| `llvm/codegen_llvm.cpp:1931-1945` | `ExternalPtrStmt` linear index accumulator and Mul/Add chain at i32. |
| `llvm/codegen_llvm.cpp:1988` | Non-TensorType branch does `CreateGEP(base_ty, base, linear_index)` with the raw i32 index. LLVM sign-extends to pointer width, so the address is correct, but the index arithmetic already wrapped. |
| `llvm/codegen_llvm.cpp:2405-2407` | `BlockLocalPtrStmt` GEP mixes an i32 zero with the IR-supplied offset. |
| `llvm/codegen_llvm.cpp:2290-2291` | `int list_element_size = std::min(leaf_block->max_num_elements(), (int64)taichi_listgen_max_element_size);` — an `int64` narrowed into an `int`. Safe today only because the constant is 1024 (`taichi/inc/constants.h:28`). **Constant line corrected from `:27`.** |

**Already 64-bit in the LLVM path** — these do not need changing:
- `codegen_llvm.cpp:1864-1871` `MatrixPtrStmt` byte-offset branch:
  `PtrToInt`→i64, `SExt` offset→i64, add, `IntToPtr`.
- `codegen_llvm.cpp:1960-1971` `ExternalPtrStmt` TensorType branch: `SExt` to
  i64 then multiply by an explicit i64 constant.
- `codegen_llvm.cpp:2385-2386` `GlobalTemporaryStmt`: `get_constant((int64)stmt->offset)`
  at `:2386`. **Line corrected from `:2385`.**
- `codegen_llvm.cpp:2393-2396` `ThreadLocalPtrStmt`: `offset` is `std::size_t`,
  so `get_constant` emits `APInt(64,...)` (`llvm_context.cpp:741-745`).
- `codegen_llvm.cpp:279` `element_size` as `get_constant((uint64)...)`.

**Withdrawn from this list.** Revision 1 listed `codegen_llvm.cpp:280-281`
`max_num_elements` here and called it "the only piece of SNode geometry already
64-bit on both sides". The emission is 64-bit and `StructMeta::max_num_elements`
is `i64` at `runtime.cpp:310` (**not `:307`, which is `struct StructMeta {`**),
but the value is truncated on the way back out: the function-pointer field is
`i32 (*get_num_elements)(Ptr, Ptr)` at `runtime.cpp:318` and
`Dense_get_num_elements` at
`taichi/runtime/llvm/runtime_module/node_dense.h:10-12` returns the `i64` field
through an `i32` return type. **[V]** So the one quantity that is 64-bit in the
struct is narrowed by its own accessor, and the claim as written was wrong.

### 3.2 SPIR-V path

**The pointer representation itself**

| File:line | Assumption |
|---|---|
| `spirv/spirv_codegen.cpp:82` | `const bool use_64bit_pointers = false;` Hardcoded. Used only at `:2318`. |
| `spirv/spirv_codegen.cpp:2322` | `uint_immediate_number(u32_type(), uint32_t(offset))` — explicit narrowing of a `size_t`. |
| `spirv/spirv_ir_builder.cpp:334-353` | `from_taichi_type`: a Taichi `PointerType` becomes `t_uint32_` unless `has_buffer_ptr`; the pointer branch is `:337-342`. **Spans corrected from `:334-341` and `:337-341` in revision 3**: the function closes at `:353` and the branch at `:342`. |
| `spirv/spirv_types.cpp:484-514` | `translate_ti_type`: same, `IntType(32, unsigned)` at `:495-496`, `IntType(64, unsigned)` at `:492-493` under `has_buffer_ptr`; the pointer branch is `:490-498`. **Span corrected twice** — revision 1 gave `:486-495`, revision 2 gave `:484-497`, and neither is the function. It opens at `:484` and closes at `:514`, which is what section 4.4 already said; revision 2 contradicted itself between the two sections. |
| `spirv/spirv_codegen.cpp:2213-2218` | `at_buffer`: byte pointer shifted right by `log2(width)` at `ptr_val.stype` (u32) at `:2214-2216` to make the SSBO element index, accessed at `:2217-2218`. **This is the 4 GiB root-buffer cap.** |

**Index arithmetic**

| File:line | Assumption |
|---|---|
| `spirv/spirv_codegen.cpp:533` | `LinearizeStmt` accumulator `const_i32_zero_`. Function opens at `:532`. |
| `spirv/spirv_codegen.cpp:535-536` | Strides as `int_immediate_number(i32_type(), ...)`. |
| `spirv/spirv_codegen.cpp:708` | `GlobalTemporaryStmt` narrows a `std::size_t` offset to i32. **Diverges from the LLVM path, which uses i64 at `codegen_llvm.cpp:2386`.** See the note below on why this has no live consequence today. |
| `spirv/spirv_codegen.cpp:727, 729` | `ExternalTensorShapeAlongAxisStmt` reads shapes as i32. |
| `spirv/spirv_codegen.cpp:737` | `ExternalPtrStmt` linear offset accumulator i32. |
| `spirv/spirv_codegen.cpp:752-755` | Shape loads i32. |
| `spirv/spirv_codegen.cpp:764-765` | Element-shape immediates i32. |
| `spirv/spirv_codegen.cpp:770-771` | Mul/Add chain i32. |
| `spirv/spirv_codegen.cpp:773-777` | `OpShiftLeftLogical` by `log2(element size)` at i32 — the byte offset into an external array is i32. **Span corrected from `:772-776`.** |
| `spirv/spirv_codegen.cpp:778-781` | Decorates that offset `DecorationNoSignedWrap` when the cap is present. Taichi is telling the driver the i32 offset does not overflow. At scale that becomes a false promise and the result is undefined, not merely wrapped. **Span corrected from `:777-781`.** |
| `spirv/spirv_codegen.cpp:2000-2004` | Range-for `begin_expr_value` and `total_elems` as i32. |
| `spirv/spirv_codegen.cpp:1999` | `const int num_elems = range_for_attribs.end - range_for_attribs.begin;` — a `size_t` difference (`taichi/codegen/spirv/kernel_utils.h:110-111`) narrowed to `int`. |
| `spirv/spirv_codegen.cpp:2004` | `advisory_total_num_threads` is `int` (`taichi/codegen/spirv/kernel_utils.h:99`). |
| `spirv/spirv_codegen.cpp:2018-2021, 2031-2035` | Global-temporary byte offsets converted to word indices by an i32 `OpShiftRightArithmetic` by 2. |
| `spirv/spirv_codegen.cpp:2054-2055` | `cast(i32_type(), get_global_invocation_id(0))` — the u32 invocation id is cast **down** to i32. |
| `spirv/spirv_codegen.cpp:2066-2071` | `total_invocs` computed at u32, cast to i32. **Span corrected from `:2064-2070` in revision 3**; `:2064` is blank and the statement runs to `:2071`. Found by this pass, by neither adversary. |
| `spirv/spirv_codegen.cpp:2093, 2096` | i32 phi induction variable (`make_phi(begin_.stype, 2)` at `:2093`, registered as `"ii"` at `:2094`) and a signed `lt` at `:2096`. **Corrected from `:2095, 2097` in revision 3**: `:2095` is `set_incoming`, `:2097` is `OpLoopMerge`. Both adversaries found this. |
| `spirv/spirv_codegen.cpp:1791, 1823, 1825` | Serial `RangeForStmt` step uses `const_i32_one_`. **Corrected from `:1791, 1821, 1823` in revision 3**: `:1821` is `spirv::Value next_value;` and carries no such use, and `:1825` was omitted. Those three are the only occurrences between `:1780` and `:1840`. Both adversaries found this. |
| `spirv/spirv_codegen.cpp:2138-2141, 2149, 2156, 2164-2168, 2175-2181` | Struct-for listgen buffer, count, index variable, entries and grid-stride increment all `u32_type()`. The index alloca is `alloca_variable(ir_->u32_type())` at `:2149`. **Corrected from `:2146` in revision 3**: that line is `spirv::Label loop_body = ir_->new_label();` and carries no `u32_type()`. The loop test at `:2157` is `OpULessThan` — **unsigned**, unlike every other loop in either path. |
| `spirv/spirv_codegen.cpp:405, 411, 412, 414` | `bitmasked_activation` hardcodes `u32_type()` for the bitmask word pointer, the shift and the access — **independently of `make_pointer`**, so bitmasked SNodes stay u32 even if the pointer width were raised. Citation re-verified in revisions 2, 3 and 4 and unchanged; see sections 7.1, 7.12 and 7.13. **Of the four, only `:411` breaks under widening. Corrected in revision 4**: `:405` and `:412` are both Shift operands, which the validator in this tree tests for type (`external/SPIRV-Tools/source/val/validate_bitwise.cpp:93-97`) and dimension (`:99-102`) and never for width; and `:414` is `struct_array_access`'s `res_type`, the pointee type of an `OpAccessChain` over a buffer of 32-bit bitmask words, which must stay u32. Revision 3 named `:412` and `:414` as breaking and both are wrong. **Three separate sites, `:392-394`, `:395-397` and `:398-399`, break in the opposite direction**; see section 6. |
| `spirv/spirv_ir_builder.cpp:164-165, 309-310, 323-324` | i32/u32 are the only integer types declared unconditionally (`:164-165`) and the only two returned by `get_primitive_type` without a capability test (`:309-310`, `:323-324`). **i32 branch line corrected from `:308-309`.** |
| `spirv/spirv_ir_builder.cpp:565, 586-591, 605` | `get_array_type` takes `uint32_t num_elems` (`:565`), emits the array length as a u32 immediate at `:576`, and narrows `container_stride` (a `size_t`) into a `uint32_t nbytes` at `:591` for the `DecorationArrayStride` emitted at `:605`. The only diagnostic is an `nbytes == 0` warning at `:596-602`. **Region corrected from `:583-589`.** |
| `spirv/spirv_ir_builder.h:149-159` | `InstrBuilder` has exactly one scalar `add` overload, `ADD(uint32_t, v)` at `:158`. Every SPIR-V literal operand is a 32-bit word. `size_t` values passed to `decorate` narrow implicitly — this is the mechanism at `spirv_types.cpp:451-452, 471`. **A SPIR-V format constraint, not a Taichi choice.** **[I]** for the characterisation; **[V]** for the code. |
| `spirv/spirv_ir_builder.cpp:963-979, 981-997` | `gl_LocalInvocationID` and `gl_GlobalInvocationID` are declared as `t_v3_uint_` and loaded as `t_uint32_`; `get_num_work_groups` likewise returns `t_uint32_` at `:960`. **Fixed by the SPIR-V builtin definitions.** A per-dimension dispatch index cannot exceed 2^32; any wider global index must be reconstructed. **[I]** for the reconstruction consequence. |

**On `GlobalTemporaryStmt`.** Revision 1 listed the LLVM/SPIR-V width
divergence without noting that it is bounded. `taichi_global_tmp_buffer_size`
is `1024 * 1024` (`taichi/inc/constants.h:15`), and the offset is asserted
against it at `taichi/transforms/offload.cpp:358` inside `allocate_global`.
**[V]** So the i32 at `spirv_codegen.cpp:708` is not a live truncation and does
not become one unless that constant moves. This is a prioritisation fact, not a
retraction: the divergence is real and the assertion is what makes it harmless.

**Not 32-bit-limited in the SPIR-V path:**
- The host-side layout computation. `SNodeDescriptor::cell_stride`,
  `container_stride`, `total_num_cells_from_root`,
  `mem_offset_in_parent_cell` and `CompiledSNodeStructs::root_size` are all
  `size_t` (`snode_struct_compiler.h:16, 19, 28, 31, 42`), and
  `compute_snode_size` accumulates in `std::size_t` throughout. **[V]**
- The root buffer's SPIR-V type. `buffer_argument` requests
  `get_struct_array_type(value_type, 0)` (`spirv_ir_builder.cpp:747`), which
  emits `OpTypeRuntimeArray` (`:581-583`). No static size cap from the type.
  **[V]**
- `spirv_codegen.cpp:504-508` `SNodeLookupStmt` **casts the index up to the
  pointer type before multiplying by the stride** (`:504-505`). If
  `make_pointer` returned u64, that multiply would happen at 64 bits with no
  further change. The LLVM path has no equivalent widen step. **[V]**
  **Qualified in revision 4, and the qualification is the point.** "No further
  change" is true of this multiply and **false of the visitor**. The same
  function's bitmasked branch reads the same `stmt->input_index->raw_name()` at
  `:488-489` with **no** cast and hands it to `bitmasked_activation`, sixteen
  lines above the cast at `:504-505`. So this row records a widen step that one
  of the visitor's two branches performs and the other omits. Section 6. **[V]**

---

## 4. SPIR-V Int64: declared, checked, assumed, or absent

**This section replaces revision 1's section 4 in full.** Revision 1 said
"checked at every use". That is false. Everything below is **[V]**, each line
opened during this revision.

### 4.1 Declared, conditionally

`spirv_ir_builder.cpp:64-66`:

```
if (caps_->get(cap::spirv_has_int64)) {
  ib_.begin(spv::OpCapability).add(spv::CapabilityInt64).commit(&header_);
}
```

The types are declared under the same guard, in `init_pre_defs()`:
`t_int64_` and `t_uint64_` at `spirv_ir_builder.cpp:166-169`. When the
capability is absent they remain default-constructed `SType`, and
`SType::id` has the default member initialiser `uint32_t id{0}`
(`spirv_ir_builder.h:51`). **Zero is not a valid SPIR-V result id.**

The same pattern governs the other optional widths: int8 at `:156-159`, int16
at `:160-163`, float16 at `:171-173`, float64 at `:174-176`. i32, u32, f32 and
bool are unconditional (`:155`, `:164-165`, `:170`).

### 4.2 Checked on one type-lowering path

`IRBuilder::get_primitive_type` (`spirv_ir_builder.cpp:288-332`) tests the
capability before returning any optional type. Exact lines, corrected from
revision 1:

| Type | `else if` opens | capability test | `TI_ERROR` | return |
|---|---|---|---|---|
| f16 | `:291` | `:292` | `:293` | `:294` |
| f64 | `:297` | `:298` | `:299` | `:300` |
| i8 | `:301` | `:302` | `:303` | `:304` |
| i16 | `:305` | `:306` | `:307` | `:308` |
| i64 | `:311` | `:312` | `:313` | `:314` |
| u8 | `:315` | `:316` | `:317` | `:318` |
| u16 | `:319` | `:320` | `:321` | `:322` |
| u64 | `:325` | `:326` | `:327` | `:328` |

i32 (`:309-310`) and u32 (`:323-324`) alone have no guard. Revision 1 gave the
i64 guard as `:310-313` and f16/f64/i16 spans that were each one or more lines
off; those citations are corrected here. Revision 1's u64 citation `:325-328`
was right.

### 4.3 Unchecked on the path that compiles kernel signatures

`Translate2Spirv::visit_int_type` (`spirv_types.cpp:393-419`) and
`Translate2Spirv::visit_float_type` (`:421-431`) **do not call
`get_primitive_type`**. They call the bare accessors on `IRBuilder`, each of
which is a one-line `return t_*;` with no `caps_` consultation:

| Call site | Accessor | Accessor decl | Guarded equivalent |
|---|---|---|---|
| `spirv_types.cpp:397` | `i8_type()` | `spirv_ir_builder.h:559-561` | `spirv_ir_builder.cpp:301-304` |
| `spirv_types.cpp:399` | `i16_type()` | `spirv_ir_builder.h:549-551` | `:305-308` |
| `spirv_types.cpp:403` | `i64_type()` | `spirv_ir_builder.h:529-531` | `:311-314` |
| `spirv_types.cpp:409` | `u8_type()` | `spirv_ir_builder.h:562-564` | `:315-318` |
| `spirv_types.cpp:411` | `u16_type()` | `spirv_ir_builder.h:552-554` | `:319-322` |
| `spirv_types.cpp:415` | `u64_type()` | `spirv_ir_builder.h:532-534` | `:325-328` |
| `spirv_types.cpp:424` | `f16_type()` | `spirv_ir_builder.h:555-557` | `:291-294` |
| `spirv_types.cpp:428` | `f64_type()` | `spirv_ir_builder.h:535-537` | `:297-300` |

**Eight call sites, eight optional types, five capability flags**
(`spirv_has_int8`, `spirv_has_int16`, `spirv_has_int64`, `spirv_has_float16`,
`spirv_has_float64`). The defect is not specific to Int64; Int64 is one of
five. I state the count explicitly because the arbitration note gave it as
"eight accessors across six optional widths" and the enumeration above is what
the source supports.

The result is written out unconditionally at `spirv_types.cpp:418`
(`ir_node_2_spv_value[type] = vt.id;`) and `:430`. Nothing validates the id.
The only assertions in `spirv_types.cpp` are at `:17`, `:47`, `:71`, `:336`,
`:346`, `:356`, `:365`, none on `vt.id`.

### 4.4 That path is live

`Translate2Spirv` is reached only through `ir_translate_to_spirv`
(`spirv_types.cpp:476-483`), which is called from exactly three sites, all in
kernel compilation:

- `spirv_codegen.cpp:2386` — `compile_args_struct` (`:2326-2400`), the kernel
  argument struct.
- `spirv_codegen.cpp:2464` — **`compile_argpack_struct`** (`:2402-2481`), the
  argpack struct. The next statement, `:2465`, is
  `argpack_struct_type.id = ir2spirv_map[struct_type];`.
- `spirv_codegen.cpp:2511` — **`compile_ret_struct`** (`:2483-2527`), the return
  struct. The next statement, `:2512`, is
  `ret_struct_type_.id = ir2spirv_map[struct_type];`.

**Correction, revision 3, and it is the best finding of round two.** Revisions 1
and 2 of this report paired `:2464` with `compile_ret_struct` and `:2511` with
`compile_argpack_struct` — while printing the function spans `:2483-...` and
`:2402-...` alongside them. A line at 2464 cannot lie inside a function that
opens at 2483, and a line at 2511 cannot lie inside one that ends at 2481. This
report supplied the evidence that disproved its own attribution and did not
notice. `report-02-codegen.md` carried the same swap, and so did both round-one
adversaries. Adversary2 02-1 §3.1 found it; adversary2 02-2 §10.2 confirmed it
independently. I re-derived it by extracting the function openings, closings and
the three call sites from `spirv_codegen.cpp:2320-2530`, and they are right.

The set of three affected structs is unchanged. What changes is where a reader
looks: the five `translate_ti_type` call sites are `:2344` and `:2358` in
`compile_args_struct`, `:2420` and `:2436` in `compile_argpack_struct`, and
`:2494` in `compile_ret_struct`. Escalation 10 named those five as a candidate
fix location, so anyone acting on revision 2 would have opened the wrong
function first.

The types fed in come from `translate_ti_type` (`spirv_types.cpp:484-514`) via
`translate_ti_primitive` (`:167-214`), which maps `PrimitiveType::i64` to
`IntType(64, signed)` at `:179-181` and `u64` to `IntType(64, unsigned)` at
`:199-201`. `translate_ti_primitive` has **no capability parameter in its
signature at all** (`:167-168`).

**No earlier check saves it.** `compile_args_struct` is entered from
`get_buffer_value` at `spirv_codegen.cpp:2276`. `get_buffer_value` does call
`ir_->get_primitive_type(dt)` first, at `:2267` — but the `dt` supplied by
`visit(ArgLoadStmt*)` is the hardcoded placeholder
`get_buffer_value(BufferType::Args, PrimitiveType::i32)` at `:618` (also
`:728`, `:753`, `:788`, `:2293`). So the guarded function is called with i32,
returns cleanly, and then `compile_args_struct` translates **every** element of
`ctx_attribs_->args_type()->elements()` through the unguarded path
(`:2356-2367`, with the nested lambda at `:2343-2355`). One i32 argument is
enough to trigger translation of all of them.

The zero id then propagates into `OpTypeStruct` via `visit_struct_type`
(`spirv_types.cpp:440-452`), and `visit(ArgLoadStmt*)` builds
`get_pointer_type(val_type, StorageClassUniform)` from it at
`spirv_codegen.cpp:619-622`. Nothing on that route consults `caps_`.

**And nothing outside codegen rejects a 64-bit kernel argument.**
`Extension::data64` is declared at `taichi/inc/extensions.inc.h:6` and granted
to x64, arm64 and cuda at `taichi/program/extension.cpp:12, 16, 20`. A grep for
`data64` across the C++ core returns only those four sites plus a commented-out
OpenGL grant at `extension.cpp:29-30`. **The extension is never read anywhere in
the C++ core.**

**Correction, revision 3.** Revision 2 wrote "a tree-wide grep for `data64`
returns only those four sites plus a commented-out OpenGL grant." That is false
as written. A tree-wide grep also returns `python/taichi/lang/misc.py:186`,
`docs/lang/articles/contribution/write_test.md`, and roughly fifty sites under
`tests/python/`. The conclusion is unaffected, because the Python front end is
out of scope under the brief's section 1.2 and none of those sites is a C++
consumer — but the sentence claimed a quantifier over the tree that the tree
does not support, which is the same species of error this report correctly
pinned on adversary 02-2's `is_extension_supported` count. Adversary2 02-1 §4.2
found it. The claim is now scoped to what was actually swept. (Adversary 02-2 supported this with the claim that
`is_extension_supported` is called exactly once in the tree. That is not so —
it is called at `taichi/program/program.cpp:149`, `codegen_llvm.cpp:2726` and
six times in `taichi/transforms/compile_to_offloads.cpp`. But none of those
calls names `data64`, so their conclusion stands and only their supporting
statement is wrong. See section 7.5.)

**Failure mode.** A kernel taking an `i64` argument, compiled for a device that
does not report `spirv_has_int64`, emits an `OpTypeStruct` referencing result
id 0 rather than raising the named error.

**What then happens to that module. Traced end to end in revision 3, and this
answers a question revision 2 got wrong by one word.** Revision 2 said the
module escapes *silently*. It does not. **[V]** for every link below; the
SPIRV-Tools submodule is checked out at `external/SPIRV-Tools`, so this is read
rather than assumed.

1. **Nothing in Taichi validates it.** `spirv_opt_options_.set_run_validator(false);`
   at `spirv_codegen.cpp:2710`, unconditional — it sits *outside* the
   `if (params.enable_spv_opt)` block, which opens at `:2683` and closes at
   `:2709`. `spirv_tools_` is constructed at `:2712` and its only use is
   `Disassemble` at `:2763`, inside `if constexpr (false)` at `:2758`. Revision
   2 named only the pass pipeline and `enable_spv_opt`
   (`taichi/codegen/spirv/kernel_compiler.cpp:38`); that is true and is not the
   load-bearing fact. A reader of revision 2 would conclude the backstop is
   conditional. It is switched off.
2. **The optimiser runs regardless.** `spirv_opt_->Run(...)` at
   `spirv_codegen.cpp:2746-2747` executes on every kernel; only the pass
   *registrations* at `:2685-2708` are gated on `enable_spv_opt`.
3. **`Optimizer::Run` parses the module whether or not the validator is on.**
   `external/SPIRV-Tools/source/opt/optimizer.cpp:584-598`: the `tools.Validate`
   call is skipped when `run_validator_` is false (`:590-594`), and then
   `BuildModule(...)` is called **unconditionally** at `:596-597`, returning
   false if it yields null at `:598`.
4. **`BuildModule` runs the binary parser.**
   `external/SPIRV-Tools/source/opt/build_module.cpp:56-75`: `spvBinaryParse` at
   `:68-69`, returns null unless the parse returns `SPV_SUCCESS` at `:74`.
5. **The parser rejects a zero id.** `external/SPIRV-Tools/source/binary.cpp:450`
   ("Error: Type Id is 0"), `:456` ("Error: Result Id is 0"), and `:473`
   ("Id is 0") for `SPV_OPERAND_TYPE_ID` — which is what an `OpTypeStruct`
   member operand is.
6. **The error reaches Taichi's own consumer and is downgraded to a warning.**
   The consumer is installed at `spirv_codegen.cpp:2682` and is
   `spriv_message_consumer` at `:2641-2659`. Its first branch is
   `if (level <= SPV_MSG_FATAL)` at `:2646`, and in the enum at
   `external/SPIRV-Tools/include/spirv-tools/libspirv.h:83-95` `SPV_MSG_FATAL`
   is **0** while `SPV_MSG_ERROR` is **2**. So a parse error falls through to
   `else if (level <= SPV_MSG_WARNING)` at `:2649` and is emitted as `TI_WARN`.
   The branch order, not the severity, decides this.
7. **`Run` returning false is recorded and then discarded.**
   `TI_WARN_IF(..., "SPIRV optimization failed")` at `:2745-2748`, then
   `success = false` at `:2750`. `success` is read **only** at `:2760`, inside
   the `if constexpr (false)` block at `:2758-2772`. Line `:2775` pushes
   `std::move(optimized_spv)` onto `generated_spirv` regardless, and on a failed
   `Run` that vector still holds the unmodified copy made at `:2740`.

**Net, and this is a third severity distinct from either alternative the
arbitration named.** The invalid module is detected inside Taichi's own process,
before any driver sees it; it produces two warnings; and it is then shipped
unchanged because the flag recording the failure is dead code. It is neither
silent nor fatal.

**Adjudication against adversary2 02-1.** That adversary's §2.1 point 3 states
there is "no Taichi-level diagnostic" and point 4 that the failure is
"deferred to the driver at shader-module creation, not absent", and its §6 asks
both reports to add a sentence saying so. **I traced it myself and that sentence
would be false.** There are two Taichi-level diagnostics, both `TI_WARN`, and
the first point of detection is in-process. Adversary2 02-2's §2.1 and §10.3
give the chain above and the source supports them. Adversary2 02-1 is right
that a zero `<id>` is a hard parse failure for any conformant consumer — the
parser it is caught by is exactly such a consumer — and right that nothing in
this tree tells you what a driver would do with the module that is nonetheless
shipped. Section 7.9 records the adjudication.

**Consequence for 6.2, stated as narrowly as the source allows.** This is two
work items, not one. A guard at the type-translation boundary is one. The other
is that `spirv_codegen.cpp:2742-2775` computes a failure flag and never acts on
it. Whether either should change is in escalations.

### 4.5 Assumed outright, by two device backends

**Scope note, revision 3.** Section 4.3 establishes that the bypass covers eight
accessors under **five** capability flags. Revision 2 then enumerated the target
landscape for `spirv_has_int64` alone. Section 4.5.1 below extends it to all
five, as adversary2 02-2 §6.1 required. This subsection keeps the Int64 table
because Int64 is the capability section 6.2 of the brief names.

Every setter of `spirv_has_int64`, from a grep across `taichi/` and `c_api/`:

| Setter | Guard | Verdict |
|---|---|---|
| `taichi/rhi/vulkan/vulkan_device_creator.cpp:632` | `if (device_supported_features.shaderInt64)` at `:630` | queried |
| `taichi/rhi/opengl/opengl_device.cpp:511` | `if (!is_gles())` at `:509`, comment "64bit isn't supported in ES profile" at `:510` | profile-based, no feature query |
| `taichi/rhi/metal/metal_device.mm:1052` | `if (feature_64_bit_integer_math)` at `:1051`, where `feature_64_bit_integer_math = family_apple3` at `:1038`, from `[mtl_device supportsFamily:kMTLGPUFamilyApple3]` at `:1035-1036` | queried |
| `c_api/src/taichi_opengl_impl.cpp:9` | **none** | asserted unconditionally |
| `taichi/rhi/dx/dx_device.cpp:563-565` | never set — `Dx11Device::Dx11Device` (`:557-568`) builds `DeviceCapabilityConfig caps{}` at `:563`, sets only `spirv_version` at `:564`, calls `set_caps` at `:565` | absent |

Consequences for section 6.2's stub requirement:

- **OpenGL ES is not the only in-tree SPIR-V target lacking Int64.**
  Pre-Apple3 Metal lacks it, and Direct3D 11 sets no integer capability at all.
- **The C API OpenGL runtime discards the GLES guard.**
  `OpenglRuntime::OpenglRuntime` (`c_api/src/taichi_opengl_impl.cpp:4-13`)
  builds a fresh `DeviceCapabilityConfig`, sets `spirv_has_int64` at `:9` and
  `spirv_has_float64` at `:10`, then calls `get_gl().set_caps(std::move(caps))`
  at `:12`. That is a whole-object replacement of the config `GLDevice::GLDevice`
  built under `if (!is_gles())` at `opengl_device.cpp:509-513`. The entry point
  `ti_import_opengl_runtime` takes `bool use_gles`
  (`taichi_opengl_impl.cpp:21-22`) and forwards it to `set_gles_override` at
  `:26` before constructing the runtime at `:28`. So on that path GLES claims
  Int64. **[V]** for the code. I have **not** established that any shipped
  configuration exercises it, and I am not asserting a live failure — only that
  the capability can be affirmed by a target that cannot execute it. That file
  is outside `taichi/` and outside my territory; it goes to escalations.
- There is one consumer of the capability outside codegen:
  `taichi/rhi/metal/metal_device.mm:131-135` raises the MSL version to 2.3 when
  `caps.contains(DeviceCapability::spirv_has_int64)`. **[V]** Neither round-one
  adversary noted it. It is a reader, not a guard.

### 4.5.1 The same question, asked of all five bypassed capabilities

**New in revision 3.** All **[V]**. The five optional scalar-type capabilities
are enumerated canonically at `taichi/inc/rhi_constants.inc.h:12-16`. I grepped
`taichi/` and `c_api/` for every `caps.set(...)` of each. **Seventeen setters,
across five capabilities and four device layers**, counted from the rows below:

| Capability | Setters | Guard |
|---|---|---|
| `spirv_has_int8` | `metal_device.mm:1046` | none — unconditional |
| | `vulkan_device_creator.cpp:790` | `shader_f16_i8_feature.shaderInt8` at `:789` |
| `spirv_has_int16` | `metal_device.mm:1047` | none — unconditional |
| | `vulkan_device_creator.cpp:628` | `device_supported_features.shaderInt16` at `:626` |
| | `opengl_device.cpp:516` | `GLAD_GL_NV_gpu_shader5` at `:515` |
| | `opengl_device.cpp:521` | `GLAD_GL_AMD_gpu_shader_int16` at `:520` |
| `spirv_has_int64` | `metal_device.mm:1052` | `feature_64_bit_integer_math` at `:1051` |
| | `vulkan_device_creator.cpp:632` | `device_supported_features.shaderInt64` at `:630` |
| | `opengl_device.cpp:511` | `!is_gles()` at `:509` |
| | `c_api/src/taichi_opengl_impl.cpp:9` | none — unconditional |
| `spirv_has_float16` | `metal_device.mm:1048` | none — unconditional |
| | `vulkan_device_creator.cpp:787` | `shader_f16_i8_feature.shaderFloat16` at `:786` |
| | `opengl_device.cpp:517` | `GLAD_GL_NV_gpu_shader5` at `:515` |
| | `opengl_device.cpp:525` | `GLAD_GL_AMD_gpu_shader_half_float` at `:524` |
| `spirv_has_float64` | `vulkan_device_creator.cpp:636` | `device_supported_features.shaderFloat64` at `:634` |
| | `opengl_device.cpp:512` | `!is_gles()` at `:509` |
| | `c_api/src/taichi_opengl_impl.cpp:10` | none — unconditional |

Seventeen rows: two for int8, four for int16, four for int64, four for float16,
three for float64. Counted from the table, not stated alongside it.

Four consequences the Int64-only table could not show.

1. **Metal reports no Float64, on any device, ever.**
   `taichi/rhi/metal/metal_device.mm` contains no occurrence of the string
   `float64` anywhere in the file. It sets int8, int16 and float16
   unconditionally at `:1046-1048` and Int64 under a family test at
   `:1051-1052`, and never sets float64. `t_fp64_` is declared only inside
   `if (caps_->get(cap::spirv_has_float64))` at `spirv_ir_builder.cpp:174-176`,
   so it stays default-constructed with `id == 0`. Combined with the bare
   `f64_type()` at `spirv_types.cpp:428`, **an `f64` kernel argument, return
   value or argpack member compiled for Metal emits an `OpTypeStruct`
   referencing id 0 on every Metal device, current hardware included.** This is
   a stronger instance of the defect than the Int64 one this report leads with,
   because it is unconditional rather than confined to a legacy subset.
   Adversary2 02-2 §6.1 found it; I confirmed it by grepping the file and
   opening the declaration site. It goes to escalations, not to a conclusion:
   whether Metal genuinely cannot do f64 or whether the capability was simply
   never wired is not determinable from this tree.
2. **Direct3D 11 and imported Vulkan lack all five, not just Int64.**
   `taichi/rhi/dx/dx_device.cpp:563-565` sets only `spirv_version`.
   `VulkanRuntimeImported::Workaround::Workaround`
   (`c_api/src/taichi_vulkan_impl.cpp:19-56`, declared at
   `c_api/src/taichi_vulkan_impl.h:27-31`) builds `DeviceCapabilityConfig caps{}`
   at `:35`, sets only `spirv_version` at `:37-43`, and calls `set_caps` at
   `:53`; the physical-storage-buffer set at `:47-50` is inside a `/* */` block
   opened at `:46` and closed at `:51`. Revision 2 said Direct3D 11 "sets no
   integer capability at all", which is narrower than the truth, and did not
   carry imported Vulkan at all.
3. **The C API OpenGL override removes capabilities as well as adding them.**
   `c_api/src/taichi_opengl_impl.cpp:8-12` sets exactly `spirv_has_int64`,
   `spirv_has_float64` and `spirv_version`, and `Device::set_caps`
   (`taichi/rhi/public_device.h:855-857`) is `caps_ = std::move(caps);`, a
   whole-object replacement. So it discards any `spirv_has_int16` or
   `spirv_has_float16` that the `GLAD_GL_NV_gpu_shader5`,
   `GLAD_GL_AMD_gpu_shader_int16` and `GLAD_GL_AMD_gpu_shader_half_float` probes
   at `opengl_device.cpp:515-526` had granted. Revision 2 recorded the addition
   and not the removal.
4. **Any embedder can assert or clear any of the five from outside the library.**
   `ti_set_runtime_capabilities_ext` (`c_api/src/taichi_core_impl.cpp:317-334`)
   builds a fresh `DeviceCapabilityConfig` at `:325`, fills it from the caller's
   array at `:326-330` with no device query and no validation of the enum value,
   and installs it with `set_caps` at `:331` — the same whole-object replacement.
   It is a documented public entry point. Revision 2 did not mention it and
   carried only the OpenGL constructor override, which is one hardcoded instance
   of what this offers to every caller. Both round-two adversaries found this
   independently and both recorded it as a gap in this report; they are right.
   **[V]** for the code. I have **not** established that any shipped
   configuration exercises it. It is outside `taichi/` and outside my territory,
   and it goes to escalations.

**What this does to 6.2's stub requirement, stated as a fact about the source
and not as a proposal.** The architecture is not being asked to express "some
older targets lack Int64". Taken across the table above, **no in-tree backend
sets all five**: Metal sets four and never float64; Vulkan sets all five but
each behind its own query; OpenGL sets four and never int8; Direct3D 11 and
imported Vulkan set none. The shape is a five-by-five matrix with two empty
rows, one permanently incomplete row, and two entry points that can overwrite
any of it from outside. Under the brief's section 5.2 closing consequence, a
portable path that emits an invalid module for an `f64` argument on an entire
backend is a gap in the portable path to record, not evidence that the backend
matters less.

### 4.6 One further unguarded read, and one dead switch

`from_taichi_type` (`spirv_ir_builder.cpp:334-353`, **span corrected from
`:334-341` in revision 3**; the pointer branch is `:337-342`) returns
`t_uint64_` when `has_buffer_ptr` is true (`:338-339`), with **no**
`spirv_has_int64` check of its own. The only guard is in the RHI: the physical-storage-buffer setter sits
inside `if (device_supported_features.shaderInt64)` at
`vulkan_device_creator.cpp:821`. **Line corrected from `:823` in revision 1;
`:823` is a URL comment.** The invariant "physical storage buffer implies
Int64" is enforced in the device layer, not in the builder that depends on it.

And the switch is dead. `spirv_has_physical_storage_buffer` is set in exactly
one place tree-wide, `vulkan_device_creator.cpp:826`, inside
`#if !defined(__APPLE__) && false` at `:825` with `#endif` at `:827`.
**Span corrected from `:820-828`.** So `has_buffer_ptr` is false everywhere
today.

**Correction to revision 1.** Revision 1 concluded from this that "every
64-bit-pointer branch in the SPIR-V path is unreachable" and listed
`at_buffer`'s `:2197-2205` among them. That is wrong. The capability-gated
branches are `spirv_codegen.cpp:783` (`ExternalPtrStmt`), `:2340`, `:2416`,
`:2491`, and `spirv_ir_builder.cpp:73` (the `OpCapability`) and `:113` (the
extension and the physical-storage addressing model). But `at_buffer` branches
on `ptr_val.stype.dt == PrimitiveType::u64` at `spirv_codegen.cpp:2197` — on
the SType's data type, **not** on the capability. `declare_primitive_type` sets
`t.dt = dt` at `spirv_ir_builder.cpp:1536`, so `u64_type()` carries
`dt == u64` whenever Int64 was declared. That branch is therefore reachable the
moment a u64-typed pointer value exists, independently of the physical-storage
capability. Section 6 gives what that means for flipping the switches.

### 4.7 Assessment against section 6.2's stub requirement

**Withdrawn:** revision 1's conclusion that "the SPIR-V codegen already
expresses a target that lacks Int64 correctly" and "the mechanism the brief
asks for exists".

**What the source supports instead.** There are two type-lowering paths. One,
`IRBuilder::get_primitive_type`, has the mechanism and fails with a named error
(section 4.2). The other, the tinyir translation in `Translate2Spirv`, has no
mechanism at all and emits type id 0 (sections 4.3, 4.4) — and that is the path
the kernel argument, return and argpack structs take.

**Revision 3 widens the target side of this.** For Int64 alone, three in-tree
device backends can present without it and one affirms it without checking
(section 4.5). Across all five bypassed capabilities the picture is worse: no
in-tree backend sets all five, Metal never sets `spirv_has_float64` on any
device, Direct3D 11 and imported Vulkan set none of the five, and two C API
entry points can install an arbitrary capability set over whatever the device
layer detected (section 4.5.1). Nothing outside codegen compensates
(section 4.4), and what the invalid module then meets is two warnings and a
discarded failure flag rather than a rejection (section 4.4).

So for 6.2: a target lacking Int64 can be *described*, and the description is
honoured for statement-level types and ignored for kernel signature types. Any
6.2 work that assumes "it already fails loudly" would be relying on a guarantee
that holds on one path and not the other. What closes the hole is a design
decision and is in escalations, not here.

Separately, and unchanged from revision 1: no consumer of the Int64 mechanism
exists for *indices*. Indices are i32 unconditionally, so no target has ever
exercised the i64-absent path for addressing. **[I]**, following from the
inventory in 3.2.

---

## 5. The assertion at `struct_llvm.cpp:266`, and the store it does not guard

**[V]**, each step re-read this pass.

### 5.1 The chain

- The assertion is `TI_ASSERT((int)snodes.size() <= taichi_max_num_snodes);`
  at `taichi/codegen/llvm/struct_llvm.cpp:266`, inside
  `StructCompilerLLVM::run(SNode &root)` (`:247-...`).
- `snodes` is `StructCompiler::snodes` (`taichi/struct/struct.h:11`), filled by
  `collect_snodes` (`taichi/struct/struct.cpp:7-13`) called at
  `struct_llvm.cpp:250`, which walks **one root**.
- `LlvmProgramImpl::compile_snode_tree_types_impl`
  (`taichi/runtime/program_impls/llvm/llvm_program.cpp:45-56`) constructs a
  **fresh** `StructCompilerLLVM` per SNode tree at lines 50-52 and calls
  `run(*root)` at line 53.
- Therefore the assertion bounds the SNode count **of a single tree**.
- The three arrays it stands in for
  (`taichi/runtime/llvm/runtime_module/runtime.cpp:567-569`) are indexed at
  `runtime.cpp:1005, 1016, 1029, 1038, 1271, 1287, 1288, 1334, 1336, 1429,
  1692, 1723, 1740, 1784`. **Fourteen subscripts, corrected in revision 3**: the
  list previously read `1334-1336`, and `:1335` is
  `int num_parent_elements = parent_list->size();`, not a subscript. This is the
  exact fault `report-02-codegen.md` found and corrected in itself and this
  report did not take. I re-derived the list by grepping `element_lists[`,
  `node_allocators[` and `ambient_elements[` over the file: those fourteen plus
  the three declarations at `:567-569`, and nothing else.
- `snode_id` is `SNode::id`, assigned from a **process-global**
  `static std::atomic<int> counter` (`taichi/ir/snode.cpp:12`, `:220`
  `id = counter++`). **The only live reset is `SNode::counter = 0;` at
  `taichi/program/program.cpp:144`**, inside the `Program` constructor, guarded
  by `TI_ASSERT_INFO(num_instances_ == 0, ...)` at `:141`. Not per tree, not on
  tree destruction. **Corrected in revision 3.** Revision 2 read "it is reset
  only by `SNode::reset_counter()` (`taichi/ir/snode.h:348-350`) and in
  `Program`'s constructor", presenting two live avenues where there is one.
  `SNode::reset_counter()` **has no caller anywhere**: a grep for
  `reset_counter` over `taichi/`, `c_api/`, `tests/` and `python/` returns
  exactly three hits — the definition at `taichi/ir/snode.h:348`, the unrelated
  `Stmt::reset_counter` at `taichi/ir/ir.h:509`, and the single call
  `Stmt::reset_counter();` at `taichi/program/program.cpp:347`, which calls the
  `ir.h` one. Both round-two adversaries flagged this and both are right. It
  matters for escalation 1: whether a per-tree reset "already exists and merely
  needs calling elsewhere" turns on this, and it does not exist.
- `kMaxNumSnodeTreesLlvm = 512` bounds the number of trees. It is defined at
  `taichi/inc/constants.h:13`; `runtime.cpp:562-563` are the two arrays sized
  by it. **Misattribution corrected from revision 1**, which cited
  `runtime.cpp:562-563` as the definition.
- `taichi_max_num_snodes = 1024` at `taichi/inc/constants.h:12`. A tree-wide
  grep gives exactly four use sites: `constants.h:12`, `struct_llvm.cpp:266`,
  and `runtime.cpp:567`, `:568`, `:569`. There is no compensating check
  anywhere.

### 5.2 What the bound actually guards: a range store, not a lookup

Revision 1 stopped at "the arrays are indexed by a global id". That was one
call site short. `runtime_initialize_snodes`
(`taichi/runtime/llvm/runtime_module/runtime.cpp:986-1017`) writes a
**contiguous range**:

```
for (int i = root_id; i < root_id + num_snodes; i++) {   // runtime.cpp:1003
  // TODO: some SNodes do not actually need an element list.
  runtime->element_lists[i] =                            // runtime.cpp:1005
      runtime->create<ListManager>(runtime, sizeof(Element), 1024 * 64);
}
```

`root_id` and `num_snodes` are parameters (`:988-989`). They are supplied from
`FieldCacheData::root_id`, read at
`taichi/runtime/llvm/llvm_runtime_executor.cpp:400`, and `(int)snode_metas.size()`
at `:444`; the call is at `:442-444`. `root_id` originates as
`int root_id = tree->root()->id;` at
`taichi/runtime/program_impls/llvm/llvm_program.cpp:61` — the **global** id of
the tree's root.

So tree *k* writes `element_lists[root_id .. root_id + n)` while the assertion
only ever sees that tree's own *n*. Once `root_id + num_snodes` exceeds 1024
this is an out-of-bounds **store** into the `LLVMRuntime` struct. The members
immediately following are `node_allocators` (`:568`), `ambient_elements`
(`:569`), `temporaries` (`:570`) and `rand_states` (`:571`). The failure mode
is silent corruption of the device runtime, not a diagnostic at the point of
error.

The other two arrays are written per global id the same way:
`runtime_NodeAllocator_initialize` writes `node_allocators[snode_id]` at
`runtime.cpp:1029` and `runtime_allocate_ambient` writes
`ambient_elements[snode_id]` at `:1038`, both called with `snode_metas[i].id`
from `llvm_runtime_executor.cpp:460-466`.

### 5.3 What narrows reachability, and what widens it

Two filters, neither of which removes the defect. **[V]**

1. `runtime_initialize_snodes` returns early when `all_dense`
   (`runtime.cpp:1000-1002`), so the range write is skipped for an entirely
   dense tree — **but only when the config field is left at its default.**
   `all_dense` is **seeded** from `config_.demote_dense_struct_fors` at
   `llvm_runtime_executor.cpp:402` and then narrowed by the loop at `:403-410`,
   which contains a single assignment, `all_dense = false;` at `:407`. The loop
   can only clear the flag; it can never set it. So
   `all_dense == demote_dense_struct_fors && every-node-is-dense/place/root`.
   `demote_dense_struct_fors` is a plain `bool` field
   (`taichi/program/compile_config.h:28`), defaulted **true** at
   `taichi/program/compile_config.cpp:18`, forced true for SPIR-V archs only at
   `:72-74` — which is irrelevant here, because those archs do not use this
   runtime — and writable from Python at
   `taichi/python/export_lang.cpp:201-202`.
2. The `node_allocators` and `ambient_elements` writes are filtered by
   `is_gc_able(snode_metas[i].type)` (`llvm_runtime_executor.cpp:447`), which
   is `pointer || dynamic` (`taichi/ir/snode_types.cpp:21-23`).

**Correction, revision 3. Revision 2's conclusion here was wrong, and it was
contradicted by a fact revision 2 recorded in the same paragraph.** Revision 2
read: "Both surviving routes therefore require **sparse** SNodes." It does not
follow, and the seed is precisely why. **With `config_.demote_dense_struct_fors`
false, `all_dense` is false regardless of tree shape, the early return at
`runtime.cpp:1000-1002` never fires, and the range store at `:1003-1007`
executes for EVERY tree, fully dense trees included.** **[V]** for each link.

The accurate statement, which is stronger and simpler than the withdrawn one:

- The `node_allocators` / `ambient_elements` route requires `pointer` or
  `dynamic` SNodes.
- The `element_lists` route requires **either** a non-dense SNode **or**
  `demote_dense_struct_fors == false`. It requires no sparsity at all in the
  second case.

**How this happened, recorded because the standing instructions §10.6 exist to
prevent exactly it.** Revision 2 added the seed as a *citation correction* to an
adversary's line range and did not propagate it into the conclusion the
paragraph draws. Revision 2's own §7.6 shows the correction being made in
isolation. Verifying that `:402` says what I quoted certified the citation and
nothing else; the sentence it sat next to was a claim about reachability, which
a citation cannot test. Both round-two adversaries pinned this, and adversary2
02-2 §5.3 further shows `report-02-codegen.md` carrying the same overstated
conclusion despite stating the seeding correctly eight lines earlier. On the
evidence I opened, that is right too, and it means the correction is owed by
both reports rather than only this one.

**Relevance to the brief, unchanged in direction and widened in reach.** Section
4.1 states that sparsity is required and that sparse machinery is not surplus,
so a sparse configuration trips this. What the correction adds is that a dense
configuration also trips it under a one-flag change from a defaulted-true public
field. Whether this project will ever set that field false is a configuration
decision nobody has made, and it goes to escalations. **[I]** for the relevance
judgement; **[V]** for the filters and the seed.

Widening it: SNode **tree** ids are recycled — `Program::destroy_snode_tree`
pushes onto `free_snode_tree_ids_` at `taichi/program/program.cpp:235`, popped
in `allocate_snode_tree_id` at `:559-567` — but SNode ids are not
(`taichi/ir/snode.cpp:220`). So reaching the overflow does not require many
concurrent trees, and `kMaxNumSnodeTreesLlvm = 512` is not a bound on it. A
process that creates and destroys small trees over its lifetime advances
`SNode::counter` monotonically towards 1024 regardless of how many are live at
once. **Revision 3: the word "sparse" is out of that sentence**, because the
correction above shows the `element_lists` route does not require sparsity when
`demote_dense_struct_fors` is false. **[V]** for each link; **[I]** for the arithmetic of how many cycles it
takes, which depends on tree shape.

`TI_ASSERT` is unconditional — it expands to `TI_ERROR` with no `NDEBUG` guard
(`taichi/common/logging.h:100-107`) — so the per-tree check is present in
release builds. It is a real, always-on check that measures the wrong quantity.
**[V]**

### 5.4 It is a post-hoc check

The assertion runs *after* `generate_child_accessors(root)` (line 258) and
after the optional IR dump (lines 260-264). **Lines corrected from revision 1's
`:253` and `:255-259`.** By the time it fires, the LLVM functions for every
SNode have already been emitted.

**Consequence.** Several trees, each individually under 1024 SNodes, can drive
the global id past 1024 without the assertion firing, and the result is an
out-of-bounds store. Whatever replaces 1024 has to decide which quantity it
bounds — per-tree SNode count, or the global id space that sizes the arrays.
Those are not the same number. I am reporting this and **not** proposing a
resolution; item 8.1.2 of the brief reserves the replacement value, and this
touches territory 03's arrays.

---

## 6. What would change in each path if index and address width became 64-bit

### LLVM path — **[I]**, built on the **[V]** inventory in 3.1

- The index side is the work. Widen `LinearizeStmt`, the range-for and
  struct-for loop variables, `LoopIndexStmt`'s two hardcoded loads,
  `get_range_for_bounds`, and `generate_refine_coordinates` including its host
  `int` arithmetic at `struct_llvm.cpp:174-175`.
- The address side is largely already correct because LLVM GEPs and pointers
  are 64-bit on the supported targets. `MatrixPtrStmt`'s byte branch,
  `GlobalTemporaryStmt` and `ThreadLocalPtrStmt` need nothing.
- **The narrowing that defines the LLVM half of 6.2 is the refine-coordinates
  ABI.** `l`, the third parameter, is i32 at `struct_llvm.cpp:153`, and
  `PhysicalCoordinates::val` is `i32[taichi_max_num_indices]` at
  `runtime.cpp:288-290`. The quantity `l` carries is the within-container
  linear index, bounded by `SNode::max_num_elements()`, which returns `int64`
  (`taichi/ir/snode.h:306-308`), is stored as `i64` in `StructMeta`
  (`runtime.cpp:310`) and is emitted at 64 bits (`codegen_llvm.cpp:280-281`).
  **[V]** So the container size is carried at 64 bits everywhere except this
  ABI and the `i32` accessor return at `runtime.cpp:318` /
  `node_dense.h:10-12`. Those two are the LLVM-side truncations.
- `BlockCornerIndexStmt` (`codegen_llvm.cpp:2356-2381`) derives its type from
  the struct at `:2370` and `:2374` and follows a widened `PhysicalCoordinates`
  automatically; `LoopIndexStmt` does not and must be edited.
- `create_bit_ptr`'s assertion at `codegen_llvm.cpp:1755` will fire and must be
  addressed deliberately. The bit offset is a bit position within a bit_struct's
  physical type, so whether it needs widening at all is a separate question I
  am not resolving.
- Every runtime signature listed in note 4 of my notes must widen in the same
  change. **[V]:** `check_func_call_signature`
  (`llvm/llvm_codegen_utils.cpp:103-145`, declared at `llvm_codegen_utils.h:52`,
  reached from `:110`, `:212` and `codegen_llvm.cpp:1715`) hard-errors on any
  parameter type mismatch. So a partial widening fails loudly at compile time
  naming the parameter, rather than truncating silently. This is a genuine
  safety property of the LLVM path.
- The signed comparisons (`ICMP_SLT`/`ICMP_SGE` at `codegen_llvm.cpp:1188,
  1192, 2214`, `cpu/codegen_cpu.cpp:111-112`, `cuda/codegen_cuda.cpp:533-535`,
  `dx12/codegen_dx12.cpp:96-98`) are a separate decision from width: a 64-bit
  signed loop counter is not the same as a 64-bit unsigned one.

### SPIR-V path — **[I]**, built on the **[V]** inventory in 3.2

- Two switches exist: `use_64bit_pointers` (`spirv_codegen.cpp:82`) and
  `spirv_has_physical_storage_buffer` (`vulkan_device_creator.cpp:826`,
  currently behind `&& false` at `:825`).
- **Flipping `use_64bit_pointers` alone does not merely fail to help.** `[V]`
  for each link, `[I]` for the emitted-module conclusion.
  `make_pointer`'s 64-bit branch (`:2318-2320`) returns a value typed
  `u64_type()`. `at_buffer` branches on `ptr_val.stype.dt == PrimitiveType::u64`
  at `:2197`, and `declare_primitive_type` sets `t.dt = dt` at
  `spirv_ir_builder.cpp:1536`. So on a device that has Int64 but no physical
  storage buffers — which is every device today, since the setter is compiled
  out — every SNode access routes into the `OpConvertUToPtr` /
  `spv::StorageClassPhysicalStorageBuffer` branch at `:2198-2204`, while
  `OpCapability PhysicalStorageBufferAddresses` is emitted only under
  `caps_->get(cap::spirv_has_physical_storage_buffer)` at
  `spirv_ir_builder.cpp:73-77` and the addressing model only at `:113-122`.
  The module would use a capability its own header never declares.
- **And on a device without Int64, `make_pointer`'s 64-bit branch calls the
  bare `u64_type()` at `:2320`** — the same unguarded accessor as section 4.3 —
  producing type id 0. So `use_64bit_pointers` is coupled to two capabilities
  and to neither by any code.
- `make_pointer`'s own comment (`spirv_codegen.cpp:2319`) says the 64-bit
  branch was never finished. That branch is the entry point for all four SNode
  address constants (`:355, 371, 408, 506`).
- `bitmasked_activation` breaks under widening **in two directions**, and
  **revision 4 counts the sites in each: one in the first direction and three
  in the second, four broken instructions in all on the `SNodeLookupStmt` path
  and two on the `SNodeOpStmt` path.** Revision 2 had one site, revision 3 had
  two, and revision 3 named two further sites as broken that are not.

  **The rule these are judged against, and where it is read from.** Revisions 2
  and 3 argued from what SPIR-V requires. Revision 4 argues from the validator
  checked out in this tree, `external/SPIRV-Tools/source/val/validate_bitwise.cpp`,
  because that is a fact about this repository rather than about a document:
  - `OpShiftLeftLogical` / `OpShiftRightLogical` / `OpShiftRightArithmetic`
    (`:65-104`): **Base** is tested for integer type (`:77-81`), dimension
    (`:83-86`) and **bit width against Result Type** (`:88-91`). The **Shift**
    operand is tested for integer type (`:93-97`) and dimension (`:99-102`) and
    **never for width**.
  - `OpBitwiseAnd` and its siblings (`:106-141`): the loop at `:118-119` walks
    **every** operand from index 2 and tests each for type (`:121-126`),
    dimension (`:128-132`) and **bit width against Result Type** (`:134-138`).
  - `OpAccessChain` (`external/SPIRV-Tools/source/val/validate_memory.cpp`,
    `ValidateAccessChain` at `:1264`): the per-index test at `:1342-1348`
    rejects only a non-`OpTypeInt` index type. **Width is never consulted.**
  **[V]** for all three, printed from the submodule.

  - **Direction one, a hardcoded u32 Result Type over a Base that follows the
    pointer. One site, `:411`.** At `:410-412` the `OpShiftRightLogical` is
    given Result Type `u32_type()` at `:411` while its Base,
    `bitmask_word_ptr`, is `ir_->add(parent_ptr, ...)` at `:409` and therefore
    carries the pointer width. Base-versus-Result width is
    `validate_bitwise.cpp:88-91`, so a widened pointer makes this invalid
    rather than truncated.
  - **Two of the four hardcodes revision 3 named do not break, and revision 4
    withdraws both.** This is the fault `adversary3-02-2.md` §7 found, and it
    is right.
    - **`:412` does not break.** It is the **Shift** operand of the very
      instruction whose Result Type is at `:411`
      (`make_value(op, out_type, args...)`, `spirv_ir_builder.h:290-291`, takes
      argument two as Result Type, three as Base, four as Shift). The validator
      tests the Shift operand for type and dimension and never for width
      (`validate_bitwise.cpp:93-102`). **That is the exact exemption this report
      already grants `:405`**, which is the Shift operand of the `ptr_dt`-typed
      `OpShiftLeftLogical` at `:404`. Revision 3 applied the exemption to `:405`
      and withheld it from `:412` in the same sentence.
    - **`:414` does not break, and widening it would be a regression.**
      `ir_->struct_array_access(ir_->u32_type(), buffer, bitmask_word_ptr)`.
      In `IRBuilder::struct_array_access` (`spirv_ir_builder.cpp:770-790`) the
      first parameter is `res_type`, asserted `TypeKind::kPrimitive` at `:774`,
      turned into `ptr_type = get_pointer_type(res_type, storage_class)` at
      `:783` and emitted as the **pointee type** of the `OpAccessChain` at
      `:785-787`. It is the element type of the buffer being indexed, and that
      buffer is `get_buffer_value(BufferInfo(BufferType::Root, root_id),
      PrimitiveType::u32)` at `:401-402` — a genuine array of 32-bit bitmask
      words. The **index** operand is `bitmask_word_ptr`, and `OpAccessChain`
      places no width constraint on indexes
      (`validate_memory.cpp:1342-1348`). So `:414` is correct as written under
      any pointer width, and replacing its `u32_type()` with a widened pointer
      type would retype the buffer's elements. **[V]**
  - **Direction two, a Result Type that follows the pointer over an operand
    that is permanently 32-bit. Three sites, from two distinct origins.**
    - **`:398-399`, a builder constant. Found in revision 3.**
      ```
      398    auto bitmask_mask = ir_->make_value(spv::OpShiftLeftLogical, ptr_dt,
      399                                        ir_->const_i32_one_, bitmask_bit_index);
      ```
      Result Type is **`ptr_dt`**, which is `parent_ptr.stype` at `:388` and
      follows the pointer; Base is **`const_i32_one_`**, which is
      `int_immediate_number(t_int32_, 1)` at `spirv_ir_builder.cpp:224` and is
      permanently 32-bit. Today `ptr_dt` is u32 and the widths agree. Widen
      `make_pointer` and this is a 64-bit Result Type over a 32-bit Base,
      invalid at `validate_bitwise.cpp:88-91`. Adversary2 02-1 §3.2 found it;
      adversary2 02-2 §10.2 confirmed it.
    - **`:392-394` and `:395-397`, an uncast caller-supplied index. New in
      revision 4, from a second mechanism, and absent from both reports and
      every prior adversary.** `adversary3-02-2.md` §8 found it.
      ```
      392    auto bitmask_word_index =
      393        ir_->make_value(spv::OpShiftRightLogical, ptr_dt, input_index,
      394                        ir_->uint_immediate_number(ptr_dt, 5));
      395    auto bitmask_bit_index =
      396        ir_->make_value(spv::OpBitwiseAnd, ptr_dt, input_index,
      397                        ir_->uint_immediate_number(ptr_dt, 31));
      ```
      Both Result Types are `ptr_dt` and both immediates are built at `ptr_dt`,
      so all three of those follow the pointer. The remaining operand,
      `input_index`, does not. At `:392-394` it is **Base** of a shift, tested
      at `validate_bitwise.cpp:88-91`; at `:395-397` it is an operand of
      `OpBitwiseAnd`, tested at `:134-138`. **[V]**
  - **The second mechanism, stated as a mechanism and not as two more lines.**
    `input_index` is a **parameter** (`:387`), so whether it follows the pointer
    is decided by the caller, and the two callers differ:
    - `visit(SNodeOpStmt*)` (`:437-466`) casts first —
      `ir_->cast(parent_val.stype, ir_->query_value(stmt->val->raw_name()))` at
      `:443-444` — before the three calls at `:448-449`, `:455-456` and
      `:458-459`. On that path `input_index` follows the pointer and `:392-397`
      stay valid under widening.
    - `visit(SNodeLookupStmt*)` (`:468-511`) does **not** —
      `ir_->query_value(stmt->input_index->raw_name())` at `:488-489`, called at
      `:490-491`. That is the raw linear index, i32 because it comes from
      `LinearizeStmt` (`:532-541`, section 3.2). **The same visitor casts the
      same `stmt->input_index->raw_name()` to `parent_val.stype` at `:504-505`
      for the dense-offset branch, sixteen lines below.** So the omission is
      visible against its own neighbour. **[V]**
    Today `ptr_dt` is u32 and i32 is the same width, so the width tests pass —
    the identical reason `:398-399` is legal today.
  - **Counts, derived from the enumeration above and not stated alongside it.**
    Direction one: `:411`. Direction two: `:398-399`, `:392-394`, `:395-397`.
    On the `SNodeLookupStmt` path all four are broken under widening; on the
    `SNodeOpStmt` path `:392-394` and `:395-397` are saved by the caller's cast,
    leaving `:411` and `:398-399`, which is two. Not broken in either case:
    `:405`, `:412`, `:414`.
  - **Consequence for any fix, restated because revision 3's version was
    wrong in both directions.** A change that replaces every `u32_type()` in
    this function with the pointer type would fix `:411`, do nothing for
    `:392-399`, and **break `:414`**. And the `:392-397` half is not fixable by
    editing `bitmasked_activation` at all: it is a property of what the caller
    hands in. Which of the two — typing the parameter, or casting at
    `:488-489` — is right is a design decision and is escalation 21.
  - **Grade, per standing instruction §10.8: ARCHITECTURAL.** If every driver,
    hardware generation and external specification were ideal today, this
    function would still mix a pointer-width Result Type with 32-bit operands,
    because that is a decision inside this codebase about how the bitmask index
    is represented. It does not expire. **Blast radius, stated separately:
    five places in one file** — four instructions in
    `taichi/codegen/spirv/spirv_codegen.cpp:383-435` and the one call site at
    `:488-491` where the uncast index enters. Architectural here is cheap; the
    cost of not recording it is that a repair scoped from revision 3's text
    lands on the wrong sites.
  - **And none of this would be diagnosed at compile time.** Section 4.4
    establishes that the only thing in Taichi's process that reads a produced
    module is the SPIRV-Tools **binary parser**, reached through
    `Optimizer::Run`, and that `spirv_opt_options_.set_run_validator(false)` at
    `spirv_codegen.cpp:2710` is unconditional. The parser catches structural
    faults such as a zero `<id>`. Every fault in this section is a **validator**
    check, in the file quoted at the head of this bullet, and the validator is
    the thing that is switched off. So a widened pointer would ship these
    instructions with no Taichi-level diagnostic at all — unlike the id-0 case,
    which at least produces two warnings. `adversary3-02-1.md` §2 reaches the
    same conclusion from `report-02-codegen.md` §4.2.10; I confirmed it against
    the two lines named. **[V]** for both facts, **[I]** for the joint
    consequence.
  **[V]** for the code at every line above and for the validator rules;
  **[I]** for the SPIR-V validity consequence, which follows from them.
- `LinearizeStmt`, `GlobalTemporaryStmt`, `ExternalPtrStmt` and both
  range-for/struct-for drivers all need explicit width changes.
- The struct-for listgen index (`:2164-2168`) is u32 and defines an ABI with
  the gfx runtime's list buffer, which is outside my territory.
- The `DecorationNoSignedWrap` at `:778-781` must be revisited: it is an
  overflow promise attached to an i32 offset.
- Two things cannot be widened at all: SPIR-V literal operands are 32-bit
  words (`spirv_ir_builder.h:149-159`), so `DecorationOffset` and
  `DecorationArrayStride` cap a single SSBO member offset and array stride at
  4 GiB; and `gl_GlobalInvocationID` is u32
  (`spirv_ir_builder.cpp:981-997`), so a wider global index has to be
  reconstructed rather than read.
- And the whole thing is gated on `spirv_has_int64`. A target without it can be
  *described* but the description is not honoured on the kernel-signature path
  (section 4), so on such a target the index width is not a knob — it is fixed
  at 32, and the capability boundary itself needs closing first.

### The asymmetry that matters

**[V]:** the SPIR-V path is capability-parameterised from
`spirv/kernel_compiler.cpp:37` down to every `caps_->get(...)` in the codegen.
The LLVM path takes the same `DeviceCapabilityConfig` at
`llvm/kernel_compiler.cpp:32` and drops it. Any architecture that must
"express a target that lacks Int64" has a plumbing point on one side and
nothing on the other.

---

## 7. Adjudications

Where the adversary analyses contradict each other, or contradict this report, I
opened the source and resolved it. Recorded so the planner can see the work
rather than take my word. **Entries 7.1 to 7.8 are from revision 2 and concern
`adversary-02-1.md` and `adversary-02-2.md`; entries 7.9 to 7.12 are from
revision 3 and concern `adversary2-02-1.md` and `adversary2-02-2.md`; entries
7.13 and 7.14 are from revision 4 and concern `adversary3-02-1.md` and
`adversary3-02-2.md`.** **Fourteen** entries, counted from the subheadings
below: 7.1 to 7.8 is eight, 7.9 to 7.12 is four, 7.13 to 7.14 is two, and
8 + 4 + 2 = 14. **Three** of the fourteen resolve against an adversary and
change nothing in the report: 7.1, 7.9 and 7.14.

### 7.1 `bitmasked_activation` line numbers — adversary 02-2 is right, and my original citation stands

Adversary 02-1 corrected my `:405, 411-412, 414` to `:404, :410, :411, :413`.
Adversary 02-2 said that correction is itself wrong. **Adversary 02-2 is
right.** Reading `spirv_codegen.cpp:383-435`, the literal `ir_->u32_type()`
calls inside the function body before the op branches are at `:405`, `:411`,
`:412` and `:414`. Lines `:404`, `:410` and `:413` are statement-opening lines
containing no such call — `:404` is
`ir_->make_value(spv::OpShiftLeftLogical, ptr_dt, bitmask_word_index,` and
`:413` is a bare `bitmask_word_ptr =`. Adversary 02-1 cited statement starts
while correcting a citation of the actual call sites. **No change made.**

### 7.2 `translate_ti_primitive` line numbers — adversary 02-1 is right

Adversary 02-1 gave the function as `spirv_types.cpp:167-214` with i64 at
`:179-181` and u64 at `:199-201`. Adversary 02-2 gave `:172-215`, `:184-186`
and `:203-205`. The function opens at `:167` and closes at `:214`; the i64
branch is `:179-181`; the u64 branch is `:199-201`. **Adversary 02-1's
numbers are correct**; section 4.4 uses them.

### 7.3 `node_allocators` and `ambient_elements` write sites — adversary 02-1 is right

Adversary 02-1 gave `runtime.cpp:1029` and `:1038`. Adversary 02-2 gave
`:1291-1296` and `:1298-1304`. The writes are at `:1029`
(`runtime_NodeAllocator_initialize`, `:1026-1031`) and `:1038`
(`runtime_allocate_ambient`, `:1033-1040`). Lines `:1282-1310` are
`element_listgen_root`, a different function. **Adversary 02-1's numbers are
correct**; section 5.2 uses them.

### 7.4 The span of `ir_translate_to_spirv` — both slightly off

Adversary 02-1 gave `:476-481`, adversary 02-2 gave `:476-485`. The function is
`:476-483`. Neither error is consequential; recorded for completeness.

### 7.5 `Extension::data64` — adversary 02-2's conclusion holds, its support does not

Adversary 02-2 wrote that `is_extension_supported` "is called exactly once in
the whole tree, for `Extension::assertion`". It is called at
`taichi/program/program.cpp:149`, `taichi/codegen/llvm/codegen_llvm.cpp:2726`
and six times in `taichi/transforms/compile_to_offloads.cpp` (`:92`, `:205`,
`:218`, `:236`, `:245`, `:288`), plus a Python binding at
`taichi/python/export_lang.cpp:1225`. But none of those names `data64`, and a
tree-wide grep for `data64` returns only the declaration
(`taichi/inc/extensions.inc.h:6`), the three grants
(`taichi/program/extension.cpp:12, 16, 20`) and a commented-out OpenGL grant
(`:29-30`). **The conclusion — nothing reads it — is correct**; the supporting
count is not. Section 4.4 states it the accurate way.

### 7.6 The `all_dense` computation — both adversaries slightly off, and one fact neither noted

Adversary 02-2 gave `llvm_runtime_executor.cpp:400-409`. It is `:402-410`.
Neither round-one adversary noted that `all_dense` is **seeded** from
`config_.demote_dense_struct_fors` at `:402` before the loop narrows it.
Recorded in section 5.3.

**Revision 3 note.** This entry is where the fault corrected in section 5.3 was
introduced. Getting the line range right and adding the seed as a citation
correction is what this entry did; propagating the seed into the conclusion the
paragraph draws is what it did not do. See section 7.11.

### 7.7 Where the adversaries agree and I now concur

Both resolved the Int64 contradiction against this report, independently and by
the same route. I re-derived it from the source in section 4 and they are
right. Both also found the range store at `runtime.cpp:1003-1006` that revision
1 missed; section 5.2 is built on it. Adversary 02-1's Metal, Direct3D 11 and
C API findings are confirmed at the lines given in section 4.5. Adversary
02-2's `at_buffer` coupling and `Dense_get_num_elements` findings are confirmed
in sections 4.6, 6 and 3.1.

### 7.8 Findings of mine the adversaries confirmed and neither report duplicated

Four, all re-verified this pass: `bitmasked_activation` not following a widened
`make_pointer` (`spirv_codegen.cpp:405, 411, 412, 414` — an inventory of where
`u32_type()` is hardcoded, upheld by all four adversaries; **which of the four
actually break under widening is §7.13, and it is one**); the
`DecorationNoSignedWrap` promise on an i32 offset (`:778-781`); the LLVM
`KernelCompiler` taking `device_caps` at `llvm/kernel_compiler.cpp:32` and
discarding it; and the host-side `size_t`-to-`int` narrowing at
`spirv_codegen.cpp:1999` feeding the range-for kernel.

### 7.9 What happens to an id-0 module — adversary2 02-2 is right, adversary2 02-1 is not

The two round-two adversaries give incompatible chains and this is the one the
arbitration most needed settled. I traced it end to end rather than choosing.

Adversary2 02-1 §2.1 point 3 and point 4: "no Taichi-level diagnostic" and the
failure is "deferred to the driver at shader-module creation, not absent", with
§6 item 3 asking both reports to add a sentence to that effect.

Adversary2 02-2 §2.1 and §10.3: `Optimizer::Run` calls `BuildModule` whether or
not the validator is on, the binary parser rejects id 0, the error reaches
Taichi's own consumer which downgrades it to a warning because of its branch
order, and the module ships anyway because `success` is read only inside a
compiled-out block.

**Adversary2 02-2 is right.** Section 4.4 gives the chain with every line I
opened. The decisive facts: `optimizer.cpp:596-597` calls `BuildModule`
unconditionally after the `run_validator_` test at `:590-594`;
`build_module.cpp:68-69, 74` returns null on a failed `spvBinaryParse`;
`binary.cpp:450, 456, 473` reject a zero id; and `spriv_message_consumer` at
`spirv_codegen.cpp:2646` tests `level <= SPV_MSG_FATAL`, which is level 0
(`libspirv.h:83-95`), so a level-2 `SPV_MSG_ERROR` falls to the `TI_WARN` branch
at `:2649`. So there *are* Taichi-level diagnostics and the first detection is
in-process. The sentence adversary2 02-1 asked both reports to add would have
been false.

**Where adversary2 02-1 is nonetheless right, and it is not nothing.** A zero
`<id>` operand is a hard parse failure for any conformant SPIR-V consumer —
the parser that catches it here *is* such a consumer, which is why the chain
works at all. And nothing in this tree determines what a driver does with the
module that is shipped regardless at `spirv_codegen.cpp:2775`, so the ultimate
outcome is still outside what the source can settle. What is settled is that the
first point of detection is not the driver.

**Which of the two work items 6.2 owes.** Both. A diagnostic at the type
boundary, because there is none where the type is still known; and separately
the dead `success` flag at `spirv_codegen.cpp:2742-2775`, which is a distinct
defect that exists whether or not the type boundary is closed. Neither is mine
to decide and both are in escalations.

### 7.10 The `:2464` / `:2511` swap — adversary2 02-1 found it, and it was mine to catch

Adversary2 02-1 §3.1 is right and I verified it independently by extracting the
function openings, closings and the three `ir_translate_to_spirv` calls from
`spirv_codegen.cpp:2320-2530`. Adversary2 02-2 §10.2 confirms it and credits
them. Both reports, both round-one adversaries and adversary2 02-2's own round
one file carried the swap. Section 4.4 is corrected.

The observation that matters more than the correction is theirs too: this report
printed `compile_ret_struct (:2483-...)` next to line 2464 and
`compile_argpack_struct (:2402-...)` next to line 2511. The spans that disprove
the attribution were in the same table cell as the attribution. That is not a
citation fault — both spans are right — it is a failure to read two verified
citations against each other, and standing instruction §10.7 asks for exactly
that kind of reconciliation.

### 7.11 The sparsity conclusion — both round-two adversaries are right, and against me

Adversary2 02-1 §2.6 and adversary2 02-2 §5.3 both hold that section 5.3's
conclusion is contradicted by the seeding this report itself records. I opened
`llvm_runtime_executor.cpp:402-410`, `compile_config.cpp:18` and `:72-74`,
`compile_config.h:28` and `runtime.cpp:1000-1007`. **They are right.** Section
5.3 is corrected and the withdrawn sentence is quoted there so the change is
visible.

Adversary2 02-2 §5.3 additionally holds that `report-02-codegen.md` states the
seeding correctly and then draws the same overstated conclusion eight lines
later, so the correction is owed by both reports and adversary2 02-1's §6 item 6
is wrong to ask it of this one alone. I opened the cited lines of that report and
the reading is right, but that report is not mine to amend and I record it only
as an adjudication.

### 7.12 The bitmasked line numbers, revisited — 7.1 stands, and it was incomplete

Section 7.1 adjudicated that `:405, 411, 412, 414` are the `u32_type()` call
sites and adversary 02-1's round-one `:404, :410, :413` were statement starts.
**Both round-two adversaries confirm that adjudication** (adversary2 02-1 §1.3
records its own error plainly; adversary2 02-2 §3.1 re-derives the twelve
`u32_type()` lines in `:383-435` by extraction). So 7.1 stands unchanged.

What 7.1 did not do was ask what the *other* instructions in the function do
under widening, and `:398-399` is the answer. Section 6 now carries it. Being
right about which lines a citation names is not the same as being right about
what the function does, and this is a second instance of standing instruction
§10.6 in the same report.

Adversary2 02-2 §3.1 makes one further refinement I checked and accept: of the
four hardcodes, `:405` is the **Shift** operand of an `OpShiftLeftLogical` whose
Result Type is `ptr_dt` (`:404`), and SPIR-V does not require the Shift operand
to match Base's width. So `:405` does not break.

**Revision 4 withdraws the rest of that sentence.** It read: "So `:411`, `:412`
and `:414` break under widening and `:405` does not." **Only `:411` breaks.**
The exemption this entry accepted for `:405` applies verbatim to `:412`, which
is the Shift operand of the instruction whose Result Type is `:411`; and `:414`
is a pointee type, not an index. Section 7.13 gives the adjudication and
section 6 the verification. This entry accepted a refinement of a list and did
not apply the refinement's own rule to the rest of the list — a third instance
of standing instruction §10.7 in this report, and the same shape as §7.11.

### 7.13 The widening enumeration — adversary3 02-2 is right, against me, and adversary3 02-1 did not test it

The two round-three adversaries **split**. `adversary3-02-1.md` §10 states it
"found nothing substantive that is wrong in either report" and lists four
residuals, all citation- or accounting-level. `adversary3-02-2.md` §7 states
that this report's widening enumeration is substantively wrong and §8 that both
reports are two sites short in the same function.

**Adversary3 02-2 is right on both, and I verified each against the validator
source in this tree rather than against the specification**, which is what
makes the difference: the specification sentence "the result type must match
Base" is what revision 3 reasoned from, and it does not by itself say what the
Shift operand or an `OpAccessChain` index must be.

- **§7, `:412` and `:414`.** Upheld. `validate_bitwise.cpp:93-102` tests the
  Shift operand for type and dimension only, never width; `:88-91` is the width
  test and applies to Base alone. `validate_memory.cpp:1342-1348` tests an
  `OpAccessChain` index for integer type only. And `struct_array_access`'s
  first parameter is the **pointee** type of the emitted access chain
  (`spirv_ir_builder.cpp:774, 783, 785-787`) over a buffer requested as
  `PrimitiveType::u32` at `spirv_codegen.cpp:401-402`. So `:414` is not merely
  benign, it is load-bearing at 32 bits: **widening it would be a regression.**
  Acting on revision 3's sentence would have produced three edits where one is
  needed and one of the three would have retyped the bitmask buffer's elements.
- **§8, `:392-394` and `:395-397`.** Upheld, including the mechanism. The
  operand that fails to follow the pointer is the `input_index` **parameter**
  (`spirv_codegen.cpp:387`), and whether it follows is decided by the caller.
  `visit(SNodeOpStmt*)` casts at `:443-444`; `visit(SNodeLookupStmt*)` does not
  at `:488-489`, while casting the same value at `:504-505`. I opened both
  visitors in full (`:437-466`, `:468-511`) rather than the two cited lines,
  because the claim is about a difference between two call paths and a citation
  cannot test that.
- **Why no earlier pass caught either.** Every round asked which lines the
  citation `:405, 411, 412, 414` names, and each round answered that question
  correctly — §7.1 in revision 2, §7.12 in revision 3, both adversaries in
  round two. Nobody asked what rule decides whether a named line breaks, and
  nobody read the enclosing function's *other* instructions until adversary2
  02-1 found `:398-399` and adversary3 02-2 found the parameter. The plan's
  section 10 priority statement names exactly this move, and the file that had
  not been opened is `validate_bitwise.cpp`.

### 7.14 Two places where the round-three adversaries are wrong, and nothing is changed

Recorded because the brief asks for it, and because neither error affects the
findings above, which I upheld in full.

1. **`adversary3-02-2.md` §7 puts the blast radius of the widening fault at
   "report B's escalation 10 and its section 6".** Section 6 is right;
   **escalation 10 is not.** Escalation 10 of this report is the tinyir
   capability hole at `spirv_types.cpp:397-428` and says nothing about
   `bitmasked_activation`. The escalation a reader would scope the SPIR-V
   pointer-width work from is **escalation 3**, `use_64bit_pointers`, which is
   what section 6's bitmasked material feeds. **No change made** beyond adding
   escalation 21 for the new mechanism.
2. **`adversary3-02-2.md` §12 item 3 describes the fault as "one sentence
   repeated at four places (`:385`, `:1099`, `:1330`, and by reference at
   `:237` and `:1315`)".** That names five places and calls them four, and two
   of the five do not carry the faulty claim: `:237` (section 2) and `:1315`
   (§7.12's opening) are inventories of which lines hold a `u32_type()` call,
   which is true and which all four adversarial rounds have upheld. The faulty
   claim stood at **three** places, `:385`, `:1099` and `:1330`, and all three
   are corrected. I extended section 2's bullet anyway, because a reader scoping
   6.2 from that list needs the narrower target, but that is an addition and not
   a correction. **Nothing was withdrawn at `:237` or `:1315`.**
3. **`adversary3-02-2.md` §8.2 says the cast at `:504-505` sits "eighteen lines
   below" the uncast read at `:488-489`.** It is **sixteen**. The finding is
   unaffected and the two line numbers it rests on are exact. **No change
   made** other than writing sixteen in section 6.

---

## 8. Escalations

Items I hit that need judgement beyond my assignment. I stopped each thread
rather than resolving it. Items 1 to 9 are carried from revision 1, re-verified
where their citations moved; items 10 to 14 are from revision 2; items 15 to 20
are new in revision 3; item 21 is new in revision 4. **Twenty-one items, counted
from the numbered list below: 1-9 is nine, 10-14 is five, 15-20 is six, 21 is
one, and 9 + 5 + 6 + 1 = 21.**

1. **Which quantity should the SNode ceiling bound?** Section 5 shows the
   assertion counts per-tree SNodes while the arrays are indexed by a global
   monotonic id, and that exceeding the bound is an out-of-bounds store rather
   than a bad lookup. Item 8.1.2 reserves the replacement value; the *quantity*
   is equally unsettled, and it interacts with territory 03's arrays and with
   `kMaxNumSnodeTreesLlvm = 512` (`taichi/inc/constants.h:13`).

2. **`spirv_has_physical_storage_buffer` is dead code behind `&& false`**
   (`vulkan_device_creator.cpp:825`). The upstream comments cite
   taichi-dev/taichi issue 6295 and "until device capability is ready". Whether
   to revive it, replace it, or route 64-bit SPIR-V addressing some other way
   is a design decision. I did not evaluate it and I am not proposing removal —
   the standing instructions forbid that.

3. **`use_64bit_pointers` is a hardcoded `const bool` whose 64-bit branch its
   own comment describes as unfinished** (`spirv_codegen.cpp:82`, `:2319`).
   Section 6 now establishes what flipping it emits. Whether the branch is
   correct as written is still not something I could establish by reading,
   because nothing exercises it.

4. **`generate_struct_for_kernel` in the SPIR-V path registers `"ii"`
   (`spirv_codegen.cpp:2171`) but `visit(LoopIndexStmt)` (`:543-563`) reads
   `"ii"` only in the `range_for` branch (`:547-551`) and `TI_NOT_IMPLEMENTED`s
   otherwise (`:552-554`).** I established that `demote_dense_struct_fors` is
   forced true for SPIR-V archs (`taichi/program/compile_config.cpp:72-74`,
   pass applied at `taichi/transforms/compile_to_offloads.cpp:191-196`), which
   explains dense struct-fors. Whether a non-dense struct-for can reach the
   SPIR-V codegen, and if so how its body reads the index, is outside my
   territory.

5. **The LLVM `KernelCompiler` ignores `device_caps`**
   (`llvm/kernel_compiler.cpp:32`). Whether that asymmetry with the SPIR-V path
   is intended (LLVM targets being selected at build time by `TI_WITH_*`, and
   CPU/CUDA/AMDGPU not lacking integer widths) or is a gap that section 6.3
   must close, is a design judgement.

6. **Signed versus unsigned loop counters.** Every loop comparison in both
   paths is signed except the SPIR-V struct-for, which is `OpULessThan` at
   `spirv_codegen.cpp:2157`. Widening to 64 bits and choosing signedness are
   separate decisions and I did not assume one.

7. **`create_bit_ptr`'s `isIntegerTy(32)` assertion**
   (`codegen_llvm.cpp:1755`). The bit offset is a bit position within a
   bit_struct's physical type, not an address, so it may not need widening at
   all. But it will collide with any change that widens offsets generically. I
   did not decide which.

8. **Overflow in host `int` arithmetic at `struct_llvm.cpp:174-175`.**
   `extractors[i].acc_shape * extractors[i].shape` is computed in C++ `int`
   before reaching LLVM, and the product becomes the divisor of the `CreateURem`
   at `:179`. `AxisExtractor`'s fields are `int` (`taichi/ir/snode.h:41,45,49`),
   which is territory 01. Recording the codegen-side consumption; the field
   types are not mine to change.

9. **Offline cache versioning.** `CompiledKernelDataFile`
   (`taichi/codegen/compiled_kernel_data.h:26-30`,
   `compiled_kernel_data.cpp:82-94`) carries a content hash and no explicit
   width or ABI version. Whether a width change needs an explicit cache
   invalidation marker is not mine to decide.

10. **What closes the tinyir capability hole** (section 4.3). The eight bare
    accessor calls at `spirv_types.cpp:397-428` sit in the type translator, not
    the codegen proper. Closing them could mean threading a capability
    parameter into `Translate2Spirv`, checking at the `translate_ti_type` call
    sites (`spirv_codegen.cpp:2344`, `:2358`, `:2420`, `:2436`, `:2494`), or
    rejecting at the front of the pipeline. Which is right depends on where 6.2
    decides the capability boundary sits. Not mine, and it is a change to the
    SPIR-V type layer rather than to codegen.

11. **Whether `Extension::data64` is meant to be live.** It is declared
    (`taichi/inc/extensions.inc.h:6`), granted per-arch
    (`taichi/program/extension.cpp:12, 16, 20`), and read by nothing
    (section 4.4). Whether it is a vestigial mechanism to revive or a dead one
    to route around is a decision, and the standing instructions forbid me
    proposing removal.

12. **Whether the C API OpenGL capability override is intended.**
    `c_api/src/taichi_opengl_impl.cpp:9-12` replaces the whole capability
    config, discarding the `is_gles()` guard at `opengl_device.cpp:509`. I did
    not determine whether any supported configuration reaches it. It is outside
    territory 02 and outside `taichi/`.

13. **Whether Direct3D 11 is a target this project cares about.**
    `taichi/rhi/dx/dx_device.cpp:563-565` sets only `spirv_version`. If DX11 is
    out of scope the Int64-lacking target set shrinks; if it is in scope it is
    a third case the stub architecture must express, alongside GLES and
    pre-Apple3 Metal. The brief's section 5.2 tiers are stated in NVIDIA
    generations and do not settle it.

14. **`taichi_global_tmp_buffer_size = 1024 * 1024`**
    (`taichi/inc/constants.h:15`) is what currently makes the SPIR-V i32
    `GlobalTemporaryStmt` offset safe, enforced by the assertion at
    `taichi/transforms/offload.cpp:358` (section 3.2). If that constant is also
    an installation factor under 6.1, the divergence at `spirv_codegen.cpp:708`
    becomes live. Nobody has said whether it is.

15. **Whether `spirv_opt_options_.set_run_validator(false)` is deliberate.**
    `spirv_codegen.cpp:2710`, unconditional, no comment, no config knob
    (section 4.4). Turning it on would catch id-0 modules at the point of
    production rather than as a downgraded warning from the optimiser's parser.
    It would also cost validation time on every kernel compile, which runs
    against the brief's 6.4 dispatch-throughput criterion. I am not proposing it
    be flipped; I record only what the source does today.

16. **Whether `success` at `spirv_codegen.cpp:2742-2750` is meant to gate
    anything.** It is initialised true at `:2742`, set false on optimiser
    failure at `:2750`, and read only at `:2760` inside the
    `if constexpr (false)` block at `:2758-2772`. The module is pushed at
    `:2775` either way. Whether the intent was to fall back to
    `task_res.spirv_code`, to fail the compile, or neither, is not determinable
    from the source, and the standing instructions forbid me proposing a change.
    This is a defect independent of the type-boundary hole and would survive
    closing it.

17. **Whether Metal's absent `spirv_has_float64` is a hardware constraint or an
    unwired capability.** `taichi/rhi/metal/metal_device.mm` sets int8, int16
    and float16 unconditionally at `:1046-1048` and Int64 under a family test at
    `:1051-1052`, and contains no occurrence of `float64` anywhere
    (section 4.5.1). This decides whether 6.2's stub architecture must express a
    permanently Float64-less backend or a temporarily unwired one. Not
    determinable from this tree.

18. **Whether `ti_set_runtime_capabilities_ext` is intended to be unchecked.**
    `c_api/src/taichi_core_impl.cpp:317-334` installs a caller-supplied
    capability set with no device query and no enum validation, through the
    whole-object `set_caps` at `taichi/rhi/public_device.h:855-857`, so it can
    clear detected capabilities as well as assert absent ones. It is the general
    case of which escalation 12's OpenGL constructor override is one hardcoded
    instance. Outside `taichi/` and outside my territory. Both round-two
    adversaries raised it.

19. **Whether the reachable set for the out-of-bounds store includes all-dense
    trees.** Section 5.3. It does when `config_.demote_dense_struct_fors` is
    false (`llvm_runtime_executor.cpp:402`), which is a one-flag change from a
    field defaulted true at `taichi/program/compile_config.cpp:18` and declared
    at `compile_config.h:28`. Whether this project will ever set it false is a
    configuration decision nobody has made, and it changes how urgent the
    correctness half of 6.1 is.

20. **Whether imported Vulkan is in scope, alongside Direct3D 11.**
    `VulkanRuntimeImported::Workaround::Workaround`
    (`c_api/src/taichi_vulkan_impl.cpp:19-56`) sets only `spirv_version` at
    `:37-43`, exactly as Direct3D 11 does at `dx_device.cpp:563-565`, so both
    present as lacking all five optional scalar-type capabilities rather than
    Int64 alone (section 4.5.1). Escalation 13 put Direct3D 11 to the planner;
    this is the second case and it was absent from revision 2. The brief's
    section 5.2 tiers are stated in NVIDIA generations and, by that section's own
    text, incidentally so — which means the tier table does not settle which
    SPIR-V consumers the portable path must express, and under section 2.2 the
    answer cannot be "the ones our test cards use".

21. **Where the uncast index into `bitmasked_activation` should be fixed.**
    New in revision 4, section 6. The function's `input_index` parameter
    (`spirv_codegen.cpp:387`) is used as a Base and as a bitwise operand under
    Result Types that follow the pointer (`:392-394`, `:395-397`), and one of
    its two callers hands it in uncast (`:488-489`) while the other casts
    (`:443-444`) and while the same visitor casts the same value sixteen lines
    below (`:504-505`). The fix is either to type the parameter, so the function
    states its own requirement, or to add the cast at the call site, matching
    its neighbour. **These are not equivalent:** typing the parameter makes the
    requirement checkable at every future call site; casting at `:488-489`
    fixes the one path that exists today and leaves the next caller free to
    repeat it. That is a decision about the SPIR-V value model, which section
    8.2 item 0 of the plan already records as where the pointer-width collision
    lives, and the standing instructions forbid me resolving it. It is coupled
    to escalation 3: it only becomes live if `use_64bit_pointers` is ever
    flipped, and it is one of the reasons flipping it is not a one-line change.
    `adversary3-02-2.md` §12 item 1 escalates the same question.

---

## 9. Territory sweep list

**New in revision 3.** Revision 2's header line read "Territory:
`taichi/codegen/` in full, 51 files" and then named 21 of them. A completeness
claim with no list is not falsifiable, and adversary2 02-2 §6.3 is right that
this was the real difference between this report and the paired one. The sweep
was done — notes §10 records part of it — but the report did not carry it. It
does now.

**The territory, counted mechanically.**
`find taichi/codegen -type f` returns **51** files. Of those, **45** are `.h` or
`.cpp` and **6** are `CMakeLists.txt`. Both numbers derived from the same `find`,
not stated alongside it.

### 9.1 The 21 files this report cites as findings

Each appears in section 2, 3, 4, 5 or 6 with at least one line number.

`codegen.cpp`, `codegen_utils.h`, `compiled_kernel_data.cpp`,
`compiled_kernel_data.h`, `amdgpu/codegen_amdgpu.cpp`, `cpu/codegen_cpu.cpp`,
`cuda/codegen_cuda.cpp`, `dx12/codegen_dx12.cpp`, `dx12/dx12_lower_intrinsic.cpp`,
`llvm/codegen_llvm.cpp`, `llvm/kernel_compiler.cpp`,
`llvm/llvm_codegen_utils.cpp`, `llvm/llvm_codegen_utils.h`,
`llvm/struct_llvm.cpp`, `spirv/kernel_compiler.cpp`, `spirv/kernel_utils.h`,
`spirv/snode_struct_compiler.h`, `spirv/spirv_codegen.cpp`,
`spirv/spirv_ir_builder.cpp`, `spirv/spirv_ir_builder.h`,
`spirv/spirv_types.cpp`.

Twenty-one names, counted from that list.

### 9.2 The 24 remaining files, opened and what is in them

All **[V]**. Where a file has nothing bearing on index width, address width or
SNode count, that is the finding and it is stated as such.

| File | Content bearing on 6.1 / 6.2 / 6.3 |
|---|---|
| `codegen.h` | Declarations and the codegen design comment. No width or count content. |
| `kernel_compiler.h` | Abstract `compile` interface. No width or count content. |
| `amdgpu/codegen_amdgpu.h` | Declarations only. |
| `cpu/codegen_cpu.h` | Declarations only. |
| `cuda/codegen_cuda.h` | Declarations only. |
| `dx12/codegen_dx12.h` | Declarations plus `std::size_t num_snode_trees{0};` at `:24` — a `size_t` tree count, so not a narrowing. |
| `dx12/dx12_llvm_passes.h` | Pass declarations. No width or count content. |
| `dx12/dx12_global_optimize_module.cpp` | LLVM pass pipeline setup for DXIL. No index, address or SNode-count content. |
| `dx12/dx12_lower_runtime_context.cpp` | Rewrites runtime-context accesses. No index, address or SNode-count content. |
| `llvm/codegen_llvm.h` | Declarations for `codegen_llvm.cpp`. The only width token is `uint64 mask` at `:279`, a bit mask, not an index. |
| `llvm/codegen_llvm_quant.cpp` | **55 lines carrying `get_constant` / `getInt32Ty` / `Int32Ty`.** Every one is bit manipulation inside a quantised float or integer encode/decode — exponent and digit fields of an f32, `1 << 23` style masks, `max_i32`/`min_u32` clamps. Its only index-side contact is through `load_bit_ptr` (`:23`, `:34`, `:74`), which inherits the i32 bit-offset assertion at `codegen_llvm.cpp:1755` already recorded in section 3.1. **Not a scaling limit.** Recorded because adversary2 02-2 §6.3 item 5 asked for it in the report rather than only in notes §10. |
| `llvm/compiled_kernel_data.cpp` | Serialisation plumbing. No width or count content. |
| `llvm/compiled_kernel_data.h` | `size_t ret_size{0}` at `:21` and `size_t args_size{0}` at `:24`. Already 64-bit. |
| `llvm/kernel_compiler.h` | Declaration of the compiler whose `device_caps` parameter section 2 records as dead. No further content. |
| `llvm/llvm_compiled_data.h` | Module and task-name plumbing. No width or count content. |
| `llvm/struct_llvm.h` | Declarations. `uint32 index` at `:37` is a child index within a struct type, not an SNode id or a linear index. |
| `spirv/compiled_kernel_data.cpp` | Serialisation plumbing. No width or count content. |
| `spirv/compiled_kernel_data.h` | `using TaskCode = std::vector<uint32_t>` at `:13` — SPIR-V words, fixed by the format — and `std::size_t num_snode_trees{0}` at `:19`. |
| `spirv/kernel_compiler.h` | Declarations. The forwarding of `device_caps` recorded in section 2 happens in the `.cpp`. |
| `spirv/kernel_utils.cpp` | **One thing worth recording.** `:87-95`: the ret-attribs loop pushes a single `RetAttributes` with `ra.dtype = PrimitiveTypeID::i32;` at `:93`, described by its own comment at `:87-90` as a placeholder kept only so `GfxRuntime::device_to_host::require_sync` works, and marked redundant pending removal. It is not an index width, but it is an i32 asserted about a return whose real type is elsewhere. Not mine to change. |
| `spirv/lib_tiny_ir.h` | **A positive result.** The whole layout interface is `size_t`: `memory_size` at `:190`, `memory_alignment_size` at `:191`, `nth_element_offset` at `:196`, `get_constant_shape` at `:209`, and the three caches at `:127-130` with `register_size` `:134`, `register_alignment` `:139`, `register_elem_offset` `:151`, `query_size` `:157`, `query_alignment` `:165`, `query_elem_offset` `:173`. The tinyir layout computation is 64-bit clean; the narrowing happens later, where `size_t` values reach the 32-bit SPIR-V literal operands recorded in section 3.2 at `spirv_ir_builder.h:149-159`. |
| `spirv/snode_struct_compiler.cpp` | `compute_snode_size` accumulates in `std::size_t`, as section 3.2 already records via the header. **The file also holds `construct` (`:34-63`), which builds `PhysicalPointerType` at `:53` for `SNodeType::pointer`.** Its only **entry** call is at `:18`, inside a `/* */` block opened at `:16` and closed at `:19`; there is a second, **recursive** call at `:44`, reachable only from `:18`. So the constructor is unreachable in the current tree. **Two corrections in revision 4**: the span was open-ended (`:34-...`) in a table row, and "its only call" was a quantifier the file falsifies. The conclusion is unchanged. `adversary3-02-1.md` §8.4 raised the span, `adversary3-02-2.md` §10 the quantifier. |
| `spirv/spirv_codegen.h` | Declarations. `std::vector<std::vector<uint32_t>> &generated_spirv` at `:32` is SPIR-V words, fixed by the format. |
| `spirv/spirv_types.h` | **`PhysicalPointerType` at `:71-91` derives from `IntType(/*num_bits=*/64, /*is_signed=*/false)` at `:75`, unconditionally — no capability gate of any kind.** It is an ungated 64-bit type in the SPIR-V type layer, which is directly on 6.2's question. Its consumers are `visit_physical_pointer_type` in the type-size visitor (`spirv_types.cpp:260`), the reducer (`:333-337`) and `Translate2Spirv` (`:433-439`, which emits `OpTypePointer` with `StorageClassPhysicalStorageBuffer`), reached through the dispatch at `:217-218`. Since its only construction site is the commented-out call above, none of that runs today. Absent from revision 2; adversary2 02-2 §6.3 item 4 asked for it. **[V]** |

Twenty-four rows, counted from the table. 21 + 24 = 45, which is the `find`
count, and the remaining 6 of the 51 are `CMakeLists.txt`.

### 9.3 What this sweep does not claim

It claims that each of the 45 files was opened and that the table says what was
found. It does **not** claim that every 32-bit site in the 21 cited files is in
section 3 — section 3 is an inventory of the sites that bear on index and
address width, and I have marked where a judgement of relevance is **[I]**.
Standing instruction §10.6 applies: opening a file certifies that it was opened.

---

## 10. Citation faults corrected in revision 3

Recorded as a ledger so the planner can see what the false method claim at the
top of revision 2 was covering, and so this pass can be audited the way it
audited the last one.

**Method.** Every `file:line` and `file:line-line` citation in this document was
extracted by script and printed from the named source. **The figure revision 3
attached here, "278 distinct file-qualified citations", is withdrawn in revision
4 as not reproducible; the header states the counting rule and the figure it
yields.** The bare `:NNNN` references were resolved to their
governing file and opened by hand. Faults found and corrected, each verified at
the line:

| # | Site | Was | Is | Found by |
|---|---|---|---|---|
| 1 | §3.1 `create_increment` | `codegen_llvm.cpp:2272` | `:2273`; `:2272` is blank | both adversaries |
| 2 | §3.1 struct-for loop test | `codegen_llvm.cpp:2213-2216` | `:2214-2216`; `:2213` is `SetInsertPoint` | adversary2 02-1 |
| 3 | §3.1 `GetChStmt` bit-struct branch | `codegen_llvm.cpp:1841-1843` | `:1834-1839`, `get_constant` at `:1838`; `:1841-1843` is the `else`/`call_struct_func` branch and holds no `get_constant` | adversary2 02-2 |
| 4 | §3.1 quant_array offset | `codegen_llvm.cpp:1823-1825` | `:1820-1824`, `get_constant` at `:1822`; `:1825` is `} else {` | this pass |
| 5 | §3.2 struct-for index variable | `spirv_codegen.cpp:2146` | `:2149`; `:2146` is a label with no `u32_type()` | adversary2 02-1 |
| 6 | §3.2 phi and signed `lt` | `spirv_codegen.cpp:2095, 2097` | `:2093`, `:2096` | both adversaries |
| 7 | §3.2 serial range-for step | `spirv_codegen.cpp:1791, 1821, 1823` | `:1791, 1823, 1825`; `:1821` is `spirv::Value next_value;` | both adversaries |
| 8 | §3.2 `total_invocs` | `spirv_codegen.cpp:2064-2070` | `:2066-2071`; `:2064` is blank | this pass |
| 9 | §3.2 `translate_ti_type` | `spirv_types.cpp:484-497` | function `:484-514`, pointer branch `:490-498`. §4.4 already said `:484-514`, so revision 2 contradicted itself | both adversaries |
| 10 | §3.2 and §4.6 `from_taichi_type` | `spirv_ir_builder.cpp:334-341` | function `:334-353`, pointer branch `:337-342` | adversary2 02-2 |
| 11 | §5.1 array subscript list | `runtime.cpp:1334-1336` | `:1334` and `:1336`; `:1335` is `int num_parent_elements = parent_list->size();` | adversary2 02-1 |
| 12 | §6 `check_func_call_signature` | `llvm_codegen_utils.cpp:103-144` | `:103-145` | adversary2 02-1 |

**Twelve citation faults**, counted from the rows of that table: rows 1-4 are in
§3.1, rows 5-10 in §3.2 — row 10 additionally in §4.6 — row 11 in §5.1 and row
12 in §6. That is 4 + 6 + 1 + 1 = 12.

By finder, counted from the last column: adversary2 02-1 identified **eight**
(rows 1, 2, 5, 6, 7, 9, 11, 12); adversary2 02-2 identified **six** (rows 1, 3,
6, 7, 9, 10); they overlap on **four** (rows 1, 6, 7, 9); and **two** (rows 4
and 8) were found by this pass and by neither. 8 + 6 - 4 + 2 = 12.

Three further defects that are not citation faults but were corrected in the
same pass, because a reader would act on each:

- **The `:2464` / `:2511` attribution swap** (§4.4). Both spans it cites are
  correct; the pairing is not.
- **The `data64` grep overstatement** (§4.4). The conclusion survives; the
  quantifier did not.
- **`SNode::reset_counter()` presented as a live reset avenue** (§5.1). It has
  no caller.

And one wrong conclusion, §5.3, corrected there and adjudicated at §7.11.

**What I did not fix, and why.** Nothing. Every fault either adversary raised
against this report was opened at the line; where the adversary was right the
correction is above and marked, and where an adversary was wrong the
adjudication is in section 7 and nothing was changed. Section 7.1 and section
7.9 are the two places I resolved against an adversary.

---

## 11. What revision 4 changed

Section 10 is revision 3's ledger and stands as written except for the
withdrawn figure noted in its method paragraph. This section is revision 4's,
kept separate so each pass can be audited against its own claims.

**Plan version worked against: `modernization/PROJECT-PLAN.md` dated
2026-09-09**, re-read in full at that stamp before finalising, per standing
instruction §10.6. Nothing in this revision contradicts it. Three of its
provisions bear directly on this pass and are applied rather than cited:
section 10's priority statement, which is what §7.13 records as the reason the
fault survived three rounds; §10.7, that a verified citation does not verify the
claim attached to it, which is the exact shape of the corrected fault; and
§10.8's grading test, applied in section 6.

### 11.1 The substantive correction

| # | Site | Was | Is | Found by |
|---|---|---|---|---|
| 1 | §3.2 table row, `spirv_codegen.cpp:405, 411, 412, 414` | "`:411`, `:412` and `:414` break under widening" | Only `:411` breaks. `:412` is a Shift operand, exempt by the same rule as `:405`; `:414` is `struct_array_access`'s pointee type over a u32 buffer and **must not** be widened | `adversary3-02-2.md` §7 |
| 2 | §6, the `bitmasked_activation` bullet | same sentence | same correction, plus the rule it is judged against, read from `external/SPIRV-Tools/source/val/validate_bitwise.cpp:88-91, 93-102, 134-138` and `validate_memory.cpp:1342-1348` | same |
| 3 | §7.12 closing paragraph | same sentence | same correction, with the adjudication moved to §7.13 | same |

**Three occurrences, counted from the rows above.** All three carried the same
sentence. Section 2's bullet at the `bitmasked_activation` entry and §7.12's
opening reference the same four line numbers as an **inventory of `u32_type()`
call sites**, which is true and is upheld by all four adversaries; they carried
no invalidity claim and nothing was withdrawn from them. Section 2's bullet is
extended with the narrower target because it is the list a reader would scope
6.2 from. §7.8's mention is likewise an inventory and gains a pointer to §7.13.

### 11.2 The addition

Two further widening-invalidity sites, `spirv_codegen.cpp:392-394` and
`:395-397`, from a second mechanism: the `input_index` **parameter** at `:387`
is used as a shift Base and as a bitwise operand under Result Types that follow
the pointer, and one of the function's two callers passes it uncast
(`:488-489`) while the other casts (`:443-444`) and while the same visitor casts
the same value at `:504-505`. **Four broken instructions on the
`SNodeLookupStmt` path, two on the `SNodeOpStmt` path.** Section 6, graded
ARCHITECTURAL with a blast radius of five places in one file. Escalation 21 is
new and carries the design question. Recorded in sections 0 (item 14), 1, 2, 6
and 8.

### 11.3 The two withdrawn spans that were still standing

| # | Site | Was | Is |
|---|---|---|---|
| 1 | §1, executive summary point 2 | `spirv_ir_builder.cpp:334-341` | `:334-353` |
| 2 | §2, SPIR-V touchpoint list | `spirv_ir_builder.cpp:334-341` and `spirv_types.cpp:484-497` | `:334-353` and `:484-514` |

Both forms were already established as wrong by section 10's ledger rows 9 and
10 in revision 3. The ledger scoped each correction to the sections where the
adversary raised it, and the same citations survived elsewhere — the second
inside a bullet list closed with a blanket "All **[V]**". `adversary3-02-1.md`
§8.3 found it. **This is the same failure shape as the corrected fault above,
and it is why the brief's instruction to check every corrected fact for other
occurrences was worth following: it is what turned up §3.2's `SNodeLookupStmt`
bullet.** That bullet said a widened `make_pointer` would need "no further
change" in that visitor, which the finding in §11.2 falsifies. It is qualified
in place rather than deleted, per §10.9's ruling that a scope decision removes
work and not evidence.

### 11.4 Two smaller corrections

- **§9.2, `snode_struct_compiler.cpp`.** The span `:34-...` is closed to
  `:34-63`, and "its only call is at `:18`" becomes "its only **entry** call",
  because a recursive call exists at `:44`. The unreachability conclusion is
  unchanged, since `:44` is reachable only from `:18`. `adversary3-02-1.md`
  §8.4 and `adversary3-02-2.md` §10.
- **The citation figure.** "278 distinct file-qualified citations" is withdrawn
  from the header and from §10's method paragraph. It is not reproducible under
  any rule I can state: both round-three adversaries independently extracted
  311 distinct `(path, start, end)` triples, 298 `(path, start)` pairs and 87
  files from revision 3, and I reproduce those three numbers exactly. The
  header now states the counting rule and gives the figure for this revision.
  **The sweep the figure was attached to is not withdrawn**: both adversaries
  confirm, by independent extraction, zero citations that fail to resolve and
  zero out-of-range lines.

### 11.5 What I did not change, and where an adversary is wrong

- **`adversary3-02-2.md` §7's blast radius names escalation 10.** It is
  escalation 3. §7.14 item 1.
- **`adversary3-02-2.md` §12 item 3 names five sites and calls them four**, two
  of which carry no faulty claim. §7.14 item 2. Nothing withdrawn at `:237` or
  `:1315`.
- **`adversary3-02-2.md` §8.2's "eighteen lines below" is sixteen.** §7.14
  item 3.
- **Upheld and untouched, per the brief:** the `bitmasked_activation` line
  numbers as originally given (`:405, 411, 412, 414`); the width and type counts
  in §4.3 ("eight call sites, eight optional types, five capability flags"); the
  eleven-link severity chain in §4.4; the sparsity withdrawal and the
  config-flag route in §5.3; the seventeen-setter census in §4.5.1; and the
  21 + 24 partition reconciling to 45 in §9. Both round-three adversaries
  verified each of these independently at source and both uphold them; I opened
  nothing in that set again and changed nothing in it.
- **`adversary3-02-1.md` §10's verdict that nothing substantive is wrong in this
  report is not upheld.** It did not test the widening enumeration against the
  validator, and §7.13 records the split and resolves it against it. Its four
  residuals are all real; one of the four is in this report and is fixed at
  §11.3.
- **Nothing outside this report and its notes was touched.** No source file, no
  other agent's file, not the plan. No removal, fix or cleanup was proposed. The
  new escalation states the two options and chooses neither.
