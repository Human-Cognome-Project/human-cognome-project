# Independent architecture review — 2026-09-09

Reviewed checkout: `ba0e81dce`. Scope: source-level suitability for compute-only hierarchical gravity and interaction analysis with many distinct interaction patterns. No engine code changed. Prior investigation documents were used as search leads, not as requirements or evidence of implementation. This review describes this checkout, not current upstream or vendor support.

**Opinion:** a focused LLVM-runtime modernization is technically justified. The strongest immediate targets are SNode identity/capacity management and sparse metadata allocation. A universal 64-bit conversion is neither a prerequisite for increasing the 1024 capacity nor a sufficient solution. The existing SPIR-V address work is partial and does not deliver an expanded SNode implementation.

Working-set requirement clarified during review: the engine holds only a small portion of the possible model data. More simultaneous granularity and scope should be available as machine resources permit. A configuration loader will probe the system and incorporate user preferences to set capacity; small edge deployments may retain a small budget. This does not imply loading the full model or requiring huge individual arrays.

The recommended interface is runtime capacity selected at initialization, backed by allocated tables and explicit validation. It should distinguish SNode slots, tree slots, sparse metadata and useful-data budgets. Hardware memory alone cannot determine a safe pattern count under the current per-node overhead; the loader needs an allocation cost model and the user's resource budget. Startup sizing avoids requiring live table relocation as a first implementation step. Probing should establish actual backend capabilities as well as available memory.

## 1. What the limits actually bound

| Resource | Implementation | Consequence |
|---|---|---|
| SNode runtime slots | `taichi_max_num_snodes = 1024` | Bounds LLVM element-list, allocator and ambient-element tables indexed by SNode ID. |
| Separately materialized LLVM trees | `kMaxNumSnodeTreesLlvm = 512` | Independent root-pointer and root-size arrays indexed by tree ID. |
| SNode IDs | Monotonic `std::atomic<int>` | Created layouts consume IDs; these are not particle counts. |
| Logical coordinates and several allocator counters | 32-bit integers | Separate overflow constraints, even where byte pointers are native-width. |

Evidence: [constants.h](/opt/project/taichi/taichi/inc/constants.h:12), [runtime tables](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:552), [ID assignment](/opt/project/taichi/taichi/ir/snode.cpp:212), [index warning](/opt/project/taichi/taichi/ir/snode.cpp:89).

The assertion in [StructCompilerLLVM::run](/opt/project/taichi/taichi/codegen/llvm/struct_llvm.cpp:266) checks the count within one tree. The runtime uses global SNode IDs. Multiple individually legal trees can therefore exceed the indexed table capacity without violating that assertion. Fully dense trees bypass list creation, so symptoms depend on the path used. This is a potential out-of-bounds correctness defect, not just an inconvenient clean rejection. It has not been dynamically reproduced in this review.

Tree destruction recycles **tree IDs**, while SNode IDs keep increasing until a new Program resets the counter. See [destroy_snode_tree](/opt/project/taichi/taichi/program/program.cpp:214), [allocate_snode_tree_id](/opt/project/taichi/taichi/program/program.cpp:559), and [Program initialization](/opt/project/taichi/taichi/program/program.cpp:144). Tree-ID allocation also has no bound check corresponding to the 512-entry root arrays at that allocation site.

Another candidate defect is the assumption that a tree occupies a contiguous SNode-ID interval. [Sparse initialization](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:1003) uses root ID plus node count, rather than the actual ID list. Independently interleaved FieldsBuilder construction deserves a reproduction test. Do not assume either this case or accumulated-ID overflow is safe because ordinary small-tree tests pass.

## 2. Small useful data does not mean small runtime storage

Every [ListManager](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:424) embeds 131,072 pointer slots. With eight-byte pointers that is **1 MiB per manager**, excluding headers, alignment and allocated data chunks.

For a non-all-dense tree, [initialization](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:1000) creates one such manager per SNode, including nodes that the source itself acknowledges may not need a list. Every [NodeManager](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:643) creates three additional list managers.

Thus roughly 1000 SNodes on that sparse path imply roughly 1 GiB of embedded list-pointer storage alone, before allocator lists and useful particle data. This is a source-derived estimate, not a measured resident-memory figure. All-dense layouts follow a materially cheaper path.

