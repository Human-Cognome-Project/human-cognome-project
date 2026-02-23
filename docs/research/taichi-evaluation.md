# Taichi Lang Evaluation for HCP Engine Architecture

**From:** Engine Specialist
**Date:** 2026-02-17
**Status:** Documentation research complete. Hands-on testing needed for several items (flagged below).
**Scope:** Assessing Taichi as a potential engine component in a multi-engine HCP architecture.

---

## Executive Summary

Taichi is a GPU compute DSL embedded in Python that compiles to CUDA/Vulkan/CPU backends. It provides custom kernels over structured fields (arrays of scalars, vectors, matrices, structs) with automatic parallelization. It is fundamentally a **compute framework**, not a physics engine -- it gives you the tools to BUILD a physics engine, but provides no built-in physics primitives (no force evaluators, no integrators, no topology model).

**Key finding:** Taichi is a strong candidate for **custom parallel compute kernels** in HCP -- specifically for the operations that OpenMM cannot do (structural analysis, directed graph traversal, parallel table lookups). It is NOT a replacement for OpenMM's physics engine role; it is a replacement for writing raw CUDA/Vulkan compute shaders.

**Project health concern:** Development has slowed significantly since mid-2023. The company (Taichi Graphics) pivoted to a commercial GenAI product. Latest release is v1.7.4 (July 2025). The project is not abandoned but is in maintenance mode. 28k GitHub stars, 870 open issues. This is a risk factor for long-term dependency.

---

## Answers to Specific Questions

### 1. GPU Compute Primitives

Taichi exposes **custom kernels** (`@ti.kernel`) that auto-parallelize outermost for-loops across GPU threads. You write the loop body; Taichi handles thread dispatch.

What it provides:
- Parallel iteration over fields (arrays of particles/data)
- Atomic operations (`ti.atomic_add`, etc.) for thread-safe accumulation
- Thread Local Storage (TLS) for reductions (100x speedup on RTX 3090 for 8M element reduction)
- Block Local Storage (BLS) for shared memory caching
- Configurable block dimensions via `ti.loop_config(block_dim=N)`

What it does NOT provide:
- No built-in particle systems (you build them from fields + kernels)
- No built-in spatial hashing (you implement it yourself using fields)
- No built-in neighbor queries (you write the kernel)
- No built-in force computation (you write the math)
- No built-in integrators

**Assessment:** This is a programmable compute layer, not a simulation framework. Every physics operation is DIY. The advantage is total control; the cost is total responsibility.

### 2. Structural Analysis / Interrogation

**This is where Taichi differs fundamentally from OpenMM.** Because your kernels are arbitrary code over fields, you CAN write:
- Proximity queries (iterate particles, compute distances, filter)
- Neighbor enumeration (build grid, iterate neighboring cells)
- Substructure detection (pattern matching over struct fields)
- Bond graph traversal (iterate edge arrays, follow references)

OpenMM is "assembly only, blind to results" -- you build the molecule, run dynamics, extract coordinates. Taichi is the opposite: it is purely a compute tool, and you can interrogate anything you compute. There is no black box.

**Caveat:** You must implement ALL of this yourself. Taichi gives you fast parallel execution of your code, not pre-built analysis algorithms. Spatial hashing, BVH, neighbor lists -- all user-implemented. The Taichi blog has reference implementations (grid-based collision detection achieving 25 FPS with 32,768 particles on M1 MacBook) but these are examples, not library functions.

**NEEDS TESTING:** Whether custom structural analysis kernels (e.g., "find all particles with token_id X that are bonded to particles with token_id Y") can run at envelope scale (100k+ particles) with acceptable latency.

### 3. Directed Graphs / Directed Bonds

**Yes, but manually.** Taichi has no built-in graph data structure. The documented approach is:

```python
# Decompose graph into separate fields
edge_from = ti.field(dtype=ti.i32, shape=num_edges)
edge_to = ti.field(dtype=ti.i32, shape=num_edges)
edge_weight = ti.field(dtype=ti.f32, shape=num_edges)

# Or as a struct field
Edge = ti.types.struct(src=ti.i32, dst=ti.i32, bond_type=ti.i32, count=ti.i32)
edges = Edge.field(shape=(num_edges,))
```

A->B != B->A is trivially represented since edges are explicit directional records. This maps directly to the PBM bond model (token_A, token_B, count).

