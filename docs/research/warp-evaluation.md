# NVIDIA Warp Evaluation for HCP PBM Particle Simulation

**Date:** 2026-02-17
**Status:** API research complete from documentation and source code. Hands-on testing needed for several items (flagged below).
**Scope:** Assessing NVIDIA Warp as the GPU compute engine for PBM-based particle simulation in HCP.
**Warp Version Reviewed:** 1.11.1 (released 2026-02-01)

---

## Executive Summary

NVIDIA Warp is a Python framework that JIT-compiles Python functions to C++/CUDA kernel code. Like Taichi, it is a **compute framework**, not a physics engine -- you define everything yourself via custom kernels. Unlike Taichi, Warp is backed by NVIDIA's active development team (latest release: 2 weeks ago), has built-in spatial data structures (HashGrid, BVH, Mesh), and uses C++/CUDA as an intermediate representation rather than LLVM/SPIRV.

**Key finding:** Warp is a stronger candidate than Taichi for HCP's particle simulation layer. It provides:
- Built-in `HashGrid` with spatial neighbor queries (Taichi requires DIY)
- `wp.struct` for custom particle types on GPU
- `wp.grad()` for computing forces from energy functions inline
- Active development with multi-world/batched simulation support (v1.11)
- Mature NumPy/PyTorch/JAX interop for data pipeline integration
- CUDA graph capture for zero-overhead simulation loops

**Key risk:** NVIDIA-only (no Vulkan/CPU backend for production GPU compute, though CPU fallback exists for development). This is acceptable if the target deployment has NVIDIA GPUs.

---

## Answers to Specific Questions

### 1. Custom Structs on GPU

**Fully supported.** The `@wp.struct` decorator creates GPU-resident composite types with zero-overhead field access.

```python
import warp as wp

@wp.struct
class Particle:
    token_id: wp.int32       # integer token ID from vocabulary
    category: wp.int32       # grammatical category (noun, verb, etc.)
    sub_cat: wp.int32        # sub-categorization pattern
    position: wp.vec3        # 3D position in simulation space
    velocity: wp.vec3        # current velocity
    mass: wp.float32         # particle mass (can vary by category)
    is_rigid: wp.int32       # 0 = free, 1 = locked into rigid group
    rigid_group_id: wp.int32 # which rigid group (-1 = none)
```

**Usage in arrays and kernels:**

```python
# Create an array of particles on GPU
num_particles = 100000
particles = wp.zeros(num_particles, dtype=Particle, device="cuda")

# Or from Python-side initialization
particle_list = []
for i in range(num_particles):
    p = Particle()
    p.token_id = token_ids[i]
    p.category = categories[i]
    p.position = wp.vec3(x[i], y[i], z[i])
    p.mass = mass_by_category[categories[i]]
    p.is_rigid = 0
    p.rigid_group_id = -1
    particle_list.append(p)
particles = wp.array(particle_list, dtype=Particle, device="cuda")

# Access in kernel
@wp.kernel
def update_particles(particles: wp.array(dtype=Particle), dt: float):
    tid = wp.tid()
    p = particles[tid]
    if p.is_rigid == 0:  # only move free particles
        p.position = p.position + p.velocity * dt
        particles[tid] = p
```

**Supported field types:** int8/16/32/64, uint8/16/32/64, float16/32/64, bool (as wp.bool), vec2/3/4, mat22/33/44, quat, transform, nested structs, and wp.array references.

**Limitations:**
- No inheritance between structs
- No methods on structs (use standalone `@wp.func` functions)
- Fixed layout once defined
- Array fields in structs are references (pointers), not embedded data -- must manage lifetime

**Comparison with Taichi:** Nearly identical capability. Taichi uses `ti.types.struct(...)` factory function; Warp uses `@wp.struct` decorator on a class. Both produce GPU-resident SOA/AOS with typed fields. Warp's decorator syntax is slightly more Pythonic.

---

### 2. Custom Force Kernels with Asymmetric Lookup

**Fully supported.** This is the core use case. Here is a complete working example of asymmetric directed forces looked up from a 2D table:

