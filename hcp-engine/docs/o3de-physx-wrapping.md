# O3DE PhysX Gem: What's Wrapped vs Raw SDK

Quick reference for which PhysX 5 features O3DE wraps and which HCP must call directly.

---

## Fully Wrapped (use O3DE APIs if convenient)

| PhysX Class | O3DE Wrapper | Access |
|-------------|-------------|--------|
| PxRigidDynamic | PhysX::RigidBody | SimulatedBodyHandle |
| PxRigidStatic | PhysX::StaticRigidBody | SimulatedBodyHandle |
| PxShape | PhysX::Shape | Shape references on bodies |
| PxMaterial | PhysX::Material | FindOrCreateMaterial() |
| PxD6Joint | PhysX::PhysXD6Joint | Joint handle |
| PxFixedJoint | PhysX::PhysXFixedJoint | Joint handle |
| PxSphericalJoint | PhysX::PhysXBallJoint | Joint handle |
| PxSimulationEventCallback | PhysX::SceneSimulationEventCallback | Scene callback |

## Partially Wrapped

| PhysX Class | What's Wrapped | What's Not |
|-------------|---------------|------------|
| PxScene | Queries, body mgmt, gravity, events | GPU config, particle systems, soft bodies, broadphase control |
| PxArticulation | Link hierarchy, joint config | Low-level reduced coordinate access |
| PxCooking | Convex/tri/heightfield mesh | Tet mesh cooking for soft bodies |
| PxControllerManager | Basic character movement | Advanced controller features |

## NOT Wrapped (must call directly — ALL HCP-critical features)

| PhysX Class | Why HCP Needs It |
|-------------|-----------------|
| **PxPBDParticleSystem** | Token particles — primary substrate |
| **PxParticleSystem** | Base particle management |
| **PxParticleBuffer** | Token stream GPU buffers |
| **PxParticleSystemCallback** | onAdvance bond force injection |
| **PxCustomParticleSystemSolverCallback** | Custom solver logic |
| **PxSoftBody** | FEM error correction / inference |
| **PxFEMSoftBodyMaterial** | Bond strength → Young's modulus |
| **PxFEMParameters** | Convergence/sleep thresholds |
| **PxAggregate** | Token cluster grouping |
| **PxCudaContextManager** | GPU context and memory |
| **PxBroadPhase** (standalone) | Custom broadphase config |
| **PxConstraint** (raw) | Custom bond constraints |
| **PxContactModifyCallback** | Contact modification during solve |

**Bottom line:** O3DE wraps rigid body game physics well. Everything HCP needs (particles, soft body, aggregates, GPU) is unwrapped. This is expected — O3DE is a game engine, not a scientific simulation framework. We access PhysX directly from the Gem.

---

## Raw PhysX Access Patterns from an O3DE Gem

### Get the PxScene
```cpp
#include <AzFramework/Physics/PhysicsScene.h>

AzPhysics::SceneInterface* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
AzPhysics::Scene* scene = sceneInterface->GetScene(sceneHandle);
physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
```

### Thread-Safe Scene Access
```cpp
#include <PhysX/PhysXLocks.h>

// Write lock (for modifications)
PHYSX_SCENE_WRITE_LOCK(pxScene);

// Read lock (for queries)
PHYSX_SCENE_READ_LOCK(pxScene);
```

### Get Raw Pointers from O3DE Wrappers
```cpp
// RigidBody → PxRigidDynamic
physx::PxRigidDynamic* px = static_cast<physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

// Shape → PxShape
physx::PxShape* pxShape = shape->GetPxShape();

// Joint → PxJoint
physx::PxJoint* pxJoint = static_cast<physx::PxJoint*>(joint->GetNativePointer());

// Material → PxMaterial
const physx::PxMaterial* pxMat = material->GetPxMaterial();
```

### Resolve PhysX Actors → O3DE Entities
```cpp
#include <PhysX/Utils.h>

physx::PxActor* pxActor = /* from callback */;
PhysX::ActorData* data = PhysX::Utils::GetUserData(pxActor);
if (data && data->IsValid()) {
    AZ::EntityId entityId = data->GetEntityId();
}
```

### Linking & Includes

CMakeLists.txt for HCP Gem already links against PhysX. To use raw PhysX:
```cpp
#include <PxPhysicsAPI.h>          // Full API
// Or specific headers:
#include <PxPBDParticleSystem.h>
#include <PxParticleBuffer.h>
#include <PxSoftBody.h>
#include <PxAggregate.h>
#include <extensions/PxParticleExt.h>
#include <extensions/PxSoftBodyExt.h>
```

The O3DE PhysX Gem's CMake config provides all necessary include paths and link targets.

### Foundation Gotcha
When accessing PhysX from a Gem (shared library):
```cpp
// PxGetFoundation() returns module-local globals
// Must set the foundation instance from the PhysX system
PxSetFoundationInstance(existingFoundation);
```

---

## Key O3DE System Classes

| Class | Role | Access |
|-------|------|--------|
| PhysX::PhysXSystem | Manages PxPhysics, PxCooking, PxCpuDispatcher | AZ::Interface |
| PhysX::PhysXScene | Manages individual PxScene instances | SceneInterface |
| PhysX::PhysXSceneInterface | Implements AzPhysics::SceneInterface | AZ::Interface |

---

## HCP Architecture Implication

The HCP Gem creates and manages its own:
1. PxPBDParticleSystem(s) — added to the O3DE-managed PxScene
2. PxParticleBuffer(s) — per-stream token containers
3. PxSoftBody instances — for FEM error correction
4. PxAggregate(s) — for resolved token clusters
5. PxCudaContextManager — for GPU operations (may reuse O3DE's if available)

All of this lives alongside O3DE's normal rigid body simulation in the same PxScene. No conflict — PhysX supports mixed actor types natively.
