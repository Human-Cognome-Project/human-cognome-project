# PhysX Consultation: Byte→Char Superposition Tokenization

## Context — What We're Building

HCP is a physics-first cognition engine. We're building a physics-based tokenizer that runs beside (not replacing) our existing computational tokenizer. The goal: prove physics can do byte→char→word resolution faster by exploiting GPU parallelism.

The **first step** is byte→char translation via superposition. Everything else (char→word, capitalization propagation, particle typing) builds on this foundation. If this doesn't work at the atomic level, nothing above it works.

## What We Have Working Today

### Engine Stack
- **O3DE** with **PhysX 5.1.1** (native, GPU-accelerated)
- **GTX 1070** (8GB, CC 6.1) as primary compute GPU
- `HCPParticlePipeline` manages: `PxPhysics`, GPU-enabled `PxScene`, `PxCudaContextManager`
- Broadphase configured as `eGPU` (GPU sweep-and-prune)

### Proven PBD Patterns (from HCPDetectionScene)
- Fresh `PxPBDParticleSystem` per operation (GPU buffers can't resize)
- `PxParticleBuffer` with pinned host buffers for H↔D transfers
- **D→H→compute→H→D force injection loop**: `simulate()` → `fetchResults()` → copy positions/velocities to host → compute forces on CPU → copy velocities back → `raiseFlags(eUPDATE_VELOCITY)`
- `PxScopedCudaLock` for all GPU memory operations
- **PxParticleClothBuffer is broken** (rest positions override springs) — all forces are manual

### What We Know Doesn't Work
- Native PBD springs (cloth buffer) — particles don't move
- Weak force accumulation (`vel += force * dt`) with small `dt` — too slow to converge
- Large `particleContactOffset` when using manual forces — PBD collision fights the injected forces

## The Superposition Concept

### The Idea
All 256 byte codepoints exist simultaneously in a "translation space" as static reference particles. When an input byte enters, it exists in superposition — it COULD match any codepoint. The physics resolves which one it matches. Broadphase handles the spatial discrimination: matching pairs are spatially close, non-matching pairs are far apart. The collapse happens in parallel for all input bytes simultaneously.

### Why This Matters Beyond Simple Translation
The particles that emerge from byte→char collapse carry **type properties**:
- A capital letter is a particle with a capitalization attribute
- A period is a punctuation particle carrying "capitalize-next"
- These properties propagate through particle contact naturally
- Proper nouns become a particle class, not a detection heuristic
- No sequential algorithms — everything resolves in parallel through physics

### The Performance Question
A computational byte→char lookup is O(1) per byte (array index). The physics approach adds GPU overhead but:
1. It parallelizes across ALL input bytes simultaneously
2. The resulting typed particles feed directly into higher-level physics operations (char→word) without round-tripping to CPU
3. At scale, the GPU pipeline amortizes setup cost

## Specific Questions for the PhysX Specialist

### Q1: What Primitive for Codepoint Targets?

The 256 codepoints in the translation space need to be collision targets that input bytes can match against. Options we see:

**Option A: PBD particles (invMass=0, static)**
- Pro: Same primitive as input bytes, known working pattern
- Con: PBD particle-particle contact uses `particleContactOffset` globally — can't discriminate per-pair. All particles within contact range interact equally.

**Option B: Rigid body sensors (kinematic, trigger shapes)**
- Pro: GPU broadphase (SAP/ABP) works natively on rigid body AABBs. Each codepoint could have an AABB sized/positioned so that only its matching byte falls within range.
- Con: Do PBD particles interact with rigid body triggers? Is there a contact callback or overlap report for particle-vs-rigid?
- Question: Does `PxSimulationFilterCallback` or `PxSimulationEventCallback::onTrigger` fire for PBD particles entering rigid trigger volumes?

**Option C: Rigid bodies as attractors (not triggers)**
- Each codepoint is a small kinematic rigid body at a known Z position
- Input bytes are PBD particles
- Question: Do PBD particles collide with rigid bodies? Can we detect particle-rigid contacts?

**Option D: Something else entirely?**
- SDF collision shapes?
- Custom broadphase query?
- Particle-particle with spatial hashing tuned to Z-axis?

### Q2: Spatial Layout for Broadphase Discrimination

The Z-axis encoding: `Z = byteValue * Z_SCALE` where `Z_SCALE = 10.0`. With `particleContactOffset = 3.0`, particles 1 ASCII code apart have Z-distance = 10.0 (beyond contact), while matching particles have Z-distance = 0 (within contact).

**Does PhysX GPU broadphase actually accelerate this?** Specifically:
- For PBD particles, does the broadphase spatial hash operate on all 3 axes (X, Y, Z)?
- Or does PBD use a flat spatial hash that ignores Z for particle-particle detection?
- If broadphase doesn't discriminate on Z, we're back to brute-force in the force loop

### Q3: Particle-to-Rigid-Body Interaction

If we use rigid bodies for codepoints and PBD particles for input bytes:
- Can PBD particles detect overlap with rigid shapes?
- Is there a contact/trigger callback that reports which particle hit which rigid body?
- Or must we manually check positions in the D→H loop?

### Q4: Carrying Properties on Particles

After a byte collapses to its character, the resulting particle needs to carry type information (is-capital, is-punctuation, punctuation-class, etc.). Options:

- **Phase groups**: Each particle type gets a unique phase. But phases are limited and mainly control self-collision.
- **User data on particle buffer**: Can we attach per-particle metadata? The buffer has positions, velocities, phases — is there a user-data channel?
- **Separate host-side array**: Track properties in a CPU array indexed by particle ID. Simple but requires H→D sync.
- **Position encoding**: Encode properties into unused spatial dimensions (e.g., W component of PxVec4 position is invMass — is there another channel?)

### Q5: Performance Architecture

For 200 bytes of input + 256 codepoint targets = ~456 particles:
- Is PBD overkill? Would a simpler approach (rigid body overlap queries, scene queries) be more appropriate at this scale?
- At what scale does PBD parallelism actually win over computational lookup?
- Is there a PhysX batched query API that could test "which of these 200 points overlap with which of these 256 regions" in one GPU call?

## Summary

We need the right PhysX primitive pairing for:
- **256 codepoint targets** (static, reference space)
- **N input bytes** (dynamic, need to find their matching target)

Where broadphase spatial discrimination does the matching work natively, and the collapsed particles carry forward as typed objects for higher-level physics operations.

The key constraint: this must be a foundation that char→word physics stacks on top of, not a dead end. The particles that emerge need to be usable by the next physics stage.

## Files to Reference
- `/opt/project/repo/hcp-engine/Gem/Source/HCPDetectionScene.cpp` — working D→H→H→D pattern
- `/opt/project/repo/hcp-engine/Gem/Source/HCPParticlePipeline.cpp` — GPU scene setup, material creation
- `/opt/project/repo/hcp-engine/Gem/Source/HCPSuperpositionTrial.cpp` — current (incorrect) attempt at word-level matching
- `/opt/project/repo/hcp-engine/Gem/Source/HCPSuperpositionTrial.h` — trial structs