```python
import warp as wp
import numpy as np

NUM_CATEGORIES = 64  # e.g., 64 grammatical categories

@wp.struct
class Particle:
    token_id: wp.int32
    category: wp.int32
    position: wp.vec3
    velocity: wp.vec3
    mass: wp.float32
    is_rigid: wp.int32

@wp.kernel
def compute_bond_forces(
    particles: wp.array(dtype=Particle),
    bond_src: wp.array(dtype=wp.int32),      # source particle index
    bond_dst: wp.array(dtype=wp.int32),      # destination particle index
    bond_count: wp.array(dtype=wp.float32),   # bond strength (frequency)
    force_table: wp.array2d(dtype=wp.float32), # [cat_A, cat_B] -> force multiplier
    forces: wp.array(dtype=wp.vec3),          # output force accumulator
    num_bonds: int,
):
    tid = wp.tid()
    if tid >= num_bonds:
        return

    src_idx = bond_src[tid]
    dst_idx = bond_dst[tid]

    src = particles[src_idx]
    dst = particles[dst_idx]

    # Direction vector from src to dst
    delta = dst.position - src.position
    dist = wp.length(delta)
    if dist < 1.0e-6:
        return
    direction = delta / dist

    # ASYMMETRIC lookup: force_table[cat_A, cat_B] != force_table[cat_B, cat_A]
    cat_a = src.category
    cat_b = dst.category
    force_multiplier = force_table[cat_a, cat_b]

    # Bond strength scales the force
    strength = bond_count[tid]

    # Force on src particle: pulled TOWARD dst
    # (asymmetric: only src feels this specific force from this bond direction)
    force_magnitude = force_multiplier * strength
    f = direction * force_magnitude

    # Accumulate force on source particle (atomic for thread safety)
    wp.atomic_add(forces, src_idx, f)

    # Optionally apply reaction force on dst (can be different magnitude)
    reverse_multiplier = force_table[cat_b, cat_a]  # DIFFERENT lookup
    reverse_force = -direction * reverse_multiplier * strength
    wp.atomic_add(forces, dst_idx, reverse_force)

# --- Setup ---
# Build force table from PBM data (on CPU, transfer to GPU)
force_np = np.zeros((NUM_CATEGORIES, NUM_CATEGORIES), dtype=np.float32)
# ... populate from linguistic analysis ...
force_table = wp.array(force_np, dtype=wp.float32, device="cuda")  # 2D array

# Launch
wp.launch(
    kernel=compute_bond_forces,
    dim=num_bonds,
    inputs=[particles, bond_src, bond_dst, bond_count, force_table, forces, num_bonds],
    device="cuda",
)
```

**Key details:**
- `wp.array2d(dtype=wp.float32)` is shorthand for `wp.array(dtype=wp.float32, ndim=2)` -- provides 2D indexing `table[i, j]`
- `wp.atomic_add` handles race conditions when multiple bonds write to the same particle
- The asymmetry is natural: you do two separate lookups `table[cat_a, cat_b]` and `table[cat_b, cat_a]`

**Alternative using wp.grad() for force-from-energy:**

Warp v1.11 introduced `wp.grad()` which computes the gradient of a function inline during the forward pass. This is ideal for defining energy functions and deriving forces automatically:

```python
@wp.func
def bond_energy(delta: wp.vec3, strength: float, k: float):
    """Energy of a bond: spring-like potential scaled by bond count."""
    dist = wp.length(delta)
    return k * strength * dist * dist  # harmonic potential

@wp.kernel
def compute_forces_from_energy(
    particles: wp.array(dtype=Particle),
    bond_src: wp.array(dtype=wp.int32),
    bond_dst: wp.array(dtype=wp.int32),
    bond_count: wp.array(dtype=wp.float32),
    force_table: wp.array2d(dtype=wp.float32),
    forces: wp.array(dtype=wp.vec3),
):
    tid = wp.tid()
    src = particles[bond_src[tid]]
    dst = particles[bond_dst[tid]]
    delta = dst.position - src.position
    k = force_table[src.category, dst.category]
    strength = bond_count[tid]

    # wp.grad() computes d(bond_energy)/d(delta) -- the force vector
    # This is automatic differentiation inline, not a tape
    force = -wp.grad(bond_energy)(delta, strength, k)
    wp.atomic_add(forces, bond_src[tid], force)
```

**Comparison with Taichi:** Taichi requires you to write force math manually or use its tape-based autodiff (which has strict rules about field mutation). Warp's `wp.grad()` is more flexible -- it works inline without a tape, and you can mix differentiable and non-differentiable code freely.

---

### 3. HashGrid Usage for Spatial Neighbor Queries

**Built-in and battle-tested.** This is one of Warp's strongest differentiators from Taichi.

