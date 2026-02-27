# PhysX 5.1.1 — Mathematical Reference for HCP Engine

## Purpose
This document describes the PhysX primitives the HCP engine uses and the math behind them. It is intended for someone who wants to understand the physics and mathematics without reading source code.

Last updated: 2026-02-27

---

## What the HCP Engine Does with Physics

The engine resolves raw byte streams into structured tokens using GPU-accelerated particle physics. Instead of hash tables and string comparison, it places data into a physics simulation where particles interact through broadphase collision detection. Matching is a physics operation — identical values attract, different values don't interact.

Two proven phases:
- **Phase 1 (byte→char):** Each input byte settles onto its matching codepoint particle (200/200, 100%, 119ms)
- **Phase 2 (char→word):** Character runs settle onto matching vocabulary words (34/35, 97.1%, 225ms)

Both use the same fundamental mechanic: **superposition resolution** — candidates exist simultaneously, physics collapses the superposition to the correct match.

---

## Core Primitive: PBD Particle System

**Position Based Dynamics (PBD)** is the solver algorithm. Unlike force-based physics (F=ma → acceleration → velocity → position), PBD works directly on positions:

1. **Predict** new positions from current velocity + external forces (gravity)
2. **Detect** constraint violations (particles overlapping)
3. **Project** constraints — move particles to satisfy constraints directly
4. **Update** velocity from position change: `v = (x_new - x_old) / dt`

This is iterative. Each simulation step runs multiple solver iterations (default: 4 position, 1 velocity) to converge on a stable configuration.

### Why PBD, not force-based?
- Unconditionally stable (no exploding simulations from stiff springs)
- Positions are the primary state (we care about WHERE particles end up, not forces)
- GPU-friendly — each constraint projection is local to 2 particles

### PhysX Implementation
- `PxPBDParticleSystem` — the GPU-accelerated PBD solver
- `PxParticleBuffer` — GPU memory holding particle state
- Each particle stores: position (x,y,z), inverse mass (w), velocity (x,y,z), phase (uint32), volume

---

## Spatial Hash Broadphase

Before the solver can project constraints, it needs to know which particles are near each other. Testing all pairs is O(N²). The **spatial hash** reduces this to approximately O(N).

### How it works

1. **Grid definition:** 3D grid with uniform cell size = `particleContactDistance`
   - Domain size per axis: `gridSize × particleContactDistance`
   - Total cells: `gridSizeX × gridSizeY × gridSizeZ`

2. **Hash particles into cells:** Each particle's position maps to a cell index:
   ```
   cellX = floor(pos.x / cellSize)
   cellY = floor(pos.y / cellSize)
   cellZ = floor(pos.z / cellSize)
   ```

3. **Sort particles by cell index:** GPU radix sort reorders particles so all particles in the same cell are contiguous in memory

4. **Generate pairs:** For each particle, check its cell and 26 neighboring cells. Only particles within `2 × particleContactOffset` distance generate contact pairs.

### HCP Exploitation of Spatial Hash

The Z-axis encodes character identity: `Z = ascii_value × Z_SCALE` where `Z_SCALE = 10`.

Two particles with different character values have Z-distance ≥ 10. With `particleContactOffset = 0.4`, interaction distance = `2 × 0.4 = 0.8`. Since `10 >> 0.8`, different characters are NEVER in the same hash cell on Z. The spatial hash prunes non-matching characters before any solver work.

This is why matching works: only particles with the SAME character identity (same Z value) can ever generate contact pairs. The physics does the comparison implicitly.

---

## Phase Groups — Collision Filtering

Each particle carries a 32-bit phase value:

```
Bits [0-19]:  Group ID (20 bits, ~1M groups)
Bit  [20]:    eSelfCollide — interact with particles of the SAME group
Bit  [21]:    eSelfCollideFilter — rest-pose proximity filter (cloth feature, NOT used by HCP)
Bit  [22]:    eFluid — generate fluid density constraints
Bits [23-31]: Reserved
```

### Collision rule
Two particles interact if and only if:
1. Both have `eSelfCollide` (bit 20) set
2. Both have the **same** 20-bit group ID (exact equality, full 20-bit comparison)

Different group IDs = **zero interaction**. No broadphase pairs generated. No solver work.

