# PhysX Answers for Engine Specialist

From the PhysX specialist, based on full SDK survey of the installed PhysX 5.1.1 headers.

---

## 1. Bond Force Injection — Proper GPU Mechanism

The D→H→compute→H→D workaround is unnecessary. PhysX has two GPU-native callback paths:

### Option A: PxParticleSystemCallback (PBD solver)
```cpp
class MyCallback : public PxParticleSystemCallback {
    void onBegin(const PxGpuMirroredPointer<PxGpuParticleSystem>& gpu, CUstream stream) override;
    void onAdvance(const PxGpuMirroredPointer<PxGpuParticleSystem>& gpu, CUstream stream) override;
    void onPostSolve(const PxGpuMirroredPointer<PxGpuParticleSystem>& gpu, CUstream stream) override;
};

particleSystem->setParticleSystemCallback(&myCallback);
```

In `onAdvance`, you get `PxGpuParticleSystem*` which exposes GPU buffers directly:
- `mSortedVelocities` (float4*) — **modify this on GPU, no D→H round trip**
- `mSortedPositions_InvMass` (float4*)
- `mSortedPhaseArray` (PxU32*)
- `mCollisionIndex` + `PxNeighborhoodIterator` — iterate neighbors per particle

Write a CUDA kernel, launch it on the provided `CUstream`, modify velocities in-place on GPU. No copies.

### Option B: PxCustomParticleSystemSolverCallback (eCUSTOM solver)
```cpp
class MyCustomSolver : public PxCustomParticleSystemSolverCallback {
    void onBegin(PxGpuParticleSystem* gpu, PxReal dt, CUstream stream) override;
    void onSolve(PxGpuParticleSystem* gpu, PxReal dt, CUstream stream) override;  // called multiple times
    void onFinalize(PxGpuParticleSystem* gpu, PxReal dt, CUstream stream) override;
};
```

More control — `onSolve` is called per iteration, so you can inject forces at each solver step. Requires `PxParticleSolverType::eCUSTOM` when creating the system.

### Recommendation
**Option A for now.** `onAdvance` in the PBD solver is the simplest path. Write a CUDA kernel that:
1. For each particle, iterate neighbors via `mCollisionIndex`
2. Look up bond strength from your bond table (also in GPU memory)
3. Modify `mSortedVelocities` directly

All on GPU. Zero copies. The bond table (5,409 entries for char→word, 1,976 for byte→char) fits trivially in GPU constant memory or a device buffer.

### Also: PxParticleSpring
For explicit pair bonds, `PxParticleSpring` is a built-in constraint:
```cpp
struct PxParticleSpring {
    PxU32 ind0;       // particle A
    PxU32 ind1;       // particle B
    PxReal length;    // rest distance
    PxReal stiffness; // bond strength (from PBM count)
    PxReal damping;   // damping factor
};
```
These run natively in the PBD solver — no custom kernel needed. If the bond relationships are known before simulation, springs are the zero-effort path.

---

## 2. Cluster → Rigid Body Promotion

### Step 1: Freeze the cluster
Two options:

**Option A: PxParticleRigidBuffer (shape-matched rigid)**
The particle system has a built-in rigid body mode. Particles within a rigid group maintain their relative positions.
```cpp
// In PxParticleRigidBuffer:
getRigidOffsets()        // start index per rigid body
getRigidCoefficients()   // stiffness [0,1] — set to 1.0 for fully rigid
getRigidLocalPositions() // relative positions within rigid
getRigidTranslations()   // world position
getRigidRotations()      // world orientation (quaternion)
setNbRigids(n)           // number of rigid groups
```
This keeps the token as particles but enforces rigid shape. The particles still live in the particle system. **Best for staying within the particle pipeline.**

**Option B: True PxRigidDynamic**
Create an actual rigid body actor:
```cpp
PxRigidDynamic* rigid = PxCreateDynamic(physics, transform, geometry, material, density);
// or via PxPhysics::createRigidDynamic()
scene->addActor(*rigid);
```
Then remove the cluster particles from the particle buffer (decohere to superposition).

### Step 2: LoD promotion
The rigid body (whether shape-matched particles or PxRigidDynamic) needs to participate at the next level.

**For PxParticleRigidBuffer approach:** The rigid group IS still particles. At the next LoD level, the group's centroid position represents the token. You can track rigid group centroids via `getRigidTranslations()` and use them as input positions for the next-level particle system.

**For PxRigidDynamic approach:** The rigid body can be:
- Added to a `PxAggregate` for broadphase optimization
- Used as an anchor for `addRigidAttachment()` from the next-level particle system
- Its position/velocity drives a particle in the next-level system

### Recommendation
**PxParticleRigidBuffer for the common case** — stays in the particle pipeline, no actor creation overhead, GPU-native. Use `rigidCoefficients` ramp: start soft (0.1), increase as convergence is detected, lock at 1.0 when fully resolved.