```python
# Create hash grid
grid = wp.HashGrid(dim_x=128, dim_y=128, dim_z=128, device="cuda")

# Build from particle positions (call every frame or when particles move)
grid.build(points=particle_positions, radius=cell_size)

# Query neighbors in a kernel
@wp.kernel
def find_neighbors(
    grid: wp.uint64,
    positions: wp.array(dtype=wp.vec3),
    categories: wp.array(dtype=wp.int32),
    output_energy: wp.array(dtype=wp.float32),
    radius: float,
):
    tid = wp.tid()

    # IMPORTANT: hash_grid_point_id reorders threads by cell for cache coherence
    i = wp.hash_grid_point_id(grid, tid)

    pos_i = positions[i]
    cat_i = categories[i]
    energy = float(0.0)

    # Query all neighbors within radius
    neighbors = wp.hash_grid_query(grid, pos_i, radius)

    for index in neighbors:
        if index != i:
            pos_j = positions[index]
            dist = wp.length(pos_i - pos_j)
            if dist <= radius:
                # Do something with the neighbor
                energy += 1.0 / (dist + 0.001)

    output_energy[i] = energy

# Launch
wp.launch(
    kernel=find_neighbors,
    dim=len(positions),
    inputs=[grid.id, positions, categories, output_energy, search_radius],
)
```

**Key API details:**
- `wp.HashGrid(dim_x, dim_y, dim_z)` -- creates the grid structure
- `grid.build(points, radius)` -- rebuilds from current positions. Call from Python before launching neighbor kernels.
- `wp.hash_grid_query(grid, point, radius)` -- returns an iterator over neighbor indices within radius
- `wp.hash_grid_query_next(query, index)` -- alternative iteration pattern (while loop)
- `wp.hash_grid_point_id(grid, tid)` -- maps thread ID to particle ID ordered by cell (improves cache performance)
- The `for index in neighbors:` syntax (Warp 1.10+) replaces the older while-loop pattern

**Performance note from DEM example:** 131,000 particles with 64 substeps per frame runs at interactive rates. The HashGrid is rebuilt every frame.

**Comparison with Taichi:** Taichi has NO built-in spatial hashing. You must implement it from scratch using fields. Warp's HashGrid is a production-quality implementation with cell-ordered thread dispatch, which is a significant development time savings and performance advantage.

---

### 4. Spring/Bond Connections (Directed Bonds)

**No built-in spring system. You build it from arrays and kernels.** This is exactly the same as Taichi.

The representation pattern for PBM bonds:

```python
# Bonds as parallel arrays (Structure of Arrays -- GPU friendly)
bond_src = wp.array(src_indices, dtype=wp.int32, device="cuda")    # source particle
bond_dst = wp.array(dst_indices, dtype=wp.int32, device="cuda")    # destination particle
bond_count = wp.array(counts, dtype=wp.float32, device="cuda")     # bond strength
bond_type = wp.array(types, dtype=wp.int32, device="cuda")         # bond category

# OR as a struct (Array of Structs)
@wp.struct
class Bond:
    src: wp.int32
    dst: wp.int32
    count: wp.float32
    bond_type: wp.int32

bonds = wp.array(bond_list, dtype=Bond, device="cuda")
```

**Direction matters and is trivially handled:** `bond_src[i]` -> `bond_dst[i]` is a directed edge. A->B and B->A are separate bond entries with potentially different counts. The force kernel (shown in section 2) naturally handles asymmetry by using the src/dst distinction.

**Spring-like forces are user-defined:**

```python
@wp.func
def spring_force(pos_a: wp.vec3, pos_b: wp.vec3, rest_length: float, stiffness: float):
    delta = pos_b - pos_a
    dist = wp.length(delta)
    if dist < 1.0e-6:
        return wp.vec3(0.0, 0.0, 0.0)
    direction = delta / dist
    displacement = dist - rest_length
    return direction * stiffness * displacement
```

**No adjacency list support** -- same as Taichi. Variable-length neighbor lists per particle are not a native data structure. Options:
1. Flat bond arrays (iterate all bonds in parallel) -- best for PBM since bond count is known
2. HashGrid neighbor query for distance-based interactions
3. Pre-allocated fixed-size neighbor arrays per particle (wasteful but cache-friendly)

**Assessment:** For PBM bonds, the flat array approach is natural and efficient. Each bond is a known directed pair with a count. The kernel iterates bonds in parallel, looks up particles by index, computes forces, and scatters with atomic_add.

---

### 5. Soft Body to Rigid Body Transition

**Not built-in, but implementable with careful design.** Warp does not have a rigid body solver in its core kernel API (the `warp.sim` module has one, but it is oriented toward robotics/character simulation, not our use case).

**Strategy for PBM crystallization:**

