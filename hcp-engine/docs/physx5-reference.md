# PhysX 5.1.1 SDK Reference for HCP

SDK Location: `~/.o3de/3rdParty/packages/PhysX-5.1.1-rev4-linux/PhysX/physx/include/`

352 headers. This document covers the subset relevant to HCP cognitive physics.

---

## Design Constraints

Every pattern chosen here must support:
- **Concurrent multi-stream processing** — thousands of simultaneous flows over overlapping token webs
- **No architectural ceilings** — scale horizontally, no serial bottlenecks
- **Bootstrap disposability** — the DI will redesign orchestration from inside; physics primitives must be composable and reusable

---

## 1. PBD Particle System (Primary Substrate)

Tokens are particles. This is the core.

### Class Hierarchy
```
PxActor
  PxParticleSystem                    # Base particle system
    PxPBDParticleSystem               # Position-Based Dynamics solver

PxBase
  PxParticleBuffer                    # Base particle container (GPU buffers)
    PxParticleAndDiffuseBuffer        # + spray/bubble/foam
    PxParticleClothBuffer             # + springs/triangles (BROKEN - do not use)
    PxParticleRigidBuffer             # + shape-matched rigid bodies

PxBaseMaterial
  PxParticleMaterial                  # Base particle material
    PxPBDMaterial                     # PBD-specific (viscosity, cohesion, adhesion...)
    PxCustomMaterial                  # User-defined
```

### PxPBDParticleSystem (PxPBDParticleSystem.h)
Extends PxParticleSystem with PBD-specific wind and fluid settings.

| Method | Signature | HCP Use |
|--------|-----------|---------|
| setWind | `(const PxVec3& wind)` | N/A for token physics |
| setFluidBoundaryDensityScale | `(PxReal scale)` [0,1) | May control token boundary behavior |
| setFluidRestOffset | `(PxReal offset)` (0, particleContactOffset) | Rest spacing between token particles |
| setGridSizeX/Y/Z | `(PxU32 size)` | Spatial grid dimensions for neighbor detection |

### PxParticleSystem (PxParticleSystem.h) — Base Class
The workhorse. All particle management lives here.

| Method | Signature | HCP Use |
|--------|-----------|---------|
| setSolverIterationCounts | `(PxU32 posIters, PxU32 velIters=1)` [1,255] | Convergence accuracy for bond resolution |
| setSimulationFilterData | `(const PxFilterData& data)` | Collision filtering between token groups |
| setParticleFlag | `(PxParticleFlag::Enum, bool)` | Self-collision, rigid collision control |
| setMaxVelocity | `(PxReal maxVel)` | Prevent token explosion |
| setContactOffset | `(PxReal offset)` | Detection range for nearby tokens |
| setParticleContactOffset | `(PxReal offset)` | Particle-particle interaction distance |
| setSolidRestOffset | `(PxReal offset)` | Rest distance to solid bodies |
| **addRigidAttachment** | `(PxRigidActor* actor)` | **Lock converged cluster to rigid body** |
| removeRigidAttachment | `(PxRigidActor* actor)` | Unlock from rigid body |
| **createPhase** | `(PxParticleMaterial*, PxParticlePhaseFlags)` | **Create token group (stream/LoD isolation)** |
| **setParticleSystemCallback** | `(PxParticleSystemCallback* cb)` | **Register onAdvance for bond force injection** |
| setPeriodicBoundary | `(const PxVec3& boundary)` | Wrap-around (probably not needed) |
| **addParticleBuffer** | `(PxParticleBuffer* buf)` | **Add token stream to system** |
| removeParticleBuffer | `(PxParticleBuffer* buf)` | Remove token stream |
| getGpuParticleSystemIndex | `()` | GPU index (0xFFFFFFFF if not in scene) |
| enableCCD | `(bool)` | Continuous collision (for fast-moving tokens) |

### PxParticleSystemCallback (PxParticleSystem.h)
Three hooks into the simulation step. This is where bond forces are injected.

| Callback | When | HCP Use |
|----------|------|---------|
| **onBegin** | Dirty data uploaded to GPU | Pre-force setup, buffer initialization |
| **onAdvance** | During simulation step | **PRIMARY: inject bond forces via velocity modification** |
| **onPostSolve** | After simulation step | Convergence detection, state readback |

All callbacks receive `PxGpuMirroredPointer<PxGpuParticleSystem>` + `CUstream`.