### How it works mathematically
```
groupA = phaseA & 0x000FFFFF
groupB = phaseB & 0x000FFFFF
interact = (groupA == groupB) && (phaseA & eSelfCollide) && (phaseB & eSelfCollide)
```

The group ID extraction is a bitwise AND with the mask. The comparison is exact equality.

### HCP usage

**Phase 1 & 2 (current):** All particles in group 0 with eSelfCollide. Single group, all particles interact. Z-axis discrimination handles matching.

**Tiered resolution (next):** Vocabulary split into frequency tiers, each at a different group ID:
- Tier 1 vocab: group = 1
- Tier 2 vocab: group = 2
- Tier 3 vocab: group = 3
- Graveyard (resolved): group = 0

Stream particles start at group 1 (interact with tier 1 vocab only). Survivors shift to group 2 between simulate() calls. Resolved particles shift to group 0 (no other particles in group 0 = zero cost). The phase group is the gate controlling which vocabulary tier the stream currently resolves against.

All tiers coexist at identical spatial positions — different group IDs prevent any cross-tier interaction. Transparent overlay.

---

## Particle Properties

### Position and Inverse Mass
Stored as `PxVec4(x, y, z, invMass)`:
- `invMass = 0.0` → **static** particle (immovable, infinite mass). Used for reference/vocabulary particles.
- `invMass = 1.0` → **dynamic** particle (mass = 1.0). Used for input/stream particles.
- Higher invMass = lighter particle (more responsive to forces)

The inverse mass formulation means static particles participate in collision detection but are never moved by the solver.

### Velocity
Stored as `PxVec4(vx, vy, vz, 0)`. Updated by the solver each step:
```
velocity = (newPosition - oldPosition) / dt
```

### Contact Offsets — Interaction Distances

Four offset parameters control when particles interact and where they come to rest:

| Parameter | Meaning | Math |
|-----------|---------|------|
| `particleContactOffset` | Two particles interact if distance < `2 × particleContactOffset` | Defines the spatial hash cell size and broadphase range |
| `solidRestOffset` | Equilibrium distance between solid particles = `2 × solidRestOffset` | Where the solver stops pushing particles apart |
| `contactOffset` | Particle-rigid interaction distance = sum of both contactOffsets | For particle-geometry collisions |
| `restOffset` | Particle-rigid rest distance = sum of both restOffsets | Equilibrium for particle-geometry |

**HCP configuration:**
```
particleContactOffset = 0.4    → interaction range = 0.8
solidRestOffset = 0.3          → rest distance = 0.6
contactOffset = 0.4
restOffset = 0.3
```

With Z_SCALE = 10, characters one ASCII value apart are at Z-distance = 10. Since 10 >> 0.8 (interaction range), only identical characters interact.

---

## Gravity and Settlement

Scene gravity: `(0, -9.81, 0)` — standard downward.

Each particle's effective gravity = `sceneGravity × material.gravityScale`.

### Settlement mechanic (Phase 1)

Static reference particles (codepoints) are positioned at Y = 0. Dynamic input particles start at Y = offset above. Gravity pulls dynamic particles downward. When a dynamic particle's Z matches a static particle's Z, they enter contact range and the solver resolves them to the rest distance. The dynamic particle "settles" onto the matching reference.

Non-matching particles (different Z) never enter contact range — they pass through each other (no interaction because Z-distance exceeds contact range).

**Settlement detection:** After simulation steps, check each dynamic particle's velocity: `|Vy| < threshold` indicates it has settled (stopped moving). The static particle it settled onto identifies the match.

---

## Simulation Pipeline

Each `simulate(dt)` call executes:

```
1. Pre-integrate external forces (gravity) into velocity
   v_predicted = v + gravity * gravityScale * dt

2. Predict positions
   x_predicted = x + v_predicted * dt

3. Reorder particles by spatial hash index

4. Build neighbor lists (broadphase)
   - Hash predicted positions into grid cells
   - For each particle, scan own cell + 26 neighbors
   - Generate pairs where distance < 2 × particleContactOffset
   - Filter by phase group (same group + eSelfCollide required)

5. Iterative solver (repeat N times, default N=4)
   For each constraint (contact pair):
   - Compute penetration depth: d = 2×solidRestOffset - distance
   - If d > 0 (particles overlap):
     - Compute correction: Δx = d × (invMassA / (invMassA + invMassB)) × normal
     - Move particle A by +Δx, particle B by -Δx
     - Static particles (invMass=0) don't move — full correction on dynamic particle

6. Velocity update
   v_new = (x_corrected - x_old) / dt

7. Apply damping
   v_final = v_new × (1 - damping × dt)
```