```python
@wp.struct
class Particle:
    token_id: wp.int32
    category: wp.int32
    position: wp.vec3
    velocity: wp.vec3
    mass: wp.float32
    is_rigid: wp.int32          # 0=free, 1=rigid
    rigid_group_id: wp.int32    # -1=none, else group index
    local_offset: wp.vec3       # offset from rigid group centroid

@wp.struct
class RigidGroup:
    centroid: wp.vec3
    velocity: wp.vec3
    angular_velocity: wp.vec3
    orientation: wp.quat
    num_members: wp.int32
    total_mass: wp.float32

@wp.kernel
def integrate_mixed(
    particles: wp.array(dtype=Particle),
    groups: wp.array(dtype=RigidGroup),
    forces: wp.array(dtype=wp.vec3),
    dt: float,
):
    tid = wp.tid()
    p = particles[tid]

    if p.is_rigid == 0:
        # FREE PARTICLE: normal Euler integration
        p.velocity = p.velocity + forces[tid] / p.mass * dt
        p.position = p.position + p.velocity * dt
    else:
        # RIGID PARTICLE: position determined by group centroid + offset
        g = groups[p.rigid_group_id]
        # Rotate local offset by group orientation
        rotated_offset = wp.quat_rotate(g.orientation, p.local_offset)
        p.position = g.centroid + rotated_offset
        p.velocity = g.velocity  # inherit group velocity

    particles[tid] = p

@wp.kernel
def detect_crystallization(
    particles: wp.array(dtype=Particle),
    grid: wp.uint64,
    positions: wp.array(dtype=wp.vec3),
    crystal_candidates: wp.array(dtype=wp.int32),  # output: 1 if should crystallize
    threshold_energy: float,
    radius: float,
):
    """Check if a particle's local neighborhood has settled into
    a low-energy configuration (recognized sequence)."""
    tid = wp.tid()
    i = wp.hash_grid_point_id(grid, tid)
    p = particles[i]

    if p.is_rigid == 1:
        return  # already crystallized

    pos_i = positions[i]
    local_energy = float(0.0)
    neighbor_count = int(0)

    neighbors = wp.hash_grid_query(grid, pos_i, radius)
    for index in neighbors:
        if index != i:
            delta = pos_i - positions[index]
            dist = wp.length(delta)
            if dist <= radius:
                local_energy += wp.length(particles[index].velocity)
                neighbor_count += 1

    # Low energy + sufficient neighbors = crystallization candidate
    if neighbor_count >= 2 and local_energy / float(neighbor_count) < threshold_energy:
        crystal_candidates[i] = 1
```

**The crystallization process would be:**
1. Run soft body simulation (all particles free)
2. Periodically run `detect_crystallization` kernel to find settled regions
3. Read candidates back to CPU, run pattern matching (is this a recognized sequence?)
4. If recognized: create a RigidGroup, set `is_rigid=1` for member particles, compute local offsets from centroid
5. Future integration uses rigid body path for those particles

**NEEDS TESTING:** Whether the mixed integration (some particles free, some rigid) causes performance issues due to thread divergence. On modern NVIDIA GPUs (sm_70+), thread divergence within a warp is handled better than older architectures, but it is still a potential concern.

**Comparison with Taichi:** Identical situation -- no built-in transition mechanism, must be user-implemented. Neither framework has a concept of "rigid body groups" in their core API. Both require the same pattern of flagging particles and branching in the integration kernel.

---

### 6. Custom Iteration and Simulation Loop Control

**Fully custom. Warp imposes NO fixed timestep, no fixed integrator, no simulation structure.**

The simulation loop is pure Python calling `wp.launch()`:

```python
class PBMSimulation:
    def __init__(self, particles, bonds, force_table):
        self.particles = particles
        self.bonds = bonds
        self.force_table = force_table
        self.forces = wp.zeros(len(particles), dtype=wp.vec3, device="cuda")
        self.grid = wp.HashGrid(128, 128, 128, device="cuda")
        self.time = 0.0

    def step(self, dt):
        """Single simulation step with custom logic."""
        # 1. Rebuild spatial hash
        self.grid.build(self.positions, cell_size)

        # 2. Zero forces
        self.forces.zero_()

        # 3. Compute bond forces (from PBM bonds)
        wp.launch(compute_bond_forces, dim=self.num_bonds,
                  inputs=[self.particles, self.bonds, self.force_table, self.forces])

        # 4. Compute proximity forces (from spatial neighbors)
        wp.launch(compute_proximity_forces, dim=self.num_particles,
                  inputs=[self.grid.id, self.particles, self.forces, search_radius])

        # 5. Integrate
        wp.launch(integrate_mixed, dim=self.num_particles,
                  inputs=[self.particles, self.groups, self.forces, dt])

        self.time += dt

    def run_assembly(self, target_time, adaptive=True):
        """Assembly direction: run until convergence or timeout."""
        dt = 0.001
        while self.time < target_time:
            if adaptive:
                energy = self.compute_total_energy()
                if energy < self.convergence_threshold:
                    break
                # Adaptive timestep based on energy
                dt = min(0.01, 0.001 / max(energy, 0.001))
            self.step(dt)

    def run_disassembly(self, input_text):
        """Disassembly direction: decompose text, settle, analyze."""
        self.load_particles_from_text(input_text)
        # Short settling run
        for _ in range(100):
            self.step(dt=0.005)
        # Analyze structure
        clusters = self.detect_clusters()
        energy_landscape = self.compute_energy_map()
        return clusters, energy_landscape
```