This is particularly relevant to the proposed workload: many small, distinct layouts can incur overhead designed for much larger homogeneous populations. Raising the constant does not fix it. Conversely, the three top-level SNode pointer tables themselves cost only about 24 KiB at capacity 1024 on a 64-bit target.

Both CPU and CUDA/AMDGPU use this runtime allocation machinery. Device pools introduce additional capacity and exhaustion behavior, but the per-manager cost is not uniquely a GPU issue. See [allocation paths](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:821).

## 3. The 64-bit issue is several issues

LLVM already has native pointer storage, `size_t` root sizes and byte allocation sizes. [SNode metadata](/opt/project/taichi/taichi/ir/snode.h:94) mixes a 64-bit cell count with `size_t` byte sizes and narrower axis metadata. The [LLVM structure compiler](/opt/project/taichi/taichi/codegen/llvm/struct_llvm.cpp:103) generates real pointer-bearing sparse layouts.

What remains narrow includes axis extraction, list counts, allocator indices and node element sizes. For example, a `size_t` node size passed to [runtime_NodeAllocator_initialize](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:1026) reaches a [NodeManager constructor](/opt/project/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp:643) taking `i32`. Widening a final pointer cannot repair overflow that happened earlier.

The SPIR-V side has a different problem:

- [make_pointer](/opt/project/taichi/taichi/codegen/spirv/spirv_codegen.cpp:2317) emits 32-bit SNode offsets with `use_64bit_pointers` fixed false in the codegen class.
- [at_buffer](/opt/project/taichi/taichi/codegen/spirv/spirv_codegen.cpp:2195) interprets a 64-bit pointer value as a physical device address. Widening a root-relative offset alone would therefore change its meaning without supplying the physical root base. Address kind needs deliberate representation.
- The external-array physical-address branch exists, but [flattening and byte-offset arithmetic](/opt/project/taichi/taichi/codegen/spirv/spirv_codegen.cpp:735) occur in `i32` before conversion to a 64-bit address. It is not end-to-end wide indexing.
- [SNode type construction](/opt/project/taichi/taichi/codegen/spirv/snode_struct_compiler.cpp:9) contains disconnected physical-pointer work: the calls that would use the alternative type construction are commented out.
- Vulkan's automatic physical-storage capability enablement is disabled in [device creation](/opt/project/taichi/taichi/rhi/vulkan/vulkan_device_creator.cpp:825). Local commit `d20f55dc8` from 2022-10-30 explicitly disabled previously enabled functionality temporarily pending device-capability work. This is not evidence of one complete, never-executed expansion waiting to be switched on.

Capacity, logical indices, byte offsets, physical addresses and floating-point precision should have explicit, separate requirements. A workload can need thousands of layouts without needing billions of elements in any one container.

## 4. What direct SNodes buy the model

SNodes describe compiled storage schemas. [FieldsBuilder.finalize](/opt/project/taichi/python/taichi/_snode/fields_builder.py:167) closes a layout to further field placement, and [LLVM type generation](/opt/project/taichi/taichi/codegen/llvm/struct_llvm.cpp:36) turns its children into concrete structures and arrays. Runtime sparse activation changes which instances exist within that schema; it does not make arbitrary schema rewiring a kernel operation.

Distinct patterns with stable, different field layouts can reasonably benefit from direct representation: specialized access, deliberate placement and separate aggregate storage. This supports the motivation for expanding structural variety.

However, a compiled SNode is not automatically a semantic interaction node. Neither centroid computation nor contributor provenance nor cross-group membership is inferred from storage parentage. Those remain model operations and data. The IR's [child access](/opt/project/taichi/taichi/ir/statements.h:1366) identifies concrete input/output SNodes and a child index; it is not a generic heterogeneous graph traversal interface.

My earlier conversational agreement should be qualified here: preserving distinct interaction identities aids analysis, but mapping every identity to a distinct SNode does not by itself establish faster execution or better accuracy. Stable schema variety and frequently changing interaction membership are different scaling dimensions. The final design should preserve the analytical distinctions in either case.

## 5. Backend assessment for this checkout