**No adjacency list support** -- you cannot have variable-length neighbor lists per particle. You must use flat edge arrays or pre-allocated fixed-size neighbor arrays. Dynamic SNodes exist but are limited (single axis, no children, no element deletion).

### 4. Per-Particle Parameters

**Yes.** Struct fields support arbitrary compositions:

```python
Token = ti.types.struct(
    token_id=ti.i32,        # integer ID
    category=ti.i32,        # grammatical category
    sub_cat=ti.i32,         # sub-categorization pattern
    lod_level=ti.i32,       # level of detail
    pos=ti.types.vector(3, ti.f32),  # position
    flags=ti.u32            # bitfield for custom flags
)
particles = Token.field(shape=(max_particles,))
```

Supported integer types: i8, i16, i32, i64 (signed), u8, u16, u32, u64 (unsigned). No booleans (use integer workaround). This is sufficient for token IDs and categorical data.

### 5. Lookup Table Support

**Yes, and this is a strong point.** A 2D lookup table is just a 2D field:

```python
# Force constant lookup: given category_A and category_B, get force value
force_table = ti.field(dtype=ti.f32, shape=(num_categories, num_categories))

@ti.kernel
def compute_bond_forces(edges: ti.template(), particles: ti.template(),
                        table: ti.template()):
    for i in range(num_edges):
        cat_a = particles[edges[i].src].category
        cat_b = particles[edges[i].dst].category
        force = table[cat_a, cat_b]   # O(1) parallel lookup
        # apply force...
```

This is a natural GPU operation -- N parallel table lookups execute in a single kernel launch. No serialization, no Python overhead. The 2D field maps directly to bond strength tables / categorical force constants.

**Comparison with OpenMM:** OpenMM's `Discrete2DFunction` does the same thing but is embedded in the force expression framework. Taichi's version is more flexible (arbitrary post-lookup computation) but requires you to write the force application yourself.

### 6. Concurrency -- Multiple Independent Particle Systems

**Partially.** Within a single Taichi runtime:
- You can have multiple independent field sets (one per "system")
- Kernels can operate on any field set
- But **kernel launches are sequential from Python** -- you call them one at a time

For true concurrency, options are:
- **Batch all systems into one large field** with a system_id per particle, then run one kernel over everything (GPU-friendly, requires careful indexing)
- **Multiple Python processes** each with their own Taichi runtime (heavyweight, separate GPU contexts)
- **AOT + C++ runtime** could potentially manage concurrent dispatch (but this is speculative)

**NEEDS TESTING:** Whether batching multiple envelopes into a single field with system_id partitioning gives acceptable performance and doesn't cause cross-system interference.

**Assessment:** This is weaker than what HCP needs. The "dozens of active contexts simultaneously" requirement would need the batched-field approach, which is doable but adds complexity.

### 7. Integration / API Surface

**Python:** First-class. Taichi IS a Python library (`pip install taichi`). NumPy interop via `field.to_numpy()` / `field.from_numpy()`. PyTorch interop exists.

**C/C++:** Via AOT (Ahead-of-Time) compilation. Kernels compile to SPIR-V (Vulkan) or native code. C API (`taichi_core.h`) exposes runtime, kernel dispatch, memory management. C++ header-only wrapper available.

**GDScript/Godot:** No native integration exists. Possible paths:
- AOT compile to shared library, call via GDNative/GDExtension (C API bridge)
- Vulkan interop -- Godot uses Vulkan, Taichi can export Vulkan buffers. Zero-copy buffer sharing is theoretically possible but the docs note this has been problematic ("users cannot plug in their existing GPU buffers")
- Python bridge via Godot's Python plugin (adds latency)

**NEEDS TESTING:** Whether Taichi AOT -> GDExtension is a viable pipeline. The C API exists; the question is whether the runtime overhead and data transfer costs are acceptable for real-time operation.

**Key limitation for AOT:** `ti.field` is NOT supported in AOT mode (listed as WIP). Only `ti.ndarray` works. This means the struct field approach for particles would need to be restructured as flat ndarrays for deployment outside Python.

### 8. Taichi vs Raw Vulkan/CUDA Compute Shaders