**CUDA graph capture for inner loops:**

If the simulation substep is fixed (same kernels, same dimensions), you can capture it as a CUDA graph for near-zero launch overhead:

```python
# Capture simulation substeps as a CUDA graph
with wp.ScopedCapture() as capture:
    for _ in range(64):  # 64 substeps
        wp.launch(compute_bond_forces, ...)
        wp.launch(compute_proximity_forces, ...)
        wp.launch(integrate_mixed, ...)
self.sim_graph = capture.graph

# Replay with zero Python overhead
def fast_step(self):
    self.grid.build(self.positions, cell_size)  # cannot be in graph (data-dependent)
    wp.capture_launch(self.sim_graph)  # replays all 64 substeps
```

**Note:** HashGrid.build() cannot be captured in a graph because it depends on runtime data. The graph captures only the fixed kernel launch sequence.

**Comparison with Taichi:** Nearly identical. Both give you full control over the simulation loop from Python. Neither imposes a fixed integrator or timestep. Warp's CUDA graph capture is a performance advantage for fixed inner loops (Taichi does not have this).

---

### 7. Data Input/Output and NumPy Interop

**Excellent. First-class NumPy integration with zero-copy where possible.**

```python
import numpy as np
import warp as wp

# NumPy -> Warp (copies to GPU)
positions_np = np.random.randn(100000, 3).astype(np.float32)
positions_wp = wp.array(positions_np, dtype=wp.vec3, device="cuda")
# or equivalently:
positions_wp = wp.from_numpy(positions_np, dtype=wp.vec3, device="cuda")

# Warp GPU -> NumPy (copies from GPU, auto-synchronizes)
result_np = positions_wp.numpy()  # returns np.ndarray on CPU

# Warp CPU -> NumPy (ZERO COPY -- shares memory)
cpu_array = wp.array(positions_np, dtype=wp.vec3, device="cpu")
view = cpu_array.numpy()  # zero-copy view, no data movement

# External memory aliasing (zero-copy wrap of contiguous buffer)
external_np = np.zeros(1024, dtype=np.float32)
wp_alias = wp.array(external_np, dtype=float, device="cpu", copy=False)
# wp_alias and external_np share the same memory
```

**LMDB integration path:**

LMDB memory-mapped data can be fed to Warp through NumPy as an intermediary:

```python
import lmdb
import numpy as np
import warp as wp

env = lmdb.open("hcp_pbms.db", readonly=True, lock=False)
with env.begin() as txn:
    raw = txn.get(b"document_key")
    # Deserialize PBM data (msgpack, etc.)
    pbm_data = deserialize(raw)

    # Convert to numpy arrays
    token_ids = np.array(pbm_data["token_ids"], dtype=np.int32)
    categories = np.array(pbm_data["categories"], dtype=np.int32)
    bond_src = np.array(pbm_data["bond_src"], dtype=np.int32)
    bond_dst = np.array(pbm_data["bond_dst"], dtype=np.int32)
    bond_counts = np.array(pbm_data["bond_counts"], dtype=np.float32)

    # Transfer to GPU
    wp_token_ids = wp.array(token_ids, dtype=wp.int32, device="cuda")
    wp_categories = wp.array(categories, dtype=wp.int32, device="cuda")
    wp_bond_src = wp.array(bond_src, dtype=wp.int32, device="cuda")
    wp_bond_dst = wp.array(bond_dst, dtype=wp.int32, device="cuda")
    wp_bond_counts = wp.array(bond_counts, dtype=wp.float32, device="cuda")
```

