# 03 — AtomTressFX as the Blueprint for a Particle/Constraint SETTLE in AZSL Compute

**Audience:** the DI who will port our particle-settle GPU engine to O3DE AZSL compute shaders, **without PhysX/CUDA**.
**Status:** RESEARCH / STUDY corpus. No port code here — this is the model to internalize first.
**Date:** 2026-06-16

---

## 0. Why TressFX is our blueprint

Our engine settles particles: a **bed** of static particles (spelling words — immovable anchors) plus **run** particles that fall/relax and come to rest when their velocity drops below a threshold. We need that done in **AZSL compute shaders** on O3DE, no PhysX, no CUDA.

AtomTressFX (AMD TressFX 4.1 ported into O3DE's Atom renderer) does **exactly the same shape of computation**: thousands of strand-vertices integrated under gravity each frame and pulled back onto a target shape by a **Gauss–Seidel constraint-relaxation solver running entirely in compute shaders**. It is a Position-Based-Dynamics (PBD)-style solver: Verlet integrate → iterate constraints → write back. That is the precise structure of a settle solver. The hair never touches PhysX or CUDA — it is pure `[numthreads]` AZSL dispatched by Atom's pass system.

So TressFX gives us, as working, shipping, readable AZSL:
- Verlet integration with damping (our "fall under gravity, lose energy, come to rest").
- Distance/length constraints (our "particles keep spacing, don't interpenetrate the bed").
- Shape constraints (our "anchors pull the system back toward a target").
- A `w`-channel movable/immovable flag — **this is exactly our bed (immovable) vs run (movable) split**.
- A capsule collision routine (our bed/run collision geometry).
- The full CPU-driven multi-dispatch-per-frame loop with double/triple-buffered positions.

**Caveat to keep front-of-mind:** TressFX runs *continuously* — it never sleeps a strand. Our settle needs a **velocity<threshold freeze**. TressFX gives us every primitive *except* the sleep/freeze; that one we add. See §6.

---

## 1. Source map (what is actually on disk here)

All paths under `/opt/O3DE/25.10.2/Gems/AtomTressFX/`.

| File | Role |
|---|---|
| `Assets/Shaders/HairSimulationCompute.azsl` | **The blueprint.** All 6 simulation compute entry points. |
| `Assets/Shaders/HairSimulationCommon.azsli` | Helper math: `Integrate` (Verlet), `ApplyDistanceConstraint`, `CapsuleCollision`, `IsMovable`, quats. |
| `Assets/Shaders/HairSimulationComputeSrgs.azsli` | The `TressFXSimulationParams` constant struct + all the `Get*()` parameter accessors (damping, stiffness, iteration counts). |
| `Assets/Shaders/HairComputeSrgs.azsli` | Buffer layout: the position/prev/prevprev buffers, the single shared pool buffer, offset getters/setters. |
| `Assets/Shaders/*Compute.shader` | 6 thin JSON entry-point declarations, one per pass. |
| `Assets/Passes/*Compute.pass` | 6 pass templates wiring each shader into the Atom frame graph (`PassClass: HairSkinningComputePass`). |
| `Assets/Passes/AtomTressFX_PassRequest.azasset` | Top-level pipeline request. |

> **Gap (important):** This is a **binary/asset distribution**. The C++ driver that issues the dispatches (`HairRenderObject`, `HairSkinningComputePass`, `HairFeatureProcessor`) is **NOT shipped here** — `Code/` contains only `HairAsset.cpp/.h` (the asset loader). The per-frame dispatch loop in §3 is reconstructed from (a) the upstream AMD source `TressFXSimulation.cpp` and (b) the O3DE `.pass`/`.shader` wiring on disk. Cross-checked, consistent. When the DI has the full O3DE checkout, confirm against `Gems/AtomTressFX/Code/Source/Rendering/HairSkinningComputePass.cpp` and `HairRenderObject.cpp`.

---

## 2. The simulation model (this is what maps to our settle)

### 2.1 Integration — Verlet with exponential velocity decay (damping)

`HairSimulationCommon.azsli:292-298`:

```hlsl
float3 Integrate(float3 curPosition, float3 oldPosition, float3 initialPos, float dampingCoeff = 1.0f)
{
    float3 force = g_GravityMagnitude * float3(0, 0, -1.0f);
    // float decay = exp(-g_TimeStep/decayTime)
    float decay = exp(-dampingCoeff * g_TimeStep * 60.0f);
    return curPosition + decay * (curPosition - oldPosition) + force * g_TimeStep * g_TimeStep;
}
```

This is **classic position Verlet**: new = cur + (cur − old) + a·dt². Velocity is *implicit* — it is the stored difference `(cur − old)` between this frame's and last frame's position. There is no separate velocity buffer.

The damping is the load-bearing detail for a settle: `decay = exp(-dampingCoeff * dt * 60)` multiplies the velocity term each frame. With `dampingCoeff > 0` the velocity term shrinks geometrically every frame, so an undisturbed particle **bleeds kinetic energy and asymptotically stops**. That is the energy-loss half of a settle, for free. (The `*60` normalizes damping to a 60 fps reference.)

> **Maps to us:** This `Integrate` is our run-particle integrator verbatim. `dampingCoeff` is our settle-rate knob. The implicit `(cur − old)` velocity is exactly the quantity we threshold on for freeze (§6).

Call site, `HairSimulationCompute.azsl:90-93`:

```hlsl
if ( IsMovable(currentPos) )
    sharedPos[indexForSharedMem].xyz = Integrate(currentPos.xyz, oldPos.xyz, initialPos.xyz, dampingCoeff);
else
    sharedPos[indexForSharedMem] = initialPos;   // immovable -> pinned to rest
```

### 2.2 Movable vs immovable — the `w` channel (our bed vs run)

`HairSimulationCommon.azsli:58-63`:

```hlsl
bool IsMovable(float4 particle)
{
    if ( particle.w > 0 )
        return true;
    return false;
}
```

Position is `float4`; `.w` is **inverse-mass-style movable flag**. `w > 0` ⇒ simulated. `w <= 0` ⇒ pinned/immovable. Every constraint and the integrator respect this. **This is precisely our bed/run distinction** — bed particles ship with `w<=0` (anchors), run particles with `w>0`. No separate "is this a bed particle" buffer needed; it rides in the position's `w`.

### 2.3 Constraint types (four of them)

TressFX maintains shape via **four** constraint families, each a separate compute pass (numbers in §3):

1. **Global shape constraint** (`HairSimulationCompute.azsl:99-110`): pulls each vertex a fraction `stiffness` of the way back toward its *rest* position `initialPos`. Only applied within an "effective range" fraction of the strand. This is a **soft spring to a target shape**.
   ```hlsl
   float3 del = factor * (initialPos - sharedPos[...]).xyz;   // factor = global stiffness
   sharedPos[...].xyz += del;
   ```
2. **Local shape constraint** (`:228-283`): for bending/twisting — preserves each vertex's position *relative to its neighbours* in the strand's local frame (uses the bind-pose rotated by the current frame). One thread per strand, loops vertices 1..n-1. Clamped: `stiffness = 0.5 * min(stiffness, 0.95)` for convergence stability (`:241-242`).
3. **Length / distance constraint** (`:344-361` + `ApplyDistanceConstraint` in common `:228-238`): keeps adjacent vertices at rest length — stops stretch. This is the **Gauss–Seidel relaxation** core (§2.4).
4. **Collision** (`CapsuleCollision`, common `:314-374`; called from `:368` — note: currently disabled in this build, see §7): pushes vertices out of capsule volumes.

`ApplyDistanceConstraint` (common `:228-238`) is the canonical PBD distance projection, and it is **mass-weighted by movability**:

```hlsl
void ApplyDistanceConstraint(inout float4 pos0, inout float4 pos1, float targetDistance, float stiffness = 1.0)
{
    float3 delta = pos1.xyz - pos0.xyz;
    float distance = max(length(delta), 1e-7);
    float stretching = 1 - targetDistance / distance;
    delta = stretching * delta;
    float2 multiplier = ConstraintMultiplier(pos0, pos1);   // splits correction by movability
    pos0.xyz += multiplier[0] * delta * stiffness;
    pos1.xyz -= multiplier[1] * delta * stiffness;
}
```

`ConstraintMultiplier` (common `:65-81`) returns `(0.5,0.5)` if both movable, `(1,0)`/`(0,1)` if only one is, `(0,0)` if neither. **This is the exact behaviour we want at the bed/run boundary:** a run particle constrained against a bed (immovable) particle moves the *full* correction itself; the bed particle does not budge. Free, correct, and already mass-aware.

### 2.4 Iteration count — Gauss–Seidel relaxation

Constraints are solved iteratively (relaxation), not in one shot. Two iteration knobs, both in `m_SimInts` (`HairSimulationComputeSrgs.azsli:56`, accessors `:169-177`):

- **Length-constraint iterations** — looped **inside the shader** with a `GroupMemoryBarrierWithGroupSync()` between sub-steps (`HairSimulationCompute.azsl:344-361`):
  ```hlsl
  int lengthContraintIterations = GetLengthConstraintIterations();   // m_SimInts.x
  for ( int iterationE=0; iterationE < lengthContraintIterations; iterationE++ ) {
      if (localVertexIndex < a) ApplyDistanceConstraint(...);
      GroupMemoryBarrierWithGroupSync();
      if (localVertexIndex < b) ApplyDistanceConstraint(...);   // red/black split
      GroupMemoryBarrierWithGroupSync();
  }
  ```
  Note the **red/black (even/odd) split** with a barrier between halves: this is how a parallel Gauss–Seidel relaxation avoids two adjacent threads writing the same vertex in the same sub-step. Study this pattern — it is the key trick for doing a *serial-feeling* relaxation in parallel.

- **Local-shape iterations** — looped on the **CPU side**: the driver re-dispatches the `LocalShapeConstraints` pass N times (`m_SimInts.y`). From upstream `TressFXSimulation.cpp`:
  ```cpp
  int iterations = iterate ? hairObjects[i]->GetCPULocalShapeIterations() : 1;
  for (int j = 0; j < iterations; ++j) ctx.Dispatch(numGroups);
  ```

> **Two ways to iterate, both available to us:** loop inside one dispatch (shared-memory, barrier-synced, cheap — for constraints local to a thread group) vs loop the dispatch from the CPU (for constraints that need a global memory barrier / full buffer settle between passes). Our settle will likely use **both**: in-group relaxation for local spacing, CPU-looped dispatches for global bed/run settling.

---

## 3. The dispatch structure (per frame)

**Six compute entry points**, all in `HairSimulationCompute.azsl`, all `[numthreads(THREAD_GROUP_SIZE,1,1)]` with `THREAD_GROUP_SIZE = 64` (`HairSimulationCommon.azsli:45-47`). Each is its own `.shader` → `.pass` (`PassClass: HairSkinningComputePass`), so each is a **separate GPU dispatch wired into the Atom frame graph**.

Per-frame order (from upstream `TressFXSimulation.cpp`, matches the O3DE pass set):

| # | Pass / entry point | Granularity | Dispatches/frame | What it does |
|---|---|---|---|---|
| 1 | `IntegrationAndGlobalShapeConstraints` (`:47-114`) | **1 thread / vertex** | once | Skin root, Verlet-integrate movable verts, apply global shape spring, rotate position history. |
| 2 | `CalculateStrandLevelData` (`:126-179`) | 1 thread / strand | once | Compute per-strand VSP rotation/translation + skinning quat. |
| 3 | `VelocityShockPropagation` (`:191-216`) | 1 thread / vertex | once | Damp sudden root motion down the strand (anti-overstretch). |
| 4 | `LocalShapeConstraints` (`:228-283`) | 1 thread / strand | **looped (N)** | Bend/twist constraint toward bind pose. CPU-looped `m_SimInts.y` times. |
| 5 | `LengthConstriantsWindAndCollision` (`:295-398`) | 1 thread / vertex | once (loops internally) | Length relaxation (internal `m_SimInts.x` iters), collision, tangent, velocity clamp. |
| 6 | `UpdateFollowHairVertices` (`:409-437`) | 1 thread / vertex | once | Slave "follow" hairs onto their guide strand. |

So a frame is **roughly `5 + N` dispatches** (N = local-shape iterations). The CPU (the feature processor / `HairSkinningComputePass`) drives this; the **GPU does no cross-pass control flow** — every iteration boundary that needs a global memory barrier is a *new dispatch*.

**Thread-group packing (study this — non-obvious):** `THREAD_GROUP_SIZE = 64` threads per group, but a group holds **multiple strands**. `g_NumOfStrandsPerThreadGroup` strands share a group; `numVerticesInTheStrand = 64 / g_NumOfStrandsPerThreadGroup`. Index math in `CalcIndicesInVertexLevelMaster` (`HairSimulationCommon.azsli:253-265`) maps `SV_GroupIndex` → (strand, vertex). The reason: length & local-shape constraints need *all vertices of a strand in the same group* so they can talk through `groupshared` memory and `GroupMemoryBarrierWithGroupSync()`. **This is the central layout decision** — neighbours that constrain each other must be co-resident in a thread group.

Dispatch group count (upstream): vertex-level `numGroups = numTotalVertices / 64`; strand-level `numGroups = numStrands / 64`.

**Group-shared memory** (`HairSimulationCommon.azsli:49-51`):
```hlsl
groupshared float4 sharedPos[THREAD_GROUP_SIZE];
groupshared float4 sharedTangent[THREAD_GROUP_SIZE];
groupshared float  sharedLength[THREAD_GROUP_SIZE];
```
Positions are pulled into `sharedPos`, relaxed in-place with barriers, then written back. **Constraint iteration happens in fast LDS, not global memory.**

---

## 4. Buffer layout & double-buffering

### 4.1 Triple-buffered positions (not just double)

`HairComputeSrgs.azsli:88-103` — the per-object dynamic SRG:

```hlsl
ShaderResourceGroup HairDynamicDataSrg : SRG_PerObject
{
    RWBuffer<float4>  m_hairVertexPositions;        // current
    RWBuffer<float4>  m_hairVertexPositionsPrev;    // last frame   (Verlet velocity = cur - prev)
    RWBuffer<float4>  m_hairVertexPositionsPrevPrev; // two frames ago (used for VSP pseudo-accel)
    RWBuffer<float4>  m_hairVertexTangents;
    RWStructuredBuffer<StrandLevelData> m_strandLevelData;
    ...offsets...
};
```

So per-particle GPU state is **three position buffers** (cur / prev / prevprev). There is **no explicit velocity buffer** — velocity is `cur − prev`, acceleration proxy is `cur − 2·prev + prevprev` (used at `:162` to detect fast motion). The history is rotated each frame by `UpdateFinalVertexPositions` (common `:438-443`):

```hlsl
void UpdateFinalVertexPositions(float4 oldPosition, float4 newPosition, int globalVertexIndex)
{
    SetSharedPrevPrevPosition(globalVertexIndex, GetSharedPrevPosition(globalVertexIndex));
    SetSharedPrevPosition(globalVertexIndex, oldPosition);
    SetSharedPosition(globalVertexIndex, newPosition);
}
```

i.e. prevprev ← prev, prev ← old current, current ← new. A clean ring rotation, done in-shader.

### 4.2 One big shared pool, sub-allocated by byte offset

The clever O3DE-specific bit: all the per-object buffers are **views into one giant `RWStructuredBuffer<int>`** (`HairComputeSrgs.azsli:62-65`):

```hlsl
ShaderResourceGroup PassSrg : SRG_PerPass {
    RWStructuredBuffer<int> m_skinnedHairSharedBuffer;   // the whole pool, shared across all passes & draws
}
```

Each object knows its byte offsets into the pool (`m_positionBufferOffset`, `m_positionPrevBufferOffset`, …, `HairComputeSrgs.azsli:98-102`), and the getters convert offset+index into a pool index:
```hlsl
int vertexOffset = (m_positionBufferOffset >> 2) + (vertexIndex << 2);   // /4 bytes->int, *4 floats/vtx
```
Reason given in the source comment (`:84-87`): keeping everything in one buffer lets it be shared between *passes* via the per-pass SRG frequency without per-buffer barriers — the barriers are handled at the shared-buffer level by the frame graph. **For us:** a single pooled scratch buffer + per-object offsets is a good pattern for many independent settle islands (each word = an island) sharing one allocation.

### 4.3 How a particle knows its neighbours

Purely **by index arithmetic**, no neighbour list. A strand is a contiguous run of `numVerticesInTheStrand` vertices; vertex `i`'s neighbours are `globalVertexIndex ± 1` (see local-shape `:255-257`, length `:353,358`). Connectivity is *implicit in the layout*. **For us:** if our run particles have structured neighbour relationships (a chain, a grid), bake them into contiguous index ranges and we get neighbour lookup for free. If our bed/run interaction is *spatial* (any run particle can hit any bed particle), that is NOT index-implicit — that needs a spatial structure (grid/hash), which TressFX does *not* provide (it only does capsule collision against a handful of explicit capsules). See §5 and §7.

---

## 5. Collision handling

`CapsuleCollision` (`HairSimulationCommon.azsli:314-374`) is the only particle-vs-world collision. It projects a moving point out of a **capsule** (two spheres + cylinder), with friction:

```hlsl
bool CapsuleCollision(float4 curPosition, float4 oldPosition, inout float3 newPosition, CollisionCapsule cc, float friction = 0.4f)
```
- Immovable points are skipped (`!IsMovable` → return false, `:320-321`).
- Resolves against sphere-0, sphere-1, or the middle cylinder; pushes the point to the capsule surface along the normal.
- For the cylinder case it splits motion into tangential (scaled by `friction`) and normal components (`:365-369`) — i.e. **friction is modelled**, which matters for settling (a particle that lands should *stick*, not slide forever).

Collision wiring in the main pass, `HairSimulationCompute.azsl:366-369`:
```hlsl
float4 oldPos = g_HairVertexPositionsPrev[globalVertexIndex];
bool bAnyColDetected = false;       // Adi
//    bool bAnyColDetected = ResolveCapsuleCollisions(sharedPos[indexForSharedMem], oldPos);
```
**It is currently commented out in this O3DE build** (the `ResolveCapsuleCollisions` aggregate that loops the capsule set is disabled). The primitive exists and works; the per-frame driving of it is off. Relevant when `bAnyColDetected` is true the code also **rewrites position history** (`:394-395`) so collision doesn't inject phantom velocity — a detail worth copying.

> **Maps to us — and the key divergence:** TressFX collision is point-vs-a-few-capsules. Our **bed/run interaction is particle-vs-particle over a large static set** (the bed). TressFX gives us:
> - the *resolution* math (push-out + friction + history-rewrite) — reusable per contact;
> - the movable/immovable split so the bed never moves;
>
> but it does **NOT** give us **broad-phase** (finding *which* bed particle a run particle is near). For that we add a uniform grid / spatial hash. **Note our token identity is a HASH, not physics** — the physics here is purely the settle *geometry* (where particles come to rest); identity/disambiguation is resolved by our hash layer, not by the solver. Don't conflate the spatial-hash broad-phase (a settle accelerator) with our token-identity hash (a semantic layer).

---

## 6. Rest / settle detection — TressFX does NOT have it (this is our addition)

**TressFX simulates every strand every frame, unconditionally.** There is no sleep, no "this strand is at rest, skip it." The closest things to settle behaviour are *energy-management*, not *freezing*:

1. **Damping** (§2.1): `decay = exp(-damping·dt·60)` bleeds velocity so motion *asymptotically* dies. A hair left alone goes still — but the GPU keeps integrating it every frame regardless.
2. **Velocity clamp / history rewrite** (`HairSimulationCompute.azsl:382-387`): caps per-frame motion to stop explosions:
   ```hlsl
   float3 positionDelta = sharedPos[...].xyz - oldPos;
   float speedSqr = dot(positionDelta, positionDelta);
   if (speedSqr > g_ClampPositionDelta * g_ClampPositionDelta) {
       positionDelta *= g_ClampPositionDelta * g_ClampPositionDelta / speedSqr;
       g_HairVertexPositionsPrev[globalVertexIndex].xyz = sharedPos[...].xyz - positionDelta;
   }
   ```
   **Note the trick:** to change a Verlet particle's velocity you *rewrite its prev position* (since velocity = cur − prev). This is exactly the lever we pull to **freeze**: set `prev = cur` ⇒ velocity 0.

> **Maps to us — the settle-and-freeze we must add:**
> - **Detect rest:** `speedSqr = dot(cur - prev, cur - prev)`. If `speedSqr < threshold²` for a particle, it is settled. (`speedSqr` is *already computed* at `:383` — we just compare against a settle threshold instead of a clamp threshold.)
> - **Freeze:** set that particle's `w <= 0` (flip it from run to bed-like immovable) **or** set `prev = cur` (zero its velocity). Flipping `w` is stronger — it removes the particle from integration *and* makes it an immovable anchor for its neighbours, which is precisely a run particle "joining the bed." That matches our model: a settled run particle effectively *becomes bed*.
> - **Skip settled work (optional optimization):** TressFX always dispatches the full set. We can additionally compact/skip frozen particles (indirect dispatch over an "active" list) to stop paying for settled ones — TressFX does not do this, so it is net-new, but the freeze mechanism above is the prerequisite.

So: **TressFX = the integrator + constraint solver + the exact `w`-flip / prev-rewrite levers** needed to implement freeze. The *policy* (threshold compare → flip) is the one piece we write.

---

## 7. Things that are stubbed/disabled in THIS build (don't copy blindly)

- **Wind** — the whole wind block in `LengthConstriantsWindAndCollision` is commented out (`HairSimulationCompute.azsl:313-337`): "does not work yet and requires some LTC".
- **Capsule collision driving** — `ResolveCapsuleCollisions` call commented out (`:368`); only the primitive `CapsuleCollision` remains live in the header.
- **Dual-quaternion skinning** (`TRESSFX_DQ`) — `#define TRESSFX_DQ 0`, "not currently functional" (`HairComputeSrgs.azsli:40-42`).
- **`CM_TO_METERS` is 1.0** here (`:48`) — unit scaling is a no-op in this build; the `*CM_TO_METERS` multiplies are inert. Don't assume cm↔m conversion is active.
- **Strand "type"** indirection removed — `GetStrandType` always returns 0 (`HairSimulationComputeSrgs.azsli:126-129`); all the `Get*(strandType)` accessors ignore the argument and read a single global. Simplifies our port: one parameter set, not per-type.

---

## 8. Parameter block (what the CPU feeds each frame)

`TressFXSimulationParams` (`HairSimulationComputeSrgs.azsli:46-70`), the per-draw constant buffer. The settle-relevant fields:

| Field | Meaning | Our settle analogue |
|---|---|---|
| `m_Shape.x` | damping coeff | settle decay rate |
| `m_Shape.y` | local stiffness | local spacing stiffness |
| `m_Shape.z` | global stiffness | pull-to-target strength |
| `m_Shape.w` | global range | how much of the system is shape-locked |
| `m_GravTimeTip.x` | gravity magnitude | our gravity |
| `m_GravTimeTip.y` | timestep `dt` | our dt |
| `m_SimInts.x` | length-constraint iterations (in-shader loop) | spacing relaxation iters |
| `m_SimInts.y` | local-shape iterations (CPU dispatch loop) | global settle iters |
| `m_VSP.x/.y` | velocity-shock-propagation coeff / accel threshold | overstretch / fast-motion handling |
| `m_ResetPositions` | teleport flag → snap all to rest | our "respawn the run" reset |
| `m_ClampPositionDelta` | max per-frame motion | velocity clamp **and our settle-threshold hook** |

`m_ResetPositions` handling (`HairSimulationCompute.azsl:76-87`) snaps cur/prev/prevprev all to the skinned rest position — the clean way to (re)initialize a run.

---

## 9. How this maps to our bed/run settle — consolidated

| Our concept | TressFX mechanism | File:line |
|---|---|---|
| Run particle integration (fall, lose energy, stop) | `Integrate` Verlet + `exp(-damping·dt·60)` decay | `HairSimulationCommon.azsli:292-298` |
| Bed = immovable anchor | `IsMovable` on `position.w <= 0` | `HairSimulationCommon.azsli:58-63` |
| Run pulled toward target arrangement | global shape constraint (spring to `initialPos`) | `HairSimulationCompute.azsl:99-110` |
| Run particles keep spacing / don't overlap | `ApplyDistanceConstraint` + length-iteration loop | common `:228-238`, compute `:344-361` |
| Bed/run boundary: run moves, bed doesn't | `ConstraintMultiplier` mass-weighting | `HairSimulationCommon.azsli:65-81` |
| Iterating the solve to convergence | in-shader length loop (LDS+barriers) AND CPU-looped dispatches | compute `:346`; upstream `TressFXSimulation.cpp` |
| Contact resolution + friction + no phantom velocity | `CapsuleCollision` + history rewrite | common `:314-374`; compute `:394-395` |
| **Settle detected (velocity < threshold)** | `speedSqr = dot(cur-prev, cur-prev)` already computed | compute `:383` — **we add the threshold compare** |
| **Freeze a settled particle** | rewrite `prev = cur` (vel→0) and/or flip `w<=0` (run→bed) | lever shown at compute `:386` — **policy is ours** |
| Position state (no velocity buffer) | triple buffer cur/prev/prevprev, ring-rotated | `HairComputeSrgs.azsli:88-103`; common `:438-443` |
| Many independent islands (words) sharing memory | one pooled `RWStructuredBuffer<int>` + per-object byte offsets | `HairComputeSrgs.azsli:62-65, 98-102` |
| Neighbour lookup | implicit by contiguous index ranges | compute `:255-257, 353-358` |
| Per-frame driving = N dispatches, CPU-controlled | 6 entry points, `5+N` dispatches/frame | §3 |

**The shape of our port:** a small set of AZSL `[numthreads(64,1,1)]` compute entry points — *integrate+global-pull*, *relax-spacing (looped)*, *settle-detect+freeze* — each wired as its own `HairSkinningComputePass`-style compute pass into Atom's frame graph, driven by a CPU feature processor that decides the per-frame dispatch count and feeds a `TressFXSimulationParams`-shaped constant buffer. Position state in a pooled triple-buffer with per-island offsets. The **only genuinely net-new logic vs TressFX is the velocity-threshold→freeze policy and (optionally) the spatial broad-phase** for bed/run contact; everything else is a direct transliteration of code on disk here.

---

## 10. Top sources

1. **On-disk O3DE AtomTressFX shaders** (primary, authoritative) — `/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairSimulationCompute.azsl`, `HairSimulationCommon.azsli`, `HairSimulationComputeSrgs.azsli`, `HairComputeSrgs.azsli`; passes under `Assets/Passes/`.
2. AMD upstream **TressFX 4.1** repo — [GPUOpen-Effects/TressFX](https://github.com/GPUOpen-Effects/TressFX) (for the C++ `TressFXSimulation.cpp` dispatch loop not shipped in the O3DE asset drop).
3. [TressFX — AMD GPUOpen](https://gpuopen.com/tressfx/) — algorithm overview (Verlet, global/local shape + length constraints, all on GPU compute).
4. [TressFX features — DeepWiki](https://deepwiki.com/GPUOpen-Effects/TressFX/1.2-features) — 4.1 changes (faster VSP, simplified local shape, dispatch reorganization).

---

## 11. Open gaps / what to verify with the full O3DE checkout

1. **C++ driver not on disk here.** The dispatch loop (§3), the CPU local-shape iteration count, double-buffer swap, and `m_ResetPositions` driving are reconstructed from upstream + the `.pass` wiring. **Verify against** `Gems/AtomTressFX/Code/Source/Rendering/HairSkinningComputePass.cpp` and `HairRenderObject.cpp` in a real O3DE source tree. Confirm whether O3DE keeps the CPU loop over `LocalShapeConstraints` or folded it into the pass graph.
2. **Exact per-frame dispatch count** for the O3DE port (does it still do `5+N`, and what N default?). The constant-buffer defaults (`m_SimInts`) aren't in the assets — they're set in C++.
3. **Collision is disabled** in this build (§5, §7). If we want particle-vs-particle bed/run contact we are building broad-phase from scratch; TressFX is no guide there beyond the per-contact resolution math.
4. **No settle/sleep exists in TressFX at all** (§6) — confirm our freeze policy (w-flip vs prev-rewrite) against our actual run/bed semantics; decide whether frozen particles should remain immovable forever or be re-armed.
5. **Thread-group strand packing** (`g_NumOfStrandsPerThreadGroup`) is tuned for hair (short strands, ≤64 verts). Our islands may have different sizes; the 64-thread group + co-resident-neighbours constraint needs re-derivation for our particle topology.
6. **Atom pass-system specifics** (SRG frequencies, `PassClass` registration, frame-graph attachment/barrier handling for the shared buffer) are O3DE-version-sensitive — validate against 25.10.2's `Atom/RPI` pass API when porting.
