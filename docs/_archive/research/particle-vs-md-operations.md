# Particle Physics Simulation vs Molecular Dynamics: Operational Comparison

**Date:** 2026-02-17
**Status:** Complete research
**Purpose:** Determine which computational model better fits HCP cognitive modeling -- specifically for systems where structure EMERGES from particle interactions carrying semantic data.

---

## Executive Summary

"Particle physics simulation" and "molecular dynamics" are not actually two different computational paradigms. They are points on a spectrum of the SAME paradigm -- particles interacting via forces -- with different assumptions about what is known upfront. The real operational distinction is between three computational models:

1. **Topology-first (MD):** Bonds are declared upfront. Forces act on known connections. The GPU computes energy/forces for a fixed graph.
2. **Discovery-first (N-body / DEM / particle life):** No bonds declared. All interactions discovered at runtime via spatial queries. The GPU computes pairwise distances and applies force rules.
3. **Hybrid (reactive MD, DPD):** Start with some topology, but bonds can form/break based on runtime conditions.

**For HCP, the answer is: discovery-first for inference, topology-first for assembly, and the transition between them is the core architectural challenge.** The good news: this exact pattern (discover structure, then track it) exists in multiple frameworks. The bad news: no single framework does both halves well.

---

## Part 1: What the GPU Actually Computes

### The Shared Core (All Methods)

Every particle simulation -- MD, N-body, DEM, SPH, particle life -- executes the same fundamental loop:

```
REPEAT:
  1. Find neighbors (who interacts with whom)
  2. Compute forces (given neighbors, compute F = f(distance, type, parameters))
  3. Integrate (update positions/velocities from forces)
  4. (Optional) Update topology (form/break bonds, create/destroy particles)
```

The difference between methods is WHERE the computation concentrates and WHAT assumptions each step makes.

### Step 1: Neighbor Discovery -- The Critical Fork

This is where methods diverge most sharply. The question is: **how do you know which particles interact?**

#### Topology-First (MD: OpenMM, GROMACS, AMBER)

**Bonded interactions:** Known at setup. The bond list is a static array. The GPU iterates it directly.
```
for each bond (i, j) in bond_list:   // O(B), B = number of bonds
    compute_bonded_force(i, j)
```

**Non-bonded interactions:** Discovered via neighbor list, but with CUTOFF. Only particles within distance R_cutoff interact.
```
// Build neighbor list (expensive, done every N steps):
for each particle i:
    for each particle j in nearby cells:
        if distance(i, j) < R_cutoff + buffer:
            add j to neighbors[i]

// Use neighbor list (cheap, done every step):
for each particle i:
    for each j in neighbors[i]:
        compute_nonbonded_force(i, j)    // Lennard-Jones, Coulomb, etc.
```

**Key detail:** The neighbor list is rebuilt every 10-20 timesteps (amortized cost). Between rebuilds, it's a static array traversal. The buffer distance determines how often rebuilds are needed -- if no particle has moved more than buffer/2, the list is still valid.

**Long-range forces (electrostatics):** Computed via Particle Mesh Ewald (PME) or Fast Multipole Method (FMM). These use FFTs on grids -- fundamentally different from pairwise computation. This is where 40-60% of MD compute time goes.

#### Discovery-First (N-body, DEM, Particle Life)

**ALL interactions discovered at runtime.** No bond list exists. The GPU must find interacting pairs.

**Brute-force (small N):**
```
for each particle i:           // O(N^2)
    for each particle j != i:
        compute_force(i, j)
```
Practical for N < 10,000 on modern GPUs. Used in gravitational N-body (GPU Gems 3 reports 20 FLOP per pair).

**Spatial hash / uniform grid (medium N):**
```
// Phase 1: Bin particles into grid cells
for each particle i:
    cell = hash(position[i] / cell_size)
    atomic_add(cell_count[cell], 1)

// Phase 2: Prefix sum to compute offsets
parallel_prefix_sum(cell_count) -> cell_offset

// Phase 3: Place particles into sorted array
for each particle i:
    idx = atomic_add(cell_offset[cell], 1)
    sorted_particles[idx] = i

// Phase 4: Query neighbors via cell lookup
for each particle i:
    for each neighboring_cell in 3x3x3 stencil:
        for each j in sorted_particles[cell_offset[c]..cell_offset[c+1]]:
            if distance(i, j) < R_cutoff:
                compute_force(i, j)
```
Reduces to O(N) expected time. Particle Life uses this with 32x32 grid, handles 65K particles at 60 FPS on GTX 1060.

**BVH tree (large N, non-uniform density):**
```
// Build linear BVH via Morton codes
for each particle i:
    morton[i] = interleave_bits(position[i])
sort(morton)
build_tree_from_sorted_morton_codes()

// Traverse tree for each particle
for each particle i:
    stack = [root]
    while stack not empty:
        node = stack.pop()
        if node.bounds overlaps query_sphere(i, R_cutoff):
            if node.is_leaf:
                compute_force(i, node.particle)
            else:
                stack.push(node.left, node.right)
```
O(N log N). HOOMD-blue uses this (LBVH) for systems with >2:1 size asymmetry. Outperforms cell lists for sparse or non-uniform systems.