**Additional interop:** DLPack protocol, `__cuda_array_interface__` (CuPy), PyTorch (`wp.from_torch`/`wp.to_torch`), JAX FFI.

**Comparison with Taichi:** Similar capability. Taichi uses `field.to_numpy()`/`field.from_numpy()`. Warp's array-based API is slightly more flexible (supports multi-dimensional arrays, structured dtypes, and zero-copy aliasing of external memory).

---

### 8. Multiple Independent Systems (Concurrent Simulations)

**Three approaches, in order of preference:**

**Approach A: Batched in one kernel (recommended for PBM)**

Pack multiple PBM documents into one particle array with a `system_id` per particle:

```python
@wp.struct
class Particle:
    token_id: wp.int32
    category: wp.int32
    system_id: wp.int32       # which document/envelope this belongs to
    position: wp.vec3
    velocity: wp.vec3
    mass: wp.float32

@wp.kernel
def compute_forces_batched(
    grid: wp.uint64,
    particles: wp.array(dtype=Particle),
    forces: wp.array(dtype=wp.vec3),
    radius: float,
):
    tid = wp.tid()
    i = wp.hash_grid_point_id(grid, tid)
    p_i = particles[i]

    f = wp.vec3()
    neighbors = wp.hash_grid_query(grid, p_i.position, radius)

    for index in neighbors:
        if index != i:
            p_j = particles[index]
            # CRITICAL: only interact within same system
            if p_j.system_id == p_i.system_id:
                # compute force...
                delta = p_j.position - p_i.position
                dist = wp.length(delta)
                if dist > 0.0 and dist <= radius:
                    f = f + delta / dist
    forces[i] = f
```

This runs ALL systems in one GPU launch. The `system_id` check prevents cross-system contamination. Efficient because the GPU is fully utilized.

**Approach B: CUDA streams for concurrent launches**

```python
stream_a = wp.Stream("cuda")
stream_b = wp.Stream("cuda")

# Launch different simulations on different streams (can execute concurrently)
wp.launch(kernel_a, dim=n_a, inputs=[...], stream=stream_a)
wp.launch(kernel_b, dim=n_b, inputs=[...], stream=stream_b)

wp.synchronize()  # wait for both
```

**Approach C: Multi-GPU**

```python
# Different simulations on different GPUs
with wp.ScopedDevice("cuda:0"):
    sim_a = PBMSimulation(...)
with wp.ScopedDevice("cuda:1"):
    sim_b = PBMSimulation(...)
```

**Warp v1.11 multi-world support:**

The group-aware BVH/Mesh queries in v1.11 allow building a single acceleration structure with geometry from multiple "worlds" and querying each independently. Currently this applies to `wp.Bvh` and `wp.Mesh`, NOT `wp.HashGrid`. For particle-only systems, Approach A (batched with system_id) is the current recommended path.

**Comparison with Taichi:** Warp is significantly stronger here. Taichi has sequential kernel launches from Python with no stream-level concurrency. Warp supports CUDA streams, multi-GPU, and CUDA graph capture with multi-stream forking. The batched approach works in both frameworks, but Warp can additionally exploit hardware concurrency.

---

### 9. Querying State (Post-Simulation Analysis)

**Full access. Every array is readable from Python after kernel execution.**

```python
# After simulation steps, read back results
positions_np = particle_positions.numpy()  # GPU -> CPU, auto-sync
velocities_np = particle_velocities.numpy()

# Compute energy on GPU, read scalar result
energy_array = wp.zeros(1, dtype=float, device="cuda")

@wp.kernel
def total_kinetic_energy(
    particles: wp.array(dtype=Particle),
    result: wp.array(dtype=float),
):
    tid = wp.tid()
    p = particles[tid]
    ke = 0.5 * p.mass * wp.dot(p.velocity, p.velocity)
    wp.atomic_add(result, 0, ke)

wp.launch(total_kinetic_energy, dim=num_particles,
          inputs=[particles, energy_array])

energy = energy_array.numpy()[0]  # single float
print(f"Total kinetic energy: {energy}")
```

**Cluster detection on GPU:**

```python
@wp.kernel
def find_clusters(
    grid: wp.uint64,
    particles: wp.array(dtype=Particle),
    positions: wp.array(dtype=wp.vec3),
    cluster_sizes: wp.array(dtype=wp.int32),  # per-particle neighbor count
    radius: float,
):
    tid = wp.tid()
    i = wp.hash_grid_point_id(grid, tid)
    pos_i = positions[i]
    count = int(0)

    neighbors = wp.hash_grid_query(grid, pos_i, radius)
    for index in neighbors:
        if index != i:
            dist = wp.length(pos_i - positions[index])
            if dist <= radius:
                count += 1

    cluster_sizes[i] = count

# Read back and analyze in Python/NumPy
wp.launch(find_clusters, dim=num_particles,
          inputs=[grid.id, particles, positions, cluster_sizes, cluster_radius])

sizes = cluster_sizes.numpy()
dense_regions = np.where(sizes > 5)[0]  # particles with >5 neighbors
isolated = np.where(sizes == 0)[0]       # isolated particles
```