### PxCustomParticleSystemSolverCallback (PxCustomParticleSystemSolverCallback.h)
Alternative callback for eCUSTOM solver type. More control.

| Callback | When | HCP Use |
|----------|------|---------|
| onBegin | After external forces, before spatial sort | Custom force injection (unsorted buffers) |
| onSolve | During iterative solve (called multiple times) | Per-iteration refinement |
| onFinalize | After all iterations | Final state processing |

All receive `PxGpuParticleSystem*` + `PxReal dt` + `CUstream`.

### PxParticleBuffer (PxParticleBuffer.h)
GPU buffer container. Each buffer = one token stream.

| Method | Returns | Notes |
|--------|---------|-------|
| getPositionInvMasses | `PxVec4*` (device) | xyz=position, w=1/mass |
| **getVelocities** | `PxVec4*` (device) | **xyz=velocity — modify this for bond forces** |
| getPhases | `PxU32*` (device) | Per-particle phase (group + flags) |
| setNbActiveParticles | `(PxU32 n)` | Dynamic particle count |
| getNbActiveParticles | `PxU32` | Current count |
| getMaxParticles | `PxU32` | Buffer capacity |
| getFlatListStartIndex | `PxU32` | Index in global list (valid after simulate) |
| raiseFlags | `(PxParticleBufferFlag::Enum)` | Mark data as dirty |
| setRigidAttachments | `(PxParticleRigidAttachment*, PxU32)` | Attach particles to rigid bodies |

### PxParticleRigidBuffer (PxParticleBuffer.h)
For shape-matched rigid body simulation within particle system.

| Method | Returns | HCP Use |
|--------|---------|---------|
| getRigidOffsets | `PxU32*` (device) | Start indices per rigid body |
| getRigidCoefficients | `PxReal*` (device) | Stiffness [0,1] per rigid |
| getRigidLocalPositions | `PxVec4*` (device) | Local positions within rigid |
| getRigidTranslations | `PxVec4*` (device) | World translations |
| getRigidRotations | `PxVec4*` (device) | World rotations (quaternions) |
| setNbRigids | `(PxU32 n)` | Set rigid count |

### PxParticleSpring (PxParticleBuffer.h)
Spring constraint between two particles. Potential dumbbell implementation.

| Field | Type | HCP Use |
|-------|------|---------|
| ind0 | PxU32 | First particle (token A) |
| ind1 | PxU32 | Second particle (token B) |
| length | PxReal | Rest length (bond distance) |
| stiffness | PxReal | Spring stiffness (bond strength from PBM count) |
| damping | PxReal | Damping factor |

### PxParticlePhaseFlag (PxParticleSystemFlag.h)
Per-particle behavior flags. 20-bit group + 4-bit flags.

| Flag | Value | HCP Use |
|------|-------|---------|
| eParticlePhaseGroupMask | bits [0-19] (~1M groups) | **Stream/LoD isolation** |
| eParticlePhaseSelfCollide | bit 20 | Interact within same group |
| eParticlePhaseSelfCollideFilter | bit 21 | Ignore close-range (rest pose) |
| eParticlePhaseFluid | bit 22 | Generate density constraints |

**Scale note:** 20-bit group ID = 1,048,576 possible groups. Sufficient for massive concurrent stream isolation.

### PxParticleSolverType (PxParticleSolverType.h)

| Solver | Flag | HCP Use |
|--------|------|---------|
| ePBD | 1<<0 | **Primary: position-based dynamics** |
| eFLIP | 1<<1 | FLIP fluid (unlikely) |
| eMPM | 1<<2 | Material Point Method (multi-material, possible future) |
| eCUSTOM | 1<<3 | **Custom solver callback — full control** |

### PxPBDMaterial (PxPBDMaterial.h)
Material properties for PBD particles.

| Property | Range | HCP Mapping |
|----------|-------|-------------|
| friction | [0, MAX_F32) | Inter-token friction |
| damping | [0, MAX_F32) | Velocity damping |
| adhesion | [0, MAX_F32) | **Token-to-token adhesion (bond attraction)** |
| adhesionRadiusScale | [0, MAX_F32) | **Adhesion fall-off distance** |
| gravityScale | (-MAX, MAX) | Gravity influence (probably 0 for tokens) |
| viscosity | [0, MAX_F32) | Fluid viscosity |
| cohesion | [0, MAX_F32) | **Intra-group cohesion (cluster formation)** |
| surfaceTension | [0, MAX_F32) | Surface tension |
| vorticityConfinement | [0, MAX_F32) | Vortex preservation |
| lift / drag | [0, MAX_F32) | Aerodynamic effects |
| CFLCoefficient | [1, MAX_F32) | Limit relative motion (stability) |
| particleFrictionScale | [0, MAX_F32) | Inter-particle friction scale |
| particleAdhesionScale | [0, MAX_F32) | **Inter-particle adhesion scale** |