### Step 2: Force Computation -- What Changes Between Methods

| Method | Force Types | GPU Operation | Dominant Cost |
|--------|------------|---------------|---------------|
| **MD (atomistic)** | Bonded (bond, angle, dihedral, improper) + Non-bonded (LJ, Coulomb) + Long-range (PME via FFT) | Table lookup + polynomial evaluation + FFT | PME (40-60% of total) |
| **MD (coarse-grained)** | Simplified pair potentials, soft repulsion | Table lookup + simple math | Neighbor list build |
| **DEM (granular)** | Normal (spring), tangential (friction), damping | Contact detection + spring force | Contact detection (70-80% of total) |
| **DPD** | Conservative (soft repulsion) + Dissipative (velocity-dependent) + Random (stochastic) | 3-term force sum per pair | Neighbor discovery |
| **N-body (gravitational)** | Gravity: F = G*m1*m2/r^2 | 20 FLOP per pair, tiled for shared memory | All-pairs computation |
| **SPH (fluid)** | Pressure + viscosity + surface tension (kernel-weighted) | Kernel function evaluation over neighbors | Kernel computation |
| **Particle Life** | Asymmetric attraction/repulsion from type-pair lookup table | Table[type_A][type_B] * distance_function | Neighbor force loop |

**Critical observation for HCP:** The force computation in Particle Life is operationally identical to what HCP needs -- a 2D lookup table indexed by (type_A, type_B) producing a force strength, multiplied by a distance function. The asymmetry (A->B != B->A) is built in because the lookup is directional. This is exactly the Discrete2DFunction concept from the OpenMM evaluation, but without the MD framework overhead.

### Step 3: Integration -- Mostly Identical Across Methods

All methods use some variant of:
```
velocity[i] += force[i] / mass[i] * dt    // or equivalent
position[i] += velocity[i] * dt
```

Variations:
- **Velocity Verlet** (MD): Half-step velocity update, position update, recompute forces, half-step velocity update. More accurate, 2 force evaluations per step.
- **Symplectic Euler** (game physics, particle life): Simple, fast, slightly less accurate.
- **Leapfrog** (N-body): Velocity at half-steps, position at full steps. Good energy conservation.

For HCP this is a detail, not a design driver. Any integrator works.

### Step 4: Topology Update -- Where Methods Truly Diverge

#### No topology changes (classic MD, N-body)
Bonds are fixed. Particles don't appear or disappear. The GPU never modifies the bond list.

#### Reactive topology changes (LAMMPS fix bond/react, fix bond/break)
Per the LAMMPS documentation:
1. Every N timesteps, loop over neighbor lists looking for eligible pairs
2. Match local topology against pre-reaction template (graph pattern matching)
3. If match found: modify bond arrays, update angle/dihedral tables, optionally change atom types
4. Force immediate neighbor list rebuild on same timestep

**This is expensive.** Bond/react documentation explicitly warns: "All of these operations increase the cost of a time step." The topology matching is essentially subgraph isomorphism -- NP-hard in general, tractable only because templates are small and searches are local.

#### Dynamic topology (HOOMD-blue MeshDynamicalBonding)
Runtime bond creation/deletion during GPU simulation. Relatively new feature (added ~2023-2024). Allows mesh surfaces to dynamically reorganize.

#### Emergent topology (Particle Life, ALIEN)
No explicit topology at all. "Structure" is an emergent property of force equilibria. Particles cluster because force rules create stable distance arrangements, not because bonds exist. If you want to DETECT the structure, you must analyze the simulation state from outside -- the simulation itself has no concept of "structure formed."

**This is the key insight for HCP:** In discovery-first simulation, structure is implicit. You must run a separate analysis pass to identify what structures have formed. This analysis pass IS the inference step.

---

## Part 2: Spatial Data Structures Comparison

| Data Structure | Build Cost | Query Cost | Memory | Best For | Used By |
|---------------|-----------|-----------|--------|----------|---------|
| **Uniform Grid / Hash Grid** | O(N) | O(1) per cell lookup | O(N + cells) | Uniform density, similar cutoff radii | HOOMD-blue (Cell), Particle Life, NVIDIA particles demo |
| **Stencil Cell List** | O(N) | O(1) with precomputed stencil | O(N + cells) | Mixed cutoff radii (up to 2:1) | HOOMD-blue (Stencil) |
| **BVH (Linear)** | O(N log N) via Morton sort | O(log N) per query | O(N) | Non-uniform density, size asymmetry >2:1, sparse systems | HOOMD-blue (Tree), quantized BVH for MD |
| **Octree** | O(N log N) | O(log N) | O(N) | 3D sparse data, adaptive resolution | Barnes-Hut N-body, FMM |
| **Verlet List** | O(N * neighbors) | O(1) -- direct iteration | O(N * avg_neighbors) | Dense systems with infrequent rebuild | OpenMM, most MD codes |
| **Sweep and Prune (SAP)** | O(N log N) sort | O(N) sweep | O(N) | Broad phase for rigid bodies, mostly static scenes | Bullet, PhysX broadphase |
| **Dynamic AABB Tree** | O(N log N) | O(log N) | O(N) | Rigid bodies with varied sizes | Bullet (btDbvtBroadphase), Box2D |