**Full analysis toolkit available:** Since data transfers GPU->CPU via `.numpy()`, you can use any Python analysis library (scipy spatial, sklearn clustering, networkx, etc.) on the results. The GPU does the heavy simulation; Python does the structural analysis.

**Comparison with Taichi:** Identical capability. Both allow reading back any field/array to NumPy after kernel execution. No difference here.

---

### 10. How "Define It Yourself" Is Warp?

**Very. Comparable to Taichi with several additional built-ins.**

| Capability | Warp | Taichi | Notes |
|-----------|------|--------|-------|
| Custom kernels | `@wp.kernel` | `@ti.kernel` | Both: full control over thread logic |
| Custom functions | `@wp.func` | `@ti.func` | Both: GPU-callable helper functions |
| Custom structs | `@wp.struct` | `ti.types.struct()` | Both: arbitrary typed composites |
| Spatial hashing | `wp.HashGrid` (built-in) | DIY from fields | **Warp advantage** |
| BVH/mesh queries | `wp.Bvh`, `wp.Mesh` (built-in) | DIY | **Warp advantage** |
| Auto-diff | `wp.Tape` + `wp.grad()` | `ti.ad.Tape` | Warp: inline `wp.grad()` is more flexible |
| CUDA graphs | `wp.ScopedCapture` | Not available | **Warp advantage** |
| Concurrent streams | `wp.Stream` | Not available | **Warp advantage** |
| Sparse data structures | Not built-in | SNodes (pointer, bitmasked) | **Taichi advantage** |
| Vulkan backend | No | Yes | **Taichi advantage** |
| Volume data (OpenVDB) | `wp.Volume` (built-in) | Not built-in | Warp advantage (not relevant for PBM) |
| Random numbers | `wp.rand_*()` in kernels | `ti.random()` in kernels | Both |
| Atomic operations | `wp.atomic_add/min/max` | `ti.atomic_add/min/max` | Both |
| 2D/3D array indexing | `array[i, j]`, ndim=2/3/4 | `field[i, j]` | Both |

**What Warp imposes:**
- Kernel arguments must be type-annotated (no dynamic typing)
- No runtime recursion in kernels
- No dynamic memory allocation in kernels
- No kernel-to-kernel calls (must return to Python between launches)
- Arrays are fixed-size once allocated (can reallocate from Python)

**What Warp leaves to you:**
- Physics model (forces, potentials, integrators)
- Simulation structure (loop, timestep, convergence criteria)
- Data layout (SoA vs AoS -- your choice via struct design)
- Analysis algorithms (clustering, pattern matching)
- Bond topology and graph structure

**Bottom line:** Warp is a "bring your own physics" compute compiler with better built-in spatial data structures than Taichi. It gives you GPU-accelerated building blocks (hash grids, BVH, auto-diff, atomic ops, N-dimensional arrays) and you assemble them into your simulation.

---

## Assembly vs Disassembly: Can Warp Handle Both Directions?

### Assembly (Output/Generation)

**Excellent fit.** The simulation loop pattern is:
1. Initialize particles from conceptual target (token vocabulary + category assignments)
2. Load force tables from PBM bond strength data
3. Run simulation: particles attract/repel based on forces derived from bond tables
4. Clusters form as particles settle into energy minima
5. Detect crystallized groups (low energy, stable configuration)
6. Lock crystallized groups into rigid bodies
7. Read out particle ordering as assembled text

Warp provides everything needed: HashGrid for proximity, custom kernels for forces, custom integration, state queries for convergence detection.

### Disassembly (Input/Analysis)

**Also excellent fit.** The analysis pattern is:
1. Tokenize input text -> particle array with positions along a line
2. Establish bonds from adjacent token pairs
3. Load force tables
4. Run short settling simulation (let structure relax under forces)
5. Query resulting state: which clusters formed? What are the energy levels? What tokens are co-located?
6. Map structure back to conceptual space

Warp's HashGrid queries and full state access make disassembly straightforward. The key advantage over OpenMM: you can interrogate any aspect of the state at any time.