| Backend | Evidence and implication |
|---|---|
| LLVM CPU | Existing sparse and 64-bit-data path; suitable reference implementation for correctness and many-pattern measurements. Shares the problematic sparse metadata design. No speed superiority established. |
| CUDA | Existing sparse and 64-bit-data path plus GPU execution support. Closest implemented GPU counterpart for the hierarchy work. Measure allocation, launch and reduction costs on the intended device. |
| AMDGPU | More code exists than its advertised extension list suggests: shared sparse runtime and implemented struct-for/listgen/GC and double-precision operations. Python sparse construction is nevertheless actively blocked by the extension gate. A targeted bring-up is plausible, not demonstrated working. |
| Vulkan | Device-level precision capabilities and partial address machinery exist. They do not establish working general pointer/dynamic SNodes. Could suit dense compute workloads, but full direct sparse hierarchy requires additional work. |
| Metal | Its capability collector exposes optional integer64 but no float64 capability, and comments out native floating atomics. It requires separate numerical qualification. |
| DX12 | Codegen exists, but the LLVM program's DX12 kernel-launcher branch is explicitly unimplemented. Separate AOT code does not establish a usable normal JIT path. |

Sources: [extension declarations](/opt/project/taichi/taichi/program/extension.cpp:8), [Python sparse gate](/opt/project/taichi/python/taichi/_snode/fields_builder.py:83), [AMDGPU codegen](/opt/project/taichi/taichi/codegen/amdgpu/codegen_amdgpu.cpp:355), [SPIR-V layout compiler](/opt/project/taichi/taichi/codegen/spirv/snode_struct_compiler.cpp:65).

Additional evidence: [Metal capabilities](/opt/project/taichi/taichi/rhi/metal/metal_device.mm:1017), [DX12 launcher branch](/opt/project/taichi/taichi/runtime/program_impls/llvm/llvm_program.cpp:144).

Your warning about vendor-stack completeness is supported by the AMDGPU discrepancy. Its absent sparse label is an admission gate over substantial implementation, not proof that no implementation exists. Equally, bypassing that gate would not validate correctness.

Rendering is unnecessary to the algorithm. Build dependencies can still be coupled: [TaichiCore.cmake](/opt/project/taichi/cmake/TaichiCore.cmake:62) enables GGUI when Vulkan is enabled. Headless packaging and compute capability are separate questions.

## 6. Recommended next engineering work

1. Establish a runnable baseline at this commit. Reproduce accumulated global-ID overflow, the independent root limit, interleaved builders and repeated creation/destruction. Include all-dense and sparse layouts.
2. Replace fixed global-ID indexing with explicitly managed runtime capacity supplied by the configuration loader after system probing and user budget selection. Keep compiler identity distinct from reusable runtime slots, or use growable tables with a defined lifetime. Account for cached kernels and outstanding device work before reuse or relocation.
3. Remove unnecessary eager lists and replace the fixed million-byte chunk-pointer directories with demand-sized or paged storage. Verify destruction/reclamation. This is essential to make greater pattern counts practical.
4. Measure representative patterns at increasing counts, recording compilation time, metadata bytes, materialization time, tick latency and transfer volume. Compare direct layouts and shared schemas without discarding interaction identity in either representation.
5. Widen the specific index/offset paths required by actual bounds. Treat SPIR-V physical addresses and sparse support as their own implementation project; qualify AMDGPU through focused operations rather than blanket feature enablement.

For analytical validation, separate centroid approximation error from arithmetic and parallel-reduction behavior. This checkout defaults to [f32 and fast math](/opt/project/taichi/taichi/program/compile_config.cpp:30); a numerical baseline should explicitly choose precision and arithmetic settings. It should also define which tick's aggregate state is consumed and how near-field refinement is decided. No claim about the supplied model's accuracy can be made without its equations and acceptance criteria.

## Validation limits

This is a static code review, supported by direct source reads, symbol searches and local history inspection. There is no built Taichi Python library or CMake build cache in the checkout, `python3 -m pip show taichi` finds no installed package, and `llvm-config` is absent from PATH. Runtime defects, backend readiness, performance and numerical accuracy have therefore not been experimentally established. No dependencies were installed and no engine changes were made.

The next decision is whether to undertake a focused runtime prototype. The source supports that investment more strongly than a broad rewrite justified by a presumed universal 32-bit address ceiling.