### PxGpuParticleSystem (PxParticleGpu.h)
GPU-side particle system container. Exposed in callbacks.

| Member | Type | Notes |
|--------|------|-------|
| mUnsortedPositions_InvMass | float4* | Device: original order |
| mUnsortedVelocities | float4* | Device: original order |
| mUnsortedPhaseArray | PxU32* | Device: original order |
| mSortedPositions_InvMass | float4* | Device: spatially sorted |
| mSortedVelocities | float4* | Device: spatially sorted |
| mSortedPhaseArray | PxU32* | Device: spatially sorted |
| mUnsortedToSortedMapping | PxU32* | Index mapping |
| mSortedToUnsortedMapping | PxU32* | Reverse mapping |
| mParticleSelfCollisionCount | PxU32* | Per-particle neighbor count |
| mCollisionIndex | PxU32* | Sorted indices per neighbor |
| mParticleMaterials | PxsParticleMaterialData* | GPU materials |
| mCommonData | PxGpuParticleData | Grid/particle params |

**PxNeighborhoodIterator** — iterate neighbors of a particle:
```cpp
PxNeighborhoodIterator iter = gpuParticleSystem->getIterator(particleId);
PxU32 neighborId = iter.getNextIndex(); // 0xFFFFFFFF when done
```

**GPU Helper Functions (device-callable):**
- `PxGetGroup(phase)` — extract 20-bit group
- `PxGetFluid(phase)` — check fluid flag
- `PxGetSelfCollide(phase)` — check self-collision flag

---

## 2. FEM Soft Body (Error Correction / Inference)

Unresolved tokens → soft body → FEM deforms toward known patterns.

### Class Hierarchy
```
PxActor
  PxSoftBody                         # FEM soft body actor

PxBaseMaterial
  PxFEMMaterial                       # Base FEM material (Young's, Poisson's)
    PxFEMSoftBodyMaterial             # + damping, dampingScale
    PxFEMClothMaterial                # + thickness, bending

PxRefCounted
  PxTetrahedronMesh                   # Immutable tet mesh
  PxSoftBodyAuxData                   # Mass, rest pose, sim state
  PxSoftBodyMesh                      # Bundles collision + sim + aux
```

### PxFEMMaterial (PxFEMMaterial.h) — Key Parameters

| Property | Range | HCP Mapping |
|----------|-------|-------------|
| **Young's Modulus** | [0, MAX_F32) | **PBM bond strength → stiffness** |
| **Poisson's Ratio** | [0, 0.5) | **Volume preservation (NEVER 0.5)** |
| Dynamic Friction | [0, MAX_F32) | Sliding resistance |

### PxFEMSoftBodyMaterial (PxFEMSoftBodyMaterial.h)

| Property | Range | HCP Mapping |
|----------|-------|-------------|
| damping | [0, MAX_F32) | Velocity damping |
| dampingScale | [0, 1] default:1 | 0 = water-filled effects |

### PxFEMParameters (PxFEMParameter.h) — Solver Control

| Parameter | Default | HCP Use |
|-----------|---------|---------|
| velocityDamping | 0.05 | Per-timestep velocity reduction |
| settlingThreshold | 0.1 | Motion threshold for sleep candidacy |
| **sleepThreshold** | 0.05 | **Convergence detection: below this = resolved** |
| sleepDamping | 10.0 | Damping for sleep candidates |
| selfCollisionFilterDistance | 0.1 | Self-collision penetration tolerance |
| selfCollisionStressTolerance | 0.9 | Stress threshold to deactivate contact |

### PxSoftBody (PxSoftBody.h) — Key Methods