### Per-step cost
The expensive part is step 4 (broadphase pair generation). Steps 5-7 only operate on generated pairs. With Z-axis discrimination eliminating most pairs, the solver work is minimal.

---

## GPU Memory and Data Transfer

### Buffer layout
All particle data lives on the GPU device:
- `getPositionInvMasses()` → device pointer to `PxVec4[N]`
- `getVelocities()` → device pointer to `PxVec4[N]`
- `getPhases()` → device pointer to `PxU32[N]`

### Host ↔ Device transfers
```
PxScopedCudaLock lock(cudaContextManager);  // acquire GPU context
cudaContextManager->copyHToD(devPtr, hostPtr, count);  // host → device
cudaContextManager->copyDToH(hostPtr, devPtr, count);  // device → host
```

Pinned (page-locked) host memory for faster transfers:
```
PxVec4* hostBuf = cuda->allocPinnedHostBuffer<PxVec4>(count);
// ... populate on CPU ...
cuda->copyHToD(devBuf, hostBuf, count);
cuda->freePinnedHostBuffer(hostBuf);
```

### Dirty flags
After modifying data on the host, notify PhysX which arrays changed:
```
buffer->raiseFlags(PxParticleBufferFlag::eUPDATE_POSITION);  // positions changed
buffer->raiseFlags(PxParticleBufferFlag::eUPDATE_VELOCITY);  // velocities changed
buffer->raiseFlags(PxParticleBufferFlag::eUPDATE_PHASE);     // phase groups changed
```

No dirty-range support — flags are per-array-type, not per-index-range. But the arrays are already on the device, so "update" means PhysX re-reads values it already has access to.

For the tier cascade, updating 200 particles' phase groups between simulate() calls costs ~800 bytes of device writes. Negligible.

---

## Scene Architecture

### One CUDA context, multiple scenes
```
PxCudaContextManager (one, shared across all scenes)
├── PxScene: byte→char space (Phase 1)
│   └── PxPBDParticleSystem (codepoints + input streams)
├── PxScene: char→word space (Phase 2)
│   └── PxPBDParticleSystem per resolution chamber
├── PxScene: word→phrase space (future)
└── PxScene: [additional spaces as needed]
```

Each scene has independent:
- Gravity (mutable at runtime)
- Broadphase type (GPU, set at creation, immutable)
- GPU dynamics memory config (~80MB baseline per GPU-enabled scene)
- Solver parameters

Scenes share the CUDA context. GPU work serializes through it but simulate() calls are independent.

### Per-scene configuration (from HCP engine)
```cpp
PxSceneDesc sceneDesc(physics->getTolerancesScale());
sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
sceneDesc.cudaContextManager = cudaContextManager;   // shared
sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;         // persistent contact manifold
sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
```

### Per-PBD-system configuration
```cpp
PxPBDParticleSystem* sys = physics->createPBDParticleSystem(cudaCtx, 96);
sys->setRestOffset(0.3f);
sys->setContactOffset(0.4f);
sys->setParticleContactOffset(0.4f);
sys->setSolidRestOffset(0.3f);
sys->setSolverIterationCounts(4, 1);  // 4 position iterations, 1 velocity
```

The `96` parameter is the number of GPU warps allocated to this system.

---

## Custom Solver Callbacks

PhysX provides three callback hooks for custom force injection:

```
onBegin()    — after external force pre-integration, BEFORE spatial hash sort
onSolve()    — during iterative solve (called multiple times per frame)
onFinalize() — after all solver iterations complete, after integration
```

These execute on the GPU via CUDA streams. Custom kernels can read/write particle state directly on device memory.

**HCP usage:** The `onBegin` callback provides an opportunity to inject custom forces (bond forces, attraction/repulsion) before the solver runs. The proven D→H→compute→H→D pattern from `HCPDetectionScene` modifies velocity buffers on the host between simulate calls, but for higher performance, custom CUDA kernels via these callbacks avoid the round-trip.