**What Taichi abstracts away:**
- Thread/block/grid management (automatic from loop structure)
- Memory allocation and layout (fields handle this)
- Backend portability (same code runs on CUDA, Vulkan, CPU)
- Kernel compilation and caching
- Data transfer boilerplate

**What Taichi limits:**
- No direct access to shared memory layout (BLS is automatic, not manual)
- No warp-level primitives (no shuffle, no ballot)
- No runtime recursion in kernels
- No dynamic memory allocation in kernels
- Vulkan backend: **dense structures only** -- no sparse SNodes, no block-shared memory, no thread-local memory
- CUDA backend: full features but locks you to NVIDIA
- Matrix sizes limited at compile time (unrolled; >16x16 causes slow compilation)
- No kernel-to-kernel calls (must return to Python between kernel launches)

**Performance:** Benchmarks on RTX 3080 show Taichi within ~5-15% of hand-written CUDA on most kernels (MPM, N-body, SAXPY). MPM was actually faster in Taichi due to automatic optimization discovery. This is near-zero abstraction cost for the common case.

### 9. Scene/Graph Management

**Minimal.** Taichi has a lightweight GUI system (GGUI, Vulkan-based) that can render particles, meshes, and lines. It has a basic `Scene` class for 3D visualization. This is a debug/demo tool, not a scene graph.

No entity management, no spatial hierarchy, no ECS, no scene graph. Taichi is purely a compute framework with a basic visualizer bolted on.

### 10. Performance Profile

Concrete numbers from documentation and benchmarks:
- **32,768 particles** with grid-based collision detection: 25 FPS on M1 MacBook
- **8M element** parallel reduction: 57 microseconds with TLS on RTX 3090 (vs 5,200 us without TLS -- 100x improvement)
- **MPM simulation:** faster than hand-written CUDA on RTX 3080
- **Roofline analysis:** Taichi kernels hit near-roofline performance for regular computations

**NEEDS TESTING:** Performance at HCP-relevant scales (100k-1M particles with struct fields, 2D table lookups, directed edge traversal). The 32k particle benchmark is for full physics simulation with collision; pure field operations should scale much higher.

---

## Field Data Structures and HCP Mappings

| HCP Concept | Taichi Mapping | Notes |
|-------------|---------------|-------|
| Token (particle) | Struct field element | Custom struct with token_id, category, sub_cat, etc. |
| Bond (A->B, count) | Struct field of edges | Flat array of (src, dst, bond_type, count) |
| Force constant table | 2D scalar field | `shape=(num_cats, num_cats)`, O(1) GPU lookup |
| PBM document | Partition within fields | system_id field or separate field set |
| Activity envelope | Set of active fields | Multiple field sets, selectively loaded |

### Sparse field applicability

Taichi's sparse SNodes (pointer, bitmasked, dynamic) are designed for spatially sparse grids -- think voxel worlds or fluid simulation where most cells are empty. They are NOT designed for graph sparsity. For bond tables where most category pairs have zero interaction, a dense 2D field with zero values is likely more GPU-efficient than sparse structures (predictable memory access patterns).

---

## Differentiable Programming and Mesh-Texture Feedback

Taichi supports reverse-mode and forward-mode automatic differentiation. In principle, this could enable:

- **Given:** a surface text configuration (texture)
- **Compute:** energy under linguistic forces (how well does this surface realize the conceptual mesh?)
- **Differentiate:** which token changes would reduce energy (improve realization)

This maps to the mesh->texture feedback loop. However:
- Autodiff requires fields with `needs_grad=True`
- Strict rules: no mutation after read, accumulative writes only
- Kernels must be split (no mixing parallel loops with serial code)
- Whether linguistic force functions are differentiable depends on whether they use continuous operations (yes for distance-based forces, problematic for discrete category lookups)

**NEEDS TESTING:** Whether the linguistic force definitions can be expressed in differentiable form. Discrete category lookups are not differentiable; continuous relaxations (soft lookups) would be needed.

---

## Assessment: Where Does Taichi Fit in Multi-Engine HCP?

### Strong fit

1. **Parallel table lookups** -- Given N particle pairs, look up force constants from a 2D table. This is Taichi's bread and butter.
2. **Custom structural analysis** -- Proximity queries, neighbor enumeration, substructure detection. OpenMM cannot do this; Taichi can.
3. **Directed bond graph operations** -- Iterate directed edges, compute properties, aggregate results. Natural in Taichi's kernel model.
4. **Per-particle data management** -- Struct fields with arbitrary typed data per particle.
5. **Prototyping speed** -- Python-native, immediate execution, fast iteration compared to raw CUDA.