| Method | Signature | HCP Use |
|--------|-----------|---------|
| setParameter | `(PxFEMParameters)` | Set solver behavior |
| getParameter | `() → PxFEMParameters` | Read solver state |
| isSleeping | `() → bool` | **Convergence check: is token resolved?** |
| setWakeCounter | `(PxReal)` [0, MAX_F32) | Control sleep timer |
| getWakeCounter | `() → PxReal` | Check time to sleep |
| setSolverIterationCounts | `(PxU32 pos, PxU32 vel=1)` [1,255] | Accuracy vs speed |
| attachShape | `(PxShape&) → bool` | Collision mesh |
| attachSimulationMesh | `(PxTetrahedronMesh&, PxSoftBodyAuxData&) → bool` | Deformation mesh |
| readData / writeData | `(flags, PxBuffer&, flush)` | GPU-CPU transfer |
| getSimPositionInvMassCPU | `() → PxBuffer*` | CPU-side vertex positions |
| getSimVelocityInvMassCPU | `() → PxBuffer*` | CPU-side velocities |
| **addParticleAttachment** | `(system, buffer, particleId, tetId, barycentric) → handle` | **Attach token particle to soft body** |
| **addRigidAttachment** | `(actor, vertId, pose, constraint) → handle` | **Attach to rigid body (resolved)** |
| addSoftBodyAttachment | `(other, tetIdx0, bary0, tetIdx1, bary1) → handle` | Inter-soft-body bonds |

**Flags (PxSoftBodyFlag):**

| Flag | HCP Use |
|------|---------|
| eDISABLE_SELF_COLLISION | Optimization for single-token bodies |
| eCOMPUTE_STRESS_TENSOR | **Monitor deformation stress (resolution confidence)** |
| eENABLE_CCD | Fast-moving token handling |
| eKINEMATIC | Drive to target pose |
| ePARTIALLY_KINEMATIC | Partial kinematic control |

**Data Access (PxSoftBodyDataFlag):**

| Flag | Data |
|------|------|
| eTET_STRESS | Cauchy stress tensors (3x3 per tet) |
| eTET_STRESSCOEFF | Von Mises stress (scalar) |
| eTET_ROTATIONS | Tet orientations (quaternions) |
| eSIM_POSITION_INV_MASS | Simulation vertex positions |
| eSIM_VELOCITY_INV_MASS | Simulation vertex velocities |

### PxSoftBodyExt (extensions/PxSoftBodyExt.h) — Helpers

| Function | Signature | Notes |
|----------|-----------|-------|
| createSoftBodyFromMesh | `(mesh, transform, material, cuda, density, iters, femParams, scale)` | **Primary creation path** |
| createSoftBodyBox | `(transform, dims, material, cuda, ...)` | Quick test body |
| createSoftBodyMesh | `(cookingParams, surfaceMesh, numVoxels, ...)` | Cook from triangle mesh |
| updateMass | `(softBody, density, maxInvMassRatio)` | Recompute mass |
| transform | `(softBody, transform, scale)` | Transform body |
| commit | `(softBody, flags, flush)` | Upload to GPU |

---

## 3. Rigid Bodies & Aggregates (Resolved Tokens)

Converged particles lock into rigid bodies. Aggregates group them.

### PxRigidBody (PxRigidBody.h)

| Method | HCP Use |
|--------|---------|
| setMass(PxReal) | Token mass |
| addForce(vec3, mode, autowake) | Apply force to resolved token |
| setLinearVelocity / setAngularVelocity | Direct velocity control |
| setLinearDamping / setAngularDamping | Motion damping |
| setMaxLinearVelocity | Speed limit |
| isSleeping() | Stable state check |
| setSleepThreshold(PxReal) | Kinetic energy threshold for sleep |
| wakeUp() / putToSleep() | Manual sleep control |

**PxForceMode::Enum:**
- eFORCE — continuous force (mass-dependent)
- eIMPULSE — instantaneous impulse (mass-dependent)
- eVELOCITY_CHANGE — direct velocity change (mass-independent)
- eACCELERATION — continuous acceleration (mass-independent)

**PxRigidDynamicLockFlag** — lock motion axes:
- eLOCK_LINEAR_X/Y/Z, eLOCK_ANGULAR_X/Y/Z
- HCP: may lock tokens to 1D or 2D motion for stream alignment

### PxAggregate (PxAggregate.h)
Group actors for broadphase optimization. Token clusters.

| Method | HCP Use |
|--------|---------|
| addActor(PxActor&) | Add resolved token to cluster |
| removeActor(PxActor&) | Remove from cluster |
| getNbActors() | Cluster size |
| getActors(buffer, size, start) | Retrieve cluster members |
| getSelfCollision() | Whether tokens within cluster interact |