---

## PhysX Primitives → HCP Operations

| HCP Operation | PhysX Primitive | Math |
|---------------|----------------|------|
| Byte identity (Phase 1) | Z = ascii × 10, gravity settlement | Z-distance discrimination + PBD contact resolution |
| Word identity (Phase 2) | N particles per word, same Z encoding | N-element AND: all characters must be in contact simultaneously |
| Static references | invMass = 0 particles | Infinite mass — immovable, but participate in collision detection |
| Dynamic input | invMass = 1 particles | Unit mass — pulled by gravity, pushed by contacts |
| Match detection | Broadphase pair generation + position readback | Spatial hash prunes by Z, host confirms all-character match |
| Settlement check | Velocity magnitude < threshold | |Vy| < 0.1 → particle has stopped moving → match found |
| Tier isolation | Different phase group IDs | 20-bit exact equality: different ID = zero interaction |
| Tier cascade | Phase group reassignment | Write new group ID to device phase array between simulate() calls |
| Resolved/consumed | Phase group → 0 (graveyard) | Group 0 has no other particles → zero broadphase cost |
| Stream isolation | Y-offset spatial separation | Particles at Y=0 vs Y=100 exceed contact range → no interaction |

---

## Hardware

### Development floor
- NVIDIA GTX 1070: 8GB VRAM, 15 SMs, Compute Capability 6.1
- Per GPU-enabled scene: ~80MB baseline allocation
- Routinely handles 100K+ PBD particles
- Phase 1 (400 particles): 119ms for 60 steps
- Phase 2 (2050 particles): 225ms for 120 steps

### Scaling
Architecture decisions target correctness and scalability, not 1070 performance. More SMs = more parallel evaluation per simulate() call. More VRAM = more concurrent chambers/streams. The code doesn't change — the hardware fills the parallelization trees faster.

---

## Files in Use

### PhysX SDK Headers (at `~/.o3de/3rdParty/packages/PhysX-5.1.1-rev4-linux/PhysX/physx/include/`)
| Header | What it provides |
|--------|-----------------|
| `PxPhysicsAPI.h` | Master include — pulls in all PhysX types |
| `PxParticleGpu.h` | GPU particle system data structures, grid parameters |
| `gpu/PxGpu.h` | GPU utilities, context management |
| `PxPBDParticleSystem.h` | PBD-specific particle system (wind, fluid boundary, grid size) |
| `PxParticleSystem.h` | Base particle system (offsets, solver iterations, phase creation) |
| `PxParticleBuffer.h` | Particle buffer (positions, velocities, phases, dirty flags) |
| `PxParticleBufferFlag.h` | Buffer update flags (eUPDATE_POSITION, eUPDATE_PHASE, etc.) |
| `PxParticleSystemFlag.h` | Phase group flags (eSelfCollide, group mask) |
| `PxScene.h` | Scene (simulate, fetchResults, gravity, actors) |
| `PxSceneDesc.h` | Scene creation descriptor (GPU config, broadphase type) |
| `PxParticleMaterial.h` | Material properties (damping, friction, gravity scale) |
| `PxCustomParticleSystemSolverCallback.h` | Custom solver hooks (onBegin, onSolve, onFinalize) |
| `cudamanager/PxCudaContextManager.h` | CUDA context, H↔D transfers, pinned memory |
| `extensions/PxDefaultSimulationFilterShader.h` | Default collision filter function |

### HCP Engine Source (at `hcp-engine/Gem/Source/`)
| File | Purpose |
|------|---------|
| `HCPParticlePipeline.cpp/.h` | Foundation, CUDA context, scene creation, material setup |
| `HCPSuperpositionTrial.cpp/.h` | Phase 1 byte→char superposition (proven) |
| `HCPWordSuperpositionTrial.cpp/.h` | Phase 2 char→word superposition (proven) |
| `HCPResolutionChamber.cpp/.h` | Tiered resolution chamber (in development) |
| `HCPDetectionScene.cpp/.h` | Force injection pattern (D→H→compute→H→D) |
| `HCPEngineSystemComponent.cpp/.h` | Console commands, O3DE integration |