### Weak fit

1. **Physics engine role** -- Taichi provides no physics. You would be writing an entire physics engine from scratch. OpenMM already has force evaluation, energy minimization, and integration.
2. **Multiple concurrent systems** -- Sequential kernel launches from Python. Batching into one field is possible but adds complexity.
3. **Godot integration** -- No native path. AOT pipeline exists but `ti.field` not supported in AOT (only ndarray). Significant engineering effort to bridge.
4. **Project health** -- Maintenance mode. Not ideal for a foundational dependency.

### Does not fit

1. **Scene/entity management** -- No capability here. Not its job.
2. **Drop-in OpenMM replacement** -- OpenMM provides a complete molecular dynamics framework; Taichi provides a compute compiler. Different layers entirely.

### Recommended role in architecture

**Taichi as the "analysis engine" complement to OpenMM as the "physics engine."**

```
PostgreSQL -> LMDB -> [Taichi: structural analysis, table lookups, bond graph queries]
                   -> [OpenMM: force evaluation, energy minimization, dynamics]
                   -> [Game engine: scene management, visualization, envelope orchestration]
```

Taichi fills the gap where OpenMM is blind: interrogating what has been built. OpenMM assembles and runs dynamics; Taichi answers "what's near what?", "which bonds match this pattern?", "what's the lookup value for these N pairs?".

### Alternative consideration

If the Godot integration path proves viable, Godot's own compute shader pipeline (Vulkan-based) could potentially replace Taichi for the analysis role. The trade-off: raw compute shaders require more boilerplate but eliminate the Taichi dependency and project health risk. Taichi's value is developer productivity (Python-native, auto-parallelization) vs. maintenance risk.

---

## Items Requiring Hands-On Testing

1. **Struct field performance at scale** -- 100k+ particles with 6-8 integer fields per particle, kernel throughput
2. **2D table lookup throughput** -- N=100k parallel lookups from a 500x500 table, latency measurement
3. **Directed edge traversal** -- Iterate 500k directed edges, aggregate per-particle, measure throughput
4. **Batched multi-system** -- 10 independent systems of 10k particles each in one field, verify no cross-contamination
5. **AOT -> C shared library** -- Compile a kernel, call from C, measure overhead
6. **Vulkan buffer interop** -- Share a buffer between Taichi and an external Vulkan application
7. **Differentiable force evaluation** -- Express a simple linguistic force in differentiable form, verify gradient computation

---

## Sources

- [Taichi Fields documentation](https://docs.taichi-lang.org/docs/field)
- [Taichi Kernels and Functions](https://docs.taichi-lang.org/docs/kernel_function)
- [Taichi Sparse Data Structures](https://docs.taichi-lang.org/docs/sparse)
- [Taichi Differentiable Programming](https://docs.taichi-lang.org/docs/differentiable_programming)
- [Taichi Performance Tuning](https://docs.taichi-lang.org/docs/performance)
- [Taichi C++ AOT Tutorial](https://docs.taichi-lang.org/docs/tutorial)
- [Taichi Core C API](https://docs.taichi-lang.org/docs/taichi_core)
- [Taichi Vulkan Backend](https://docs.taichi-lang.org/docs/taichi_vulkan)
- [GPU-Accelerated Collision Detection with Taichi (blog)](https://docs.taichi-lang.org/blog/acclerate-collision-detection-with-taichi)
- [Is Taichi comparable to CUDA? (blog/benchmark)](https://docs.taichi-lang.org/blog/is-taichi-lang-comparable-to-or-even-faster-than-cuda)
- [Taichi benchmark repository](https://github.com/taichi-dev/taichi_benchmark)
- [Taichi GitHub repository](https://github.com/taichi-dev/taichi) -- 28k stars, v1.7.4 (July 2025)
- [Development status discussion](https://github.com/taichi-dev/taichi/discussions/8506)
- [Taichi SIGGRAPH 2019 paper](https://yuanming.taichi.graphics/publication/2019-taichi/taichi-lang.pdf)
- [GGUI visualization system](https://docs.taichi-lang.org/docs/ggui)