**PxAggregateType::Enum:**
- eGENERIC — mixed types
- eSTATIC — static only
- eKINEMATIC — kinematic only

**Scale note:** Aggregates appear as single entries in broadphase. Critical for performance when thousands of resolved clusters exist.

### PxRigidBodyExt (extensions/PxRigidBodyExt.h) — Force Helpers

| Function | Notes |
|----------|-------|
| addForceAtPos | World force at world position |
| addForceAtLocalPos | World force at local position |
| addLocalForceAtPos | Local force at world position |
| addLocalForceAtLocalPos | Local force at local position |
| getVelocityAtPos | Sample velocity at arbitrary point |
| computeVelocityDeltaFromImpulse | Predict velocity change |
| updateMassAndInertia | Recompute from shape densities |

---

## 4. Scene Management

### PxScene (PxScene.h) — Core Methods

| Method | HCP Use |
|--------|---------|
| **simulate(PxReal dt)** | **Run physics step** |
| **fetchResults(bool block)** | **Complete step, fire callbacks** |
| fetchResultsParticleSystem() | Sync particle data specifically |
| collide(dt) / advance() | Split-step (collision then dynamics) |
| addActor / removeActor | Single actor |
| addActors / removeActors | **Batch add/remove (critical for stream injection)** |
| addAggregate / removeAggregate | Cluster management |
| setGravity(PxVec3) | Global gravity (probably (0,0,0) for tokens) |
| setSimulationEventCallback | Contact/sleep/wake notifications |
| setBroadPhaseCallback | Out-of-bounds notifications |
| setContactModifyCallback | Modify contacts during solve |
| resetFiltering(actor) | Recompute collisions |
| shiftOrigin(PxVec3) | Reposition entire scene |
| getActiveActors(count) | Recently-moved actors (needs eENABLE_ACTIVE_ACTORS) |
| getNbParticleSystems / getParticleSystems | Query particle systems |
| getNbSoftBodies / getSoftBodies | Query soft bodies |
| copySoftBodyData / applySoftBodyData | GPU bulk transfer |
| applyParticleBufferData | GPU particle bulk transfer |

### PxSceneDesc (PxSceneDesc.h) — Scene Configuration

**Required fields:**
- `filterShader` — collision filter function (use PxDefaultSimulationFilterShader or custom)
- `cpuDispatcher` — task dispatcher for CPU work

**Key configuration:**

| Field | Default | HCP Setting |
|-------|---------|-------------|
| gravity | (0,0,0) | (0,0,0) — no gravity for tokens |
| broadPhaseType | ePABP | **eGPU for GPU scenes, ePABP for CPU** |
| solverType | ePGS | ePGS (default) or eTGS (more accurate) |
| frictionType | ePATCH | Default fine |
| flags | eENABLE_PCM | **Add: eENABLE_GPU_DYNAMICS, eENABLE_ACTIVE_ACTORS** |
| gpuMaxNumPartitions | 8 | [2-32] power of 2, higher = more parallel |
| gpuDynamicsConfig | defaults | Tune: maxParticleContacts, heapCapacity |
| solverBatchSize | 128 | Actors per solver task |
| wakeCounterResetValue | 0.4 | Sleep timer reset (20 frames at 0.02s) |

**PxSceneFlag (key flags):**

| Flag | Purpose |
|------|---------|
| eENABLE_GPU_DYNAMICS | **Enable GPU physics** |
| eENABLE_ACTIVE_ACTORS | Track which actors moved |
| eENABLE_CCD | Continuous collision detection |
| eENABLE_PCM | GJK-based contact (default, leave on) |
| eSUPPRESS_READBACK | **Skip GPU→CPU readback (performance)** |
| eFORCE_READBACK | Force readback (debugging) |
| eENABLE_STABILIZATION | Extra stabilization for stacking |

### PxBroadPhase (PxBroadPhase.h)

| Type | Best For | HCP Use |
|------|----------|---------|
| eSAP | Many sleeping objects | Maybe for stable token storage |
| eMBP | All moving, needs regions | Manual region control |
| eABP | Automatic regions | Good CPU default |
| **ePABP** | **Fastest CPU, auto regions** | **CPU fallback** |
| **eGPU** | **GPU, handles spikes** | **Primary for GPU scenes** |

**Factory functions:**
- `PxCreateBroadPhase(desc)` — standalone BP instance
- `PxCreateAABBManager(broadphase)` — high-level AABB management