### Neither Direction Is Harder

Both use the same simulation infrastructure. Assembly runs longer (converging from disorder to structure). Disassembly runs shorter (settling from ordered input to equilibrium). The kernels, data structures, and force models are identical.

---

## Head-to-Head: Warp vs Taichi for HCP

| Factor | Warp | Taichi | Winner |
|--------|------|--------|--------|
| Project health | Active (NVIDIA-backed, v1.11.1, Feb 2026) | Maintenance mode (v1.7.4, Jul 2025) | **Warp** |
| Built-in spatial hashing | Yes (wp.HashGrid) | No (DIY) | **Warp** |
| Auto-diff (inline) | wp.grad() -- inline in forward pass | Tape-based only | **Warp** |
| CUDA graph capture | Yes (ScopedCapture) | No | **Warp** |
| Concurrency | Streams + multi-GPU | Sequential launches | **Warp** |
| Custom kernels | Equivalent | Equivalent | Tie |
| Custom structs | Equivalent | Equivalent | Tie |
| 2D lookup tables | Equivalent (array2d) | Equivalent (2D field) | Tie |
| Sparse data structures | None | SNodes | **Taichi** |
| Cross-vendor GPU | No (NVIDIA only) | Yes (Vulkan, Metal, CPU) | **Taichi** |
| Compilation speed | C++/CUDA intermediate | LLVM/SPIRV intermediate | Taichi (faster iteration) |
| Documentation quality | Excellent + examples | Good but stale | **Warp** |
| Community/support | NVIDIA resources, GTC talks | Reduced (company pivoted) | **Warp** |

**Recommendation:** Warp is the stronger choice for HCP's particle simulation layer. The built-in HashGrid alone saves weeks of development compared to implementing spatial hashing from scratch in Taichi. The active development, CUDA graph capture, and inline autodiff are additional significant advantages. The NVIDIA-only limitation is acceptable for a system that will run on NVIDIA GPUs.

---

## Items Requiring Hands-On Testing

1. **Struct field performance at scale** -- 100k+ Particle structs with 8 fields, kernel throughput
2. **2D table lookup throughput** -- N=100k parallel lookups from a 500x500 table
3. **HashGrid rebuild cost** -- Rebuilding every frame for 100k particles, measure overhead
4. **Directed bond iteration** -- 500k bonds, asymmetric force computation, throughput
5. **Mixed free/rigid integration** -- Thread divergence impact with 50% rigid particles
6. **CUDA graph capture** -- Measure overhead reduction for fixed inner loops
7. **Batched multi-system** -- 10 independent PBMs in one array with system_id filtering
8. **wp.grad() for force computation** -- Verify inline autodiff works with table lookups
9. **Data pipeline latency** -- LMDB read -> numpy -> wp.array -> GPU, end-to-end timing
10. **Cluster detection at scale** -- HashGrid-based neighbor counting for 100k particles

---

## Sources

- [NVIDIA Warp Documentation (v1.11.1)](https://nvidia.github.io/warp/)
- [Warp GitHub Repository](https://github.com/NVIDIA/warp)
- [Warp Blog Post: Differentiable Simulation](https://developer.nvidia.com/blog/creating-differentiable-graphics-and-physics-simulation-in-python-with-nvidia-warp/)
- [Warp DEM Example (source)](https://github.com/NVIDIA/warp/blob/main/warp/examples/core/example_dem.py)
- [Warp SPH Example (source)](https://github.com/NVIDIA/warp/blob/main/warp/examples/core/example_sph.py)
- [Warp Particle Repulsion Example (source)](https://github.com/NVIDIA/warp/blob/main/warp/examples/optim/example_particle_repulsion.py)
- [Warp N-Body Tile Example (source)](https://github.com/NVIDIA/warp/blob/main/warp/examples/tile/example_tile_nbody.py)
- [Warp Struct Types (DeepWiki)](https://deepwiki.com/NVIDIA/warp/2.4-context-and-device-management)
- [Warp Interoperability (DeepWiki)](https://deepwiki.com/NVIDIA/warp/6-interoperability)
- [Warp FAQ: Comparison to Taichi](https://nvidia.github.io/warp/faq.html)
- [Warp Changelog (v1.11.0, v1.11.1)](https://github.com/NVIDIA/warp/blob/main/CHANGELOG.md)
- [Warp vs Taichi Discussion (GitHub Issue #30)](https://github.com/NVIDIA/warp/issues/30)
- [Taichi Discussion on Warp](https://github.com/taichi-dev/taichi/discussions/8184)