**For HCP's use case:** The uniform grid / spatial hash is the right choice. Particles have uniform interaction radius (no size asymmetry), density is moderate (10K-1M particles), and the build cost must be low because structure changes dynamically during inference. BVH is overkill unless particle density becomes highly non-uniform.

---

## Part 3: Framework Comparison -- Operations Focus

### OpenMM (Topology-First MD)

**What GPU computes:**
- Bonded forces: iterate bond list, evaluate energy expression per bond
- Non-bonded forces: build Verlet neighbor list from cell list, evaluate LJ + Coulomb per pair
- Custom forces: evaluate user-defined algebraic expressions with Discrete2DFunction lookups
- PME: 3D FFT on charge grid (not relevant for HCP)
- Integration: Velocity Verlet or Langevin

**Operations MD has that particle sim does NOT:**
- Long-range electrostatics (PME / Ewald sums) -- O(N log N) FFT-based
- Constraint algorithms (SHAKE, SETTLE) -- fix bond lengths/angles
- Thermostats (Langevin, Nose-Hoover) -- maintain temperature ensemble
- Barostats -- maintain pressure
- Periodic boundary conditions with minimum image convention
- Implicit solvent models (GB/SA)

**Operations MD LACKS that particle sim has:**
- Dynamic topology (bonds are frozen after Context creation)
- Asymmetric forces (Newton's third law is enforced)
- Runtime structural analysis (OpenMM is "assemble and run, blind to results")
- Dynamic particle creation/destruction

**HCP fit assessment:** OpenMM excels at evaluating forces on known structures (assembly validation). It cannot discover structures (inference). The frozen topology and symmetric force requirements are fundamental architectural limitations, not bugs.

### LAMMPS (Hybrid MD + Particle Methods)

**What GPU computes:**
- Same as OpenMM for standard MD
- PLUS: granular contact detection (DEM), soft particle dynamics (DPD), reactive bond formation/breaking
- Kokkos backend provides portable GPU execution across CUDA/HIP/SYCL

**Unique operations:**
- `fix bond/react`: Runtime graph pattern matching against templates, topology mutation
- `fix bond/break`: Distance-based bond breaking with topology cleanup
- `pair_style granular`: Contact force computation with normal + tangential + rolling + twisting components
- `pair_style dpd`: Three-term force (conservative + dissipative + random) per pair

**Key advantage:** LAMMPS is the only major MD package that can both maintain topology AND modify it at runtime. The `fix bond/react` command performs subgraph matching to find reaction sites, then applies predetermined topology changes.

**Key limitation:** Topology changes are template-based. You define pre-reaction and post-reaction templates. The engine matches and transforms -- it does not DISCOVER new topologies. This is pattern matching, not emergence.

**HCP fit assessment:** LAMMPS's reactive MD is closer to what HCP needs than OpenMM, because topology can change. But the template-based approach (pre-define what bonds CAN form) is the wrong model for inference where the structure should emerge without templates.

### HOOMD-blue (GPU-Native MD + Monte Carlo)

**What GPU computes:**
- Pair forces via cell list, stencil, or BVH tree neighbor lists (all on GPU)
- Bond, angle, dihedral forces (standard MD)
- Monte Carlo moves (accept/reject based on energy) -- alternative to dynamics
- Dynamic mesh bonding (MeshDynamicalBonding) -- bonds can form/break at runtime

**Unique operations:**
- Monte Carlo integration: propose random move, compute energy delta, accept with Boltzmann probability. No forces computed -- only energies.
- Hard particle MC: geometric overlap detection for anisotropic shapes (not force-based at all)
- Three distinct neighbor list implementations selectable per simulation (Cell, Stencil, Tree/LBVH)

**Key advantage:** Monte Carlo mode. Instead of computing forces and integrating, you propose configurations and accept/reject based on energy. This is closer to a search algorithm than a dynamics engine. For HCP inference, MC could explore the space of possible bond configurations.

**Key limitation:** Still fundamentally a chemistry/physics tool. Asymmetric forces are not natural. Per-particle identity beyond "type" is limited.

### Taichi Lang (Programmable Compute Layer)

**What GPU computes:** Whatever you write. No built-in physics.

**Relevant primitives:**
- Parallel iteration over struct fields (particles with arbitrary typed data)
- 2D field lookups (force constant tables)
- Atomic operations for thread-safe accumulation
- Automatic parallelization of outermost loops

**What you must implement yourself:**
- Neighbor discovery (spatial hash, BVH, brute force)
- Force computation
- Integration
- Topology management
- Structural analysis

**Key advantage:** Total control. Asymmetric forces, directed bonds, per-particle integer IDs, custom structural queries -- all trivially expressible. No framework fights you.

**Key limitation:** You are writing a physics engine from scratch. 1-2 person-months for a basic particle system with spatial hashing. The existing Taichi evaluation (in this repo) covers this thoroughly.

### Game Physics (PhysX / Jolt / Bullet)

**What GPU computes:**

The pipeline is fundamentally different from particle simulation:

```
Broadphase:  AABB overlap test on bounding volumes     -> candidate pairs
Narrowphase: GJK/EPA exact collision on convex shapes  -> contact points + normals
Solver:      Constraint resolution (PGS, TGS)          -> impulses
Integration: Apply impulses, update transforms          -> new positions
```

**Operations game physics has that MD does NOT:**
- Broad/narrow phase split (AABB tree + GJK/EPA vs. neighbor list + cutoff)
- Contact manifold management (persistent contacts across frames)
- Constraint solver (joints, contacts as constraints, iterative resolution)
- Sleeping / island detection (skip computation for static objects)
- Continuous collision detection (CCD) for fast-moving objects

**Operations game physics LACKS:**
- Long-range forces (no electrostatics)
- Bonded forces (no bond/angle/dihedral potentials)
- Thermodynamic ensembles
- Energy conservation guarantees
- Per-particle force accumulation from many simultaneous interactions

**Key insight:** Game physics engines are CONSTRAINT solvers, not FORCE integrators. They compute "what contacts exist and how to resolve them" rather than "what forces act and where do particles move." The distinction matters: constraints enforce geometry, forces produce dynamics.

**HCP fit assessment:** Game physics is the wrong computational model. HCP needs continuous force evaluation to find energy minima, not contact constraint resolution. Game physics is for rigid/soft body dynamics where objects have shape and volume -- HCP particles are points with identity.

---

## Part 4: The Core Question -- Which Model for Emergent Structure?

### Restating the Requirements

HCP needs:
1. Particles carry integer IDs, category data (per-particle identity)
2. Force rules defined by 2D lookup table: (type_A, type_B) -> force_strength
3. Forces are DIRECTED: A->B != B->A
4. Structure should EMERGE from force interactions (no predefined topology)
5. Once structure is detected, it needs to be tracked and queried
6. Thousands of concurrent particle systems
7. Structural analysis: proximity, comparison, substructure detection

### Why MD (Topology-First) Is Wrong for Inference

MD assumes you KNOW the bonds before simulation starts. The entire computational pipeline is optimized for this:
- Bond lists are static arrays, traversed without branching
- Neighbor lists are for NON-bonded interactions (already bonded pairs are excluded)
- Context creation compiles GPU kernels for the specific topology
- Adding/removing bonds requires rebuilding the Context (expensive)

When you try to use MD for structure DISCOVERY, you hit:
- **Chicken-and-egg:** You need bonds to compute bonded forces, but you need forces to discover bonds
- **Symmetric forces:** Newton's third law is hardcoded -- F(A->B) = -F(B->A)
- **Frozen topology:** Can't form new bonds without stopping simulation, modifying topology, restarting

LAMMPS's reactive MD partially addresses this, but with TEMPLATE-BASED matching -- you must pre-define which bonds CAN form. This is guided discovery, not emergence.

### Why Discovery-First (Particle Life Model) Is Right for Inference

The Particle Life computational model matches HCP's inference requirements almost exactly:

| HCP Requirement | Particle Life Operation |
|----------------|------------------------|
| Per-particle identity (token_id, category) | Per-particle struct: species ID + custom fields |
| 2D force lookup table | M x M array indexed by (type_A, type_B) |
| Directed/asymmetric forces | Force = table[type_A][type_B], NOT = table[type_B][type_A] |
| Emergent structure | Stable configurations from force equilibria |
| No predefined topology | All interactions discovered via spatial hash |
| Concurrent systems | Batch into single field with system_id partition |

**The Particle Life model does what HCP needs:**
- Start with a bag of particles (tokens with categories)
- Apply force rules from a lookup table
- Let particles settle into equilibrium configurations
- The configurations that form ARE the inferred structures
- Analyze the configurations to extract structure

**What it DOESN'T do (and HCP still needs):**
- Track which structures have formed (no topology tracking)
- Efficiently query substructures
- Transition from "discovering structure" to "evolving known structure"

### The Operational Model for HCP

**Phase 1: Discovery (Particle Life model)**
```
GPU Operations per timestep:
  1. Spatial hash build: O(N) -- bin particles into grid
  2. Neighbor enumeration: O(N * avg_neighbors) -- 3x3 stencil per particle
  3. Force lookup: O(N * avg_neighbors) -- table[type_i][type_j] per pair (ASYMMETRIC)
  4. Force accumulation: O(N * avg_neighbors) -- sum forces per particle
  5. Integration: O(N) -- update positions/velocities
  6. (Periodic) Structure detection: O(N * avg_neighbors) -- identify stable clusters
```

**Phase 2: Tracking (Topology model)**
```
Once structure is detected:
  - Build explicit bond list from stable proximity relationships
  - Switch to topology-aware force computation (bonded + non-bonded)
  - Track bond strengths, detect weakening/breaking
  - Enable structural queries (subgraph matching, comparison)
```

**Phase 3: Evolution (Hybrid)**
```
Known structure evolves under new input:
  - Bonded forces maintain existing structure
  - Non-bonded forces from new particles interact with existing
  - New bonds can form (discovery within existing context)
  - Existing bonds can break (force exceeds threshold)
```

---

## Part 5: Hybrid Approaches -- Do They Exist?

### Yes, and they have specific names:

#### 1. Coarse-Grained Self-Assembly MD
Used in biochemistry to study how molecules spontaneously form structures:
- Start with particles in random positions (no bonds)
- Run dynamics with attractive/repulsive pair potentials
- Structures form spontaneously (micelles, vesicles, membranes)
- Detect formed structures by analyzing radial distribution functions
- Once detected, can switch to finer-grained representation

**This is exactly the discovery->tracking pattern HCP needs.** The computational model is: pair potentials (no bonds) -> dynamics -> structure emerges -> detect -> track.

**Frameworks:** LAMMPS (pair_style dpd), HOOMD-blue (pair potentials + Monte Carlo), Martini (coarse-grained MD)

#### 2. Reactive Coarse-Grained MD
Extends coarse-grained MD with bond formation rules:
- Pair potentials drive particles together
- When distance < threshold AND type compatibility, FORM A BOND
- Bond is now tracked explicitly
- Reverse: when bond force > threshold, BREAK THE BOND
- Topology is dynamic, driven by forces

**LAMMPS implements this** via `fix bond/create` and `fix bond/break`. The compute operations per timestep add:
```
Every N steps:
  1. Loop over neighbor list: O(N * avg_neighbors)
  2. Check distance + type criteria: O(1) per pair
  3. If match: create bond entry, update topology arrays
  4. Rebuild neighbor list immediately
```

#### 3. Particle Life + Post-Hoc Graph Analysis
Used in artificial life research (ALIEN simulator):
- Run particle dynamics with asymmetric forces (no topology)
- Periodically analyze spatial configuration
- Build graph from proximity relationships
- Detect communities, clusters, hierarchies
- Use graph properties to modulate forces (feedback loop)

**No single framework does this.** It requires combining:
- Particle simulation engine (Taichi, custom CUDA, or stripped-down HOOMD)
- Graph analysis library (NetworkX, graph-tool, or custom)
- Control layer that bridges them

#### 4. Multi-Resolution / LoD Transitions
This maps directly to HCP's LoD model:
- Fine-grained: individual particles with discovery forces
- Coarse-grained: clusters treated as single particles with aggregate properties
- Transition: detect cluster, compute center of mass, replace particles with single coarse particle
- Reverse transition: replace coarse particle with fine-grained constituent particles

**Frameworks:** LAMMPS (fix adapt, fix momentum), OpenMM (multi-scale approaches with custom forces)

---

## Part 6: Framework Recommendation Matrix

| Requirement | OpenMM | LAMMPS | HOOMD-blue | Taichi | Game Physics |
|-------------|--------|--------|------------|--------|-------------|
| Emergent structure (inference) | NO -- frozen topology | PARTIAL -- template-based reactive | PARTIAL -- MC exploration | YES -- custom kernels | NO -- constraint solver |
| Asymmetric forces (A->B != B->A) | NO -- Newton's 3rd law | NO -- Newton's 3rd law | NO -- Newton's 3rd law | YES -- custom force | NO |
| Per-particle identity | PARTIAL -- per-particle params (float) | YES -- custom atom properties | YES -- per-particle data | YES -- struct fields | NO -- shape-based |
| Force lookup tables | YES -- Discrete2DFunction | YES -- tabulated potentials | YES -- tabulated pair potentials | YES -- 2D field | NO |
| Dynamic bond formation | NO | YES -- fix bond/create, bond/react | YES -- MeshDynamicalBonding | YES -- custom | NO |
| Topology-aware forces | YES -- excellent | YES -- excellent | YES -- good | MANUAL -- you build it | NO |
| Concurrent systems | PARTIAL -- multiple Contexts | YES -- multiple simulations | YES -- multiple simulations | PARTIAL -- batched fields | YES -- islands |
| Structural analysis / queries | NO -- blind to results | LIMITED -- compute groups | LIMITED -- analysis tools | YES -- custom kernels | NO |
| GPU acceleration | YES -- CUDA/OpenCL | YES -- CUDA/Kokkos | YES -- CUDA/HIP | YES -- CUDA/Vulkan | YES -- CUDA (PhysX) |
| Project health (2026) | Active | Very active | Active | Maintenance mode | Active (Jolt) |

### The Uncomfortable Truth

**No existing framework does what HCP needs out of the box.** The requirements combine:
- Asymmetric forces (violates Newton's 3rd law -- all physics engines enforce this)
- Emergent topology (most engines assume topology is known)
- Per-particle semantic identity (physics engines care about mass and charge, not meaning)
- Structural analysis (physics engines compute forces, not structures)

The closest existing system is **Particle Life** -- but it's a demo/toy, not a production framework.

---

## Part 7: Recommended Architecture

### Option A: Custom Engine on Taichi (Maximum Control)

```
┌─────────────────────────────────────────────────┐
│              CUSTOM HCP ENGINE (Taichi)           │
│                                                   │
│  Spatial Hash:   ti.field grid, O(N) build        │
│  Force Lookup:   ti.field 2D table, asymmetric    │
│  Integration:    Symplectic Euler, custom kernel   │
│  Bond Tracking:  ti.field edge list, directed      │
│  Analysis:       Custom kernels for proximity,     │
│                  substructure, comparison           │
│  Concurrency:    Batched fields with system_id     │
│                                                   │
│  Total: ~2000-3000 lines of Taichi + Python       │
└─────────────────────────────────────────────────┘
```

**Pros:** Complete control over every operation. Asymmetric forces trivial. Directed bonds native. Analysis integrated.
**Cons:** Writing a physics engine from scratch. Taichi in maintenance mode. AOT limitations for deployment.

### Option B: LAMMPS Reactive MD (Closest Existing Framework)

```
┌─────────────────────────────────────────────────┐
│                LAMMPS + Extensions                │
│                                                   │
│  pair_style dpd:     Soft particle interactions    │
│  fix bond/create:    Distance-based bond formation │
│  fix bond/break:     Force-based bond breaking     │
│  compute group:      Structural analysis           │
│  Kokkos:             GPU acceleration              │
│                                                   │
│  PROBLEM: Symmetric forces, template-based react  │
│  PROBLEM: No asymmetric force without custom pair  │
└─────────────────────────────────────────────────┘
```

**Pros:** Battle-tested, massive community, GPU support, reactive topology.
**Cons:** Symmetric force assumption requires custom pair_style to override. Template-based reactions are the wrong model for open-ended emergence.

### Option C: Hybrid -- Discovery Engine + OpenMM Evaluator

```
┌─────────────────────────────────────────────────┐
│            DISCOVERY ENGINE (Custom/Taichi)        │
│                                                   │
│  Spatial hash + asymmetric force lookup            │
│  Particles settle into equilibrium                 │
│  Structure detection: identify stable clusters     │
│            ↓ detected structure                    │
│  ┌─────────────────────────────────────────────┐  │
│  │        EVALUATION ENGINE (OpenMM)            │  │
│  │                                              │  │
│  │  Build topology from detected bonds          │  │
│  │  Compute energy/forces for known structure   │  │
│  │  Score, compare, identify violations         │  │
│  └─────────────────────────────────────────────┘  │
│            ↓ evaluation results                    │
│  ┌─────────────────────────────────────────────┐  │
│  │        ANALYSIS ENGINE (Taichi/Custom)        │  │
│  │                                              │  │
│  │  Proximity queries                           │  │
│  │  Substructure detection                      │  │
│  │  Cross-system comparison                     │  │
│  └─────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

**Pros:** Each engine does what it's best at. OpenMM handles evaluation. Custom engine handles discovery. Taichi handles analysis.
**Cons:** Three engines. Data transfer between them. Integration complexity.

### Option D: Custom CUDA/Vulkan Compute (No Framework)

Write the compute shaders directly. The core algorithm for HCP inference is roughly:

```cuda
// Kernel 1: Spatial hash build (~50 lines)
__global__ void build_hash(Particle* particles, int* cell_count, int* cell_start, int N, float cell_size)

// Kernel 2: Force computation with asymmetric lookup (~80 lines)
__global__ void compute_forces(Particle* particles, float* force_table, int* cell_start,
                               int M, int N, float cutoff)

// Kernel 3: Integration (~20 lines)
__global__ void integrate(Particle* particles, int N, float dt)

// Kernel 4: Structure detection (~100 lines)
__global__ void detect_bonds(Particle* particles, Bond* bonds, int* cell_start,
                             float bond_threshold, int N)
```

**Total: ~300-500 lines of CUDA** for the core engine.
**Pros:** Maximum performance. No framework overhead. No dependency risk.
**Cons:** NVIDIA-only (CUDA) or write twice (CUDA + Vulkan). No automatic parallelization help.

---

## Part 8: Specific Answers to Questions

### Q1: What operations does particle physics simulation use that MD does NOT?

**Particle simulation has:**
- Runtime spatial hash rebuild every timestep (MD amortizes over 10-20 steps)
- Asymmetric force evaluation (MD enforces Newton's 3rd)
- Dynamic particle creation/destruction (MD has fixed particle count)
- Full N-body interaction (MD uses cutoff + long-range correction)
- Contact detection with geometric shapes (DEM -- MD uses point particles)

**MD has that particle simulation does NOT:**
- Long-range electrostatics (PME, Ewald sums, FFT-based)
- Constraint algorithms (SHAKE, SETTLE for fixed bond lengths)
- Thermostats / barostats (temperature/pressure control)
- Bonded force evaluation (bond, angle, dihedral, improper potentials)
- Implicit solvent models
- Free energy perturbation / thermodynamic integration

### Q2: How are interactions discovered in particle simulation?

Three methods, used depending on scale:

1. **Brute force all-pairs:** O(N^2). Every particle checks every other. Practical for N < 10K. Tiled for GPU shared memory (GPU Gems 3: 20 FLOP per pair).

2. **Spatial hash / uniform grid:** O(N). Hash particle positions into grid cells. Check 3^d neighboring cells (27 in 3D, 9 in 2D). Dominant method for uniform-density systems. Particle Life uses 32x32 grid for 65K particles.

3. **BVH tree (LBVH):** O(N log N). Build bounding volume hierarchy via Morton code sort. Query via tree traversal. HOOMD-blue uses this for size-asymmetric systems. Outperforms cell lists for sparse or non-uniform systems.

### Q3: What happens in MD when topology changes?

**In standard MD (OpenMM, GROMACS):** Topology CANNOT change. Bonds are frozen at Context creation. Adding a bond requires: destroy Context -> modify System -> create new Context -> recompile GPU kernels -> upload data. This takes 100ms-seconds.

**In reactive MD (LAMMPS):**
- `fix bond/break`: Every N steps, check all bonded pairs. If distance > threshold, remove bond from topology arrays, remove associated angles/dihedrals, rebuild neighbor list immediately.
- `fix bond/create`: Every N steps, check neighbor list for unbonded pairs. If distance < threshold AND type match, add bond to topology arrays, add associated angles if templates match, rebuild neighbor list.
- `fix bond/react`: Template-based. Match local topology against pre-reaction template. If match, swap to post-reaction template. Most powerful but most expensive.

**In HOOMD-blue:** MeshDynamicalBonding allows runtime bond changes for mesh surfaces. Less general than LAMMPS's approach.

### Q4: How does each handle emergent structure?

**MD:** Does not handle emergent structure. Structure must be predefined. Exception: self-assembly simulations where pair potentials (not bonds) drive particles into stable configurations (micelles, membranes). But the "structure" is never tracked -- you analyze it post-hoc.

**Particle simulation (discovery-first):** Structure is ALWAYS emergent. Particles settle into equilibrium configurations determined by force rules. The simulation has no concept of "structure" -- only particles and forces. Detecting structure requires a separate analysis pass:
- Cluster detection: identify groups of particles in stable proximity
- Graph construction: build adjacency graph from proximity relationships
- Community detection: identify subgroups within clusters
- Pattern matching: compare detected patterns against known templates

**Reactive MD (hybrid):** Structure partially emerges. Pair potentials drive particles together (emergence), then explicit bond formation triggers (guided emergence). Once bonds form, they're tracked as topology.

### Q5: Spatial data structures by method?

| Method | Primary Structure | Build Frequency | Query Pattern |
|--------|------------------|----------------|---------------|
| MD (OpenMM) | Verlet list from cell list | Every 10-20 steps | Direct iteration |
| MD (HOOMD) | Cell list OR Stencil OR LBVH tree | Adaptive (buffer-based) | Per-particle query |
| N-body | None (all-pairs) or Octree (Barnes-Hut) | Every step | Tree traversal |
| DEM / Granular | Uniform grid | Every step | 3^d neighbor cells |
| SPH | Uniform grid or compact hash | Every step | Kernel-weighted neighbor sum |
| Particle Life | Uniform grid (spatial hash) | Every step | 3^d neighbor cells |
| Game physics | Dynamic AABB tree (broadphase) | Incremental update | Tree query + GJK narrowphase |

---

## Part 9: Uncertainties and Flags

### Things I am confident about:
- The operational distinction between topology-first and discovery-first is real and fundamental
- Asymmetric forces are trivial in custom code but violated by all physics frameworks
- Spatial hashing is the right neighbor discovery method for HCP's use case
- Particle Life's computational model is the closest match to HCP inference
- No existing framework does all of what HCP needs

### Things I am uncertain about:
- **Performance at scale:** Can a Particle Life-style engine handle 1M particles with complex force tables? The 65K benchmark is with simple force rules. HCP's force tables are larger (hundreds of categories). NEEDS TESTING.
- **Structure detection latency:** How expensive is it to detect emergent structures from particle configurations? Cluster detection is O(N log N) with DBSCAN or similar, but the constants matter. NEEDS TESTING.
- **Concurrent system feasibility:** Batching thousands of independent particle systems into one GPU dispatch is theoretically sound but practically untested for this use case. Cross-system interference could be a problem if spatial hashes collide. NEEDS TESTING.
- **Transition cost:** Switching from discovery mode (no topology) to tracking mode (explicit bonds) mid-simulation -- what is the latency? How disruptive is it? No benchmark data found.
- **HOOMD-blue MC mode for HCP:** Monte Carlo exploration (propose random configurations, accept/reject by energy) might be a better model than dynamics for HCP inference. MC doesn't compute forces at all -- only energies. This could be significantly cheaper. NEEDS INVESTIGATION.

### Things that need decision, not research:
- Whether to build custom (maximum control, high dev cost) or adapt existing (compromise on asymmetric forces, lower dev cost)
- Whether Taichi's maintenance mode risk is acceptable for a foundational dependency
- Whether the three-engine hybrid (discovery + evaluation + analysis) is acceptable complexity or if a single custom engine is better

---

## Sources

### Frameworks
- [OpenMM Documentation](https://docs.openmm.org)
- [LAMMPS fix bond/react](https://docs.lammps.org/fix_bond_react.html)
- [LAMMPS fix bond/break](https://docs.lammps.org/fix_bond_break.html)
- [LAMMPS pair_style granular](https://docs.lammps.org/pair_granular.html)
- [LAMMPS pair_style dpd](https://docs.lammps.org/pair_dpd.html)
- [HOOMD-blue Documentation](https://hoomd-blue.readthedocs.io/en/latest/)
- [HOOMD-blue Neighbor Lists](https://hoomd-blue.readthedocs.io/en/v2.9.5/nlist.html)
- [HOOMD-blue GitHub](https://github.com/glotzerlab/hoomd-blue)
- [Taichi Lang](https://www.taichi-lang.org/)
- [Jolt Physics](https://github.com/jrouwe/JoltPhysics)

### GPU Compute References
- [NVIDIA GPU Gems 3: Fast N-Body Simulation with CUDA](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-31-fast-n-body-simulation-cuda)
- [NVIDIA GPU Gems 3: Rigid Body Simulation on GPUs](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-29-real-time-rigid-body-simulation-gpus)
- [NVIDIA GPU Gems 3: Broad-Phase Collision Detection with CUDA](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-32-broad-phase-collision-detection-cuda)
- [NVIDIA Particle Simulation using CUDA](https://developer.download.nvidia.com/assets/cuda/files/particles.pdf)
- [Bullet Collision Detection Pipeline](https://deepwiki.com/bulletphysics/bullet3/2.1-collision-detection-pipeline)

### Spatial Data Structures
- [Data Structures for SPH (Ihmsen et al., CGF 2011)](https://cg.informatik.uni-freiburg.de/publications/2011_CGF_dataStructuresSPH.pdf)
- [Maximizing Parallelism in BVH Construction (Karras, HPG 2012)](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf)
- [Survey on BVH (Meister et al., Eurographics 2021)](https://meistdan.github.io/publications/bvh_star/paper.pdf)

### Emergent Structure / Self-Assembly
- [Particle Life Simulation in WebGPU](https://lisyarus.github.io/blog/posts/particle-life-simulation-in-browser-using-webgpu.html)
- [Particle Life (hunar4321)](https://github.com/hunar4321/particle-life)
- [ALIEN Artificial Life Simulator](https://github.com/chrxh/alien)
- [Self-Assembly Simulation (Rapaport, Bar-Ilan)](https://faculty.biu.ac.il/~rapaport/research/sassem.html)
- [Molecular Simulations of Self-Assembly (PMC)](https://pmc.ncbi.nlm.nih.gov/articles/PMC5961611/)
- [Hierarchical CG Self-Assembly (PMC)](https://pmc.ncbi.nlm.nih.gov/articles/PMC9740473/)

### Performance Comparisons
- [MD Software Comparison (NVIDIA Forum)](https://forums.developer.nvidia.com/t/molecular-dynamics-software-comparison/8817)
- [GPU Performance Scaling for MD (ACM 2025)](https://dl.acm.org/doi/10.1145/3708035.3736080)
- [Jolt Physics Multicore Scaling](https://jrouwe.nl/jolt/JoltPhysicsMulticoreScaling.pdf)
- [Jolt GPU Discussion](https://github.com/jrouwe/JoltPhysics/discussions/501)

### Reactive / Hybrid Methods
- [Chemical Reactions in Classical MD (PMC)](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7608055/)
- [Reactivity in MD with Harmonic Force Fields (Nature 2024)](https://www.nature.com/articles/s41467-024-50793-0)
- [Hybrid Atomistic and CG MD](https://pubs.acs.org/doi/10.1021/acs.jpcb.6b02327)