**PxRigidDynamic for long-lived resolved tokens** that will be referenced many times — worth the promotion cost.

---

## 3. Bifurcation into Dumbbells

Once a token is a rigid body and needs to split into twin halves:

### Option A: PxParticleSpring (simplest)
If the twins are particles:
```cpp
PxParticleSpring dumbbell;
dumbbell.ind0 = twinA_index;
dumbbell.ind1 = twinB_index;
dumbbell.length = bond_rest_length;
dumbbell.stiffness = bond_strength;  // from PBM count
dumbbell.damping = 0.5f;
```
Native PBD spring. Two particles connected. That IS a dumbbell. Simplest possible implementation.

### Option B: PxD6Joint (if using rigid bodies)
If twins are PxRigidDynamic:
```cpp
PxD6Joint* joint = PxD6JointCreate(physics,
    twinA, localFrameA,
    twinB, localFrameB);
// Lock all axes except the bond axis:
joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLIMITED);  // bond axis
joint->setLinearLimit(PxD6Axis::eZ, PxJointLinearLimitPair(lowLimit, highLimit));
```
D6 is the most configurable joint — lock/free/limit each axis independently.

### Option C: PxFixedJoint (rigid dumbbell)
If the twins should have zero relative motion:
```cpp
PxFixedJoint* joint = PxFixedJointCreate(physics,
    twinA, localFrameA,
    twinB, localFrameB);
```
No freedom of motion. Rigid connection. Appropriate if dumbbells are stiff.

### Recommendation
**PxParticleSpring if twins are particles** (most likely for the PBM assembly pipeline). The spring IS the bond. Stiffness IS the bond count.

**PxD6Joint if twins are rigid bodies** and you need configurable constraint axes. This gives you the direction and spin characteristics needed for grammatical forces later.

---

## 4. Twin Binding — Dynamic Proximity Attachment

Each twin half needs to find and bind to the nearest neighbor's twin during simulation.

### Detection: Use the particle neighborhood system
In `onAdvance` or `onPostSolve`:
```cpp
PxNeighborhoodIterator iter = gpuParticleSystem->getIterator(particleId);
PxU32 neighborId;
while ((neighborId = iter.getNextIndex()) != 0xFFFFFFFF) {
    // Check if neighbor is a matching twin (by phase/identity)
    // If match found, create bond
}
```

### Attachment options:

**Between particles:** Create a `PxParticleSpring` dynamically.
Springs are stored in the particle buffer — update the spring list between sim steps:
1. Detect matching twins via neighborhood iterator
2. Add spring (ind0=twinA, ind1=neighborTwinB, stiffness from bond strength)
3. Raise `eUPDATE_CLOTH` flag (springs are in the cloth buffer path)

**Between rigid bodies:** Use `PxD6Joint` or `PxFixedJoint` created dynamically:
```cpp
// When proximity detected:
PxD6Joint* bond = PxD6JointCreate(physics, bodyA, frameA, bodyB, frameB);
```
Joints can be created/destroyed at any time. Scene handles the bookkeeping.

**Between particle and rigid body:**
```cpp
particleSystem->addRigidAttachment(rigidActor);
// Plus per-particle attachment via PxParticleAttachmentBuffer:
attachmentBuffer->addRigidAttachment(rigidBody, particleID, localPos);
attachmentBuffer->copyToDevice();
```

### Recommendation
**PxParticleSpring for particle-level twin binding.** Dynamic spring creation between steps, GPU-native, strength from bond count. The neighborhood iterator gives you proximity detection for free — PhysX already computed it for the broadphase.

---

## 5. FEM Soft Body for Fuzzy Matching

When a particle cluster doesn't match vocabulary (not a known rigid body), it becomes a soft body for FEM resolution.

### Setup

**Create the soft body:**
```cpp
// Quick path — box approximation of the cluster shape:
PxSoftBody* soft = PxSoftBodyExt::createSoftBodyBox(
    transform, clusterDimensions,
    femMaterial, cudaContextManager,
    maxEdgeLength,     // tet mesh resolution
    density,
    solverIterationCount,  // higher = more accurate
    femParams,
    numVoxels,
    scale);

// Or from a custom mesh for better shape matching:
PxSoftBody* soft = PxSoftBodyExt::createSoftBodyFromMesh(
    softBodyMesh, transform,
    femMaterial, cudaContextManager,
    density, solverIterationCount,
    femParams, scale);
```

**Material — bond strength → Young's modulus:**
```cpp
PxFEMSoftBodyMaterial* mat = physics->createFEMSoftBodyMaterial(
    youngsModulus,   // stiffness from PBM bond strength (higher count = stiffer)
    poissonsRatio,   // volume preservation [0, 0.45] — keep below 0.5!
    dynamicFriction
);
mat->setDamping(damping);      // velocity damping for convergence
mat->setDampingScale(scale);   // [0,1]
```