### PxSimulationEventCallback (PxSimulationEventCallback.h)

| Callback | When | HCP Use |
|----------|------|---------|
| onContact | Shape pairs touch | Token collision detection |
| onTrigger | Trigger overlap | Boundary/zone detection |
| **onWake** | Actors wake up | Token re-activation |
| **onSleep** | Actors sleep | **Token convergence/stability** |
| onConstraintBreak | Constraint breaks | Bond breaking |
| onAdvance | Pose preview (during sim) | Early state access |

**PxContactPairFlag (key flags):**
- eACTOR_PAIR_HAS_FIRST_TOUCH — first contact between tokens
- eACTOR_PAIR_LOST_TOUCH — last contact lost

---

## 5. Constraints & Attachments (Bonds)

### PxConstraint (PxConstraint.h)
General-purpose constraint between two actors.

| Method | HCP Use |
|--------|---------|
| getActors / setActors | Get/set bonded tokens |
| setFlag(eBROKEN) | Check if bond broke |
| getForce(linear, angular) | Bond stress measurement |
| markDirty() | Notify scene of changes |

**PxConstraintFlag:**
- eCOLLISION_ENABLED — allow contacts between bonded tokens
- eGPU_COMPATIBLE — GPU-supported constraint type
- eDRIVE_LIMITS_ARE_FORCES — limits as forces (not impulses)

### PxConeLimitedConstraint (PxConeLimitedConstraint.h)
Cone-limited attachment for directional bonds.

| Field | Type | HCP Use |
|-------|------|---------|
| mAxis | PxVec3 | Bond direction |
| mAngle | PxReal (radians) | Angular freedom |
| mLowLimit / mHighLimit | PxReal | Distance limits |

### Px1DConstraint (PxConstraintDesc.h)
Low-level 1D constraint row.

**Spring parameters:**
- stiffness, damping — spring behavior
- restitution, velocityTarget — bounce behavior
- geometricError — position error to correct

**Solver hints:**
- eEQUALITY / eINEQUALITY — constraint type
- eACCELERATION1/2/3 — force limits

---

## 6. GPU Infrastructure

### PxCudaContextManager (cudamanager/PxCudaContextManager.h)
Thread-safe GPU context management.

**Setup:**
```cpp
int device = PxGetSuggestedCudaDeviceOrdinal(errorCallback);
PxCudaContextManagerDesc desc;
desc.ctx = NULL; // create new
PxCudaContextManager* cuda = PxCreateCudaContextManager(foundation, desc);
```

**Memory Management (templated):**
- `allocDeviceBuffer<T>(ptr, count)` — GPU memory
- `freeDeviceBuffer<T>(ptr)`
- `allocPinnedHostBuffer<T>(ptr, count)` — Pinned host (zero-copy GPU access)
- `freePinnedHostBuffer<T>(ptr)`

**Data Transfer (templated):**
- `copyDToH<T>(host, device, count)` — GPU→CPU
- `copyHToD<T>(device, host, count)` — CPU→GPU
- `copyDToDAsync<T>(dst, src, count, stream)` — GPU→GPU async
- `copyDToHAsync<T>(host, device, count, stream)` — GPU→CPU async
- `copyHToDAsync<T>(device, host, count, stream)` — CPU→GPU async

**Thread Safety:**
```cpp
PxScopedCudaLock lock(*cudaContextManager); // RAII lock
```

**Device Queries:**
- `getDeviceTotalMemBytes()` — GPU memory
- `getMultiprocessorCount()` — SM count
- `getDeviceName()` — GPU name
- `supportsArchSM*()` — Compute capability checks

### Convenience Macros
```cpp
PX_DEVICE_ALLOC(cudaMgr, ptr, count)
PX_DEVICE_FREE(cudaMgr, ptr)
PX_PINNED_HOST_ALLOC(cudaMgr, ptr, count)
PX_PINNED_HOST_FREE(cudaMgr, ptr)
```

---

## 7. Extensions (Helper Functions)

### PxParticleExt (extensions/PxParticleExt.h)
Buffer creation helpers. All require CudaContextManager.

| Factory Function | Creates | Notes |
|-----------------|---------|-------|
| PxCreateAndPopulateParticleBuffer | PxParticleBuffer* | Basic particle buffer |
| PxCreateAndPopulateParticleAndDiffuseBuffer | PxParticleAndDiffuseBuffer* | + diffuse particles |
| PxCreateAndPopulateParticleRigidBuffer | PxParticleRigidBuffer* | + shape-matched rigids |
| PxCreateAndPopulateParticleClothBuffer | PxParticleClothBuffer* | **BROKEN — do not use** |
| PxCreateParticleClothPreProcessor | PxParticleClothPreProcessor* | Spring partitioning |
| PxCreateParticleAttachmentBuffer | PxParticleAttachmentBuffer* | Particle↔rigid attachments |

**Buffer Descriptors (ExtGpu namespace):**
```cpp
PxParticleBufferDesc {
    PxVec4* positions;      // xyz + unused w
    PxVec4* velocities;     // xyz + unused w
    PxU32*  phases;         // group + flags
    PxU32 numActiveParticles, maxParticles;
};
```

### PxSimpleFactory (extensions/PxSimpleFactory.h)
Quick actor creation.

| Function | Creates |
|----------|---------|
| PxCreateDynamic | PxRigidDynamic* from geometry+material |
| PxCreateKinematic | PxRigidDynamic* (kinematic) |
| PxCreateStatic | PxRigidStatic* |
| PxCloneDynamic | Clone existing dynamic |

### PxDefaultSimulationFilterShader (extensions/PxDefaultSimulationFilterShader.h)
Group-based collision filtering.

| Function | Purpose |
|----------|---------|
| PxSetGroupCollisionFlag(g1, g2, bool) | Enable/disable group pair collision |
| PxGetGroupCollisionFlag(g1, g2) | Query group pair |
| PxSetGroup(actor, group) | Assign actor to group [0-31] |
| PxGetGroup(actor) | Get actor's group |

### PxSceneQueryExt (extensions/PxSceneQueryExt.h)
Scene queries (raycast, sweep, overlap).

| Function | Variants |
|----------|----------|
| raycastAny/Single/Multiple | Quick, closest, all hits |
| sweepAny/Single/Multiple | Shape sweep |
| overlapAny/Multiple | Shape overlap test |

---

## 8. Known Gotchas

| Issue | Details |
|-------|---------|
| `PxGetFoundation()` is module-local | Must call `PxSetFoundationInstance()` when using PhysX from a shared library/Gem |
| GPU particle pointers not CPU-accessible | Use `copyDToH` after `fetchResults` — never dereference device pointers on CPU |
| No `addForce()` for particles | Modify velocity buffers directly in onAdvance callback |
| `PxParticleClothBuffer` is BROKEN | Do not use. Plain PxParticleBuffer with PxParticleSpring works |
| Poisson's ratio cannot be 0.5 | Numerical instability at incompressible limit — use 0.45 max |
| `eSUPPRESS_READBACK` disables CPU queries | Cannot read positions from CPU. Use for production, disable for debugging |
| Phase group is 20-bit | Max ~1M groups. Sufficient but finite |
| `PxAggregate` broadphase | Entire aggregate = one AABB. Too-large aggregates defeat broadphase purpose |

---

## 9. HCP Operation → PhysX Mapping Summary

| HCP Operation | PhysX Implementation |
|---------------|---------------------|
| Token as particle | PxPBDParticleSystem + PxParticleBuffer |
| Bond force injection | onAdvance callback → modify velocity buffers |
| Stream isolation | PxParticlePhaseFlag groups (20-bit, ~1M) |
| Multiple concurrent streams | Multiple PxParticleBuffer per system |
| Convergence detection | Sleep monitoring (PxFEMParameters.sleepThreshold) |
| Resolved token → rigid body | PxParticleSystem.addRigidAttachment → PxAggregate |
| LoD promotion | Aggregate-as-particle at next level |
| Error correction (FEM) | PxSoftBody + PxFEMSoftBodyMaterial (Young's = bond strength) |
| Bond stress monitoring | PxSoftBody.eCOMPUTE_STRESS_TENSOR / PxConstraint.getForce |
| Token cluster grouping | PxAggregate (self-collision configurable) |
| Dumbbell bonds | PxParticleSpring (ind0, ind1, length, stiffness, damping) |
| Custom solver logic | PxCustomParticleSystemSolverCallback (eCUSTOM solver) |
| GPU acceleration | PxSceneFlag::eENABLE_GPU_DYNAMICS + PxBroadPhaseType::eGPU |
| CPU fallback | Same API, PxBroadPhaseType::ePABP, no CUDA context needed |