**FEM parameters — convergence control:**
```cpp
PxFEMParameters params;
params.velocityDamping = 0.05f;      // per-step velocity reduction
params.settlingThreshold = 0.1f;     // motion for sleep candidacy
params.sleepThreshold = 0.05f;       // motion to enter sleep (= resolved)
params.sleepDamping = 10.0f;         // damping for sleep candidates
```

### Injecting bond forces as FEM constraints

**Option A: Kinematic targets (most direct)**
Use `ePARTIALLY_KINEMATIC` flag:
```cpp
soft->setSoftBodyFlag(PxSoftBodyFlag::ePARTIALLY_KINEMATIC, true);
```
Set known-good vertices as kinematic targets:
```cpp
PxBuffer* targets = soft->getKinematicTargetCPU();
// Write target positions for high-confidence vertices
// Leave uncertain vertices free for FEM to resolve
PxSoftBodyExt::commit(*soft, PxSoftBodyData::eSIM_KINEMATIC_TARGET);
```

**Option B: Particle attachments (pull toward known patterns)**
Attach anchor particles from the vocabulary (known patterns) to the soft body:
```cpp
PxU32 handle = soft->addParticleAttachment(
    particleSystem, particleBuffer,
    anchorParticleId,   // known-good particle
    tetId,              // tetrahedron in soft body
    barycentric         // position within tet
);
```
The anchor pulls the soft body vertex toward the known pattern. Strength determined by the attachment + material stiffness.

**Option C: Rigid body anchors**
If vocabulary entries are rigid bodies:
```cpp
PxU32 handle = soft->addRigidAttachment(
    knownWordRigidBody,
    vertexId,
    actorSpacePose,
    &coneLimitConstraint  // optional: constrain direction
);
```

### Recommendation
**Option A (kinematic targets) for the primary path.** High-confidence character positions are driven kinematically. Low-confidence positions are free. FEM minimizes energy across the free vertices using the material stiffness (= bond strength). The soft body deforms toward the nearest known word shape.

**Option B as supplement** — attach anchor particles from nearby vocabulary entries to pull the soft body toward specific known patterns. Multiple anchors from multiple candidate words compete. The one with strongest bonds (highest material stiffness) wins.

---

## 6. Convergence Detection — Zero-Loss / Equilibrium

### Option A: Sleep monitoring (simplest, built-in)

**For soft bodies:**
```cpp
bool resolved = soft->isSleeping();
PxReal energy = soft->getWakeCounter();  // approaches 0 as motion decreases
```
`PxFEMParameters::sleepThreshold` controls what "resolved" means. When all vertex motion drops below this, the body sleeps. **Sleeping = converged = token resolved.**

**For rigid bodies:**
```cpp
bool stable = rigid->isSleeping();
rigid->setSleepThreshold(threshold);  // kinetic energy threshold
```

**For particles:** No per-particle sleep. But you can check in `onPostSolve`:
```cpp
// In callback — read sorted velocities, check magnitude per particle/cluster
// If all velocities below threshold → cluster converged
```

### Option B: Stress tensor (diagnostic quality)

Enable on soft body:
```cpp
soft->setSoftBodyFlag(PxSoftBodyFlag::eCOMPUTE_STRESS_TENSOR, true);
```
Then read:
- `eTET_STRESS` — full Cauchy stress tensor (3x3 per tetrahedron)
- `eTET_STRESSCOEFF` — von Mises stress (scalar per tet)

**Von Mises scalar is the key metric.** Low stress = good fit. High stress = the FEM solution is strained = poor match to the vocabulary target. This gives you not just "converged or not" but **confidence per tetrahedron** — exactly where the match is weak.

### Option C: Velocity monitoring (most flexible)

In `onPostSolve` callback:
```cpp
// Sum velocity magnitudes across cluster
// Compare to threshold
float maxVel = 0;
for each particle in cluster:
    float4 vel = mSortedVelocities[sortedIndex];
    float mag = sqrt(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
    maxVel = max(maxVel, mag);
if (maxVel < convergenceThreshold) {
    // Cluster has converged — promote to rigid body
}
```

### Option D: Constraint force monitoring

For joints/constraints between rigid bodies:
```cpp
PxVec3 linear, angular;
constraint->getForce(linear, angular);
// If force magnitude is near zero → equilibrium
// If force magnitude is high → stress, possible bond break
```

### Recommendation
**Use all of them at different stages:**

| Stage | Detection Method |
|-------|-----------------|
| Particle clustering | Velocity monitoring in onPostSolve (Option C) |
| Cluster → rigid promotion | Velocity below threshold → lock rigid coefficient to 1.0 |
| FEM fuzzy matching | Sleep monitoring (Option A) + stress tensor (Option B) |
| Dumbbell chain assembly | Constraint force monitoring (Option D) |
| Final zero-loss check | Von Mises stress across all soft bodies = 0 |

The von Mises stress scalar is your "confidence score." Zero stress = perfect match = lossless. Non-zero = residual error = the system knows exactly where and how much.
