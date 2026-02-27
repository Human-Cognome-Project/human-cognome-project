# PhysX Consultation: Phase-Gated Tier Cascade Model

## Context

Phase 2 design has evolved. The superposition zone model (runs in chambers, Z discrimination, broadphase pruning) is unchanged. What's new is **how frequency tiers are managed within each chamber.**

Previous model: sequential buffer-swapping between tiers — load tier 1 vocab, simulate, check, rebuild buffer with tier 2 vocab, simulate again.

New model: **all tiers present simultaneously** in the same buffer, differentiated by phase groups with `eSelfCollideFilter`. Stream particles cascade through tiers by phase group reassignment between simulate() calls. No buffer rebuild. No vocab swapping.

See `physx-phase2-vocabulary-scanning.md` for the full updated design.

## The Phase-Gated Tier Model

### Setup (per chamber)

One PBD buffer containing:
- **Tier 1 vocab (static, invMass=0):** 200-500 words × avg 5 chars = 1,000-2,500 particles. Phase group 1, `eSelfCollideFilter`.
- **Tier 2 vocab (static, invMass=0):** 500-1,000 words × avg 5 chars. Phase group 2, `eSelfCollideFilter`.
- **Tier 3 vocab (static, invMass=0):** 1,000-2,000 words × avg 5 chars. Phase group 3, `eSelfCollideFilter`.
- **Stream runs (dynamic, invMass=1):** Variable. Phase group starts at 1.

All particles in the same buffer. All tiers spatially coexist (same X/Y/Z layout as before — Z = character identity, X = position within word). Tiers are NOT spatially separated — they're phase-group separated.

### Resolution Cycle

```
1. Stream particles enter buffer at phase group 1
2. simulate() — stream interacts ONLY with tier 1 vocab (eSelfCollideFilter)
3. Host reads back, checks N-element AND per word candidate
4. Matched: mark inert (phase=0 or move to dead zone)
5. Unresolved: update phase group to 2 (partial H→D write of phase array)
6. simulate() — stream now interacts ONLY with tier 2 vocab
7. Repeat for tier 3, tier 4, etc.
8. Exhausted: var-wrap
```

### Key Advantage

The entire resolution cycle is **one standardized routine**. Same logic for every chamber. The only variable is what PBM-derived vocab populates each tier. Chambers clone from templates, resolve, rescind. Standard buffer sizes make GPU memory budgeting trivial.

## Questions for PhysX Specialist

### Q1: eSelfCollideFilter Isolation — Full or Partial? — ANSWERED

**CRITICAL CORRECTION:** `eParticlePhaseSelfCollideFilter` (bit 21) is NOT for tier isolation. It's a rest-position proximity filter for cloth-like setups (requires `setRestParticles()`). The actual isolation mechanism is simply different phase group IDs + `eParticlePhaseSelfCollide` (bit 20, "interact with SAME group"). Different group IDs = zero collision pairs. Isolation is inherent and stronger than expected. See specialist notes in `physx-phase2-vocabulary-scanning.md`.

The tier cascade depends on different group IDs providing complete isolation between phase groups. Specifically:

a) If particle A is in group 1 with `eSelfCollideFilter`, and particle B is in group 2 with `eSelfCollideFilter` — is it guaranteed that A and B generate ZERO broadphase pairs? Not just "no collision response" but actually no broadphase work?

b) If particle A is in group 1 with `eSelfCollideFilter`, and particle C is in group 1 with `eSelfCollideFilter` — they interact normally (self-collision within group). Correct?

c) What about particles with `eSelfCollide` (without Filter) — do they interact with ALL particles regardless of group? If vocab in group 1 has `eSelfCollideFilter` but stream also in group 1 has `eSelfCollide` instead, does the stream see particles in other groups?

**Why this matters:** If isolation is partial (broadphase still generates pairs but solver ignores them), the tier model has hidden cost. If isolation is complete (broadphase skips cross-group pairs entirely), the model is clean.

### Q2: Phase Group Update on Live Buffer — ANSWERED

**CONFIRMED:** `getPhases()` returns device pointer. Direct device write via `PxScopedCudaLock` + targeted CUDA memcpy to specific offsets. `raiseFlags(eUPDATE_PHASE)` marks dirty. ~800 bytes for 200 particles. Negligible cost. See specialist notes in `physx-phase2-vocabulary-scanning.md`.

Between tier steps, we need to change unresolved stream particles from group N to group N+1.

a) Can we modify just the phase entries for specific particle indices in the phase array, call `raiseFlags(eUPDATE_PHASE)`, and simulate again? Or does `eUPDATE_PHASE` re-upload the entire phase array?

b) Is there a partial-update API? The phase array is `PxU32*` — can we write to specific offsets in the device array directly via `PxScopedCudaLock` + raw CUDA memcpy, then raise flags?

c) What's the cost? For 200 stream particles in a 4,000-particle buffer, we're updating 200 PxU32 values = 800 bytes. Is `raiseFlags(eUPDATE_PHASE)` smart enough to only sync the dirty range, or does it transfer the whole array?

**Why this matters:** If the phase update is a full-buffer re-upload, the tier cascade model loses its advantage over buffer rebuild. If it's a partial update (or even a full upload of a small array), it's negligible.

### Q3: Multiple Vocab Tiers at Same Spatial Position — ANSWERED

**CONFIRMED:** Tiers at identical (X, Y, Z) with different group IDs generate zero collision pairs. No Y-lane separation needed. Transparent overlay. See specialist notes in `physx-phase2-vocabulary-scanning.md`.

In the previous model, vocab words were spatially differentiated by Y-lane. In the phase-gated model, multiple tiers can have vocab words at the **same X positions** (same character positions within word) and **same Z** (same character identity).

Example: "there" (tier 1, group 1) and "these" (tier 2, group 2) both start with 'th' — their first two character particles are at identical (X, Z) positions.

a) With `eSelfCollideFilter`, do the tier 1 and tier 2 versions of 'th' interact with each other? (They shouldn't — different groups.)

b) Do they need Y-lane separation anyway? Or can they occupy exactly the same position since they never interact?

c) If they CAN overlap spatially without interference, this simplifies the layout significantly — no Y-lane management for tiers. Each tier's vocab is a transparent overlay at the same coordinates. Only the phase group determines which tier the stream sees.

**Why this matters:** If tiers can overlap spatially, the buffer layout is much simpler. If they can't (broadphase generates pairs even with filter), we need Y-lane separation per tier.

### Q4: Standard Buffer Size — Profiling Strategy

We want to determine the optimal standard buffer size empirically.

a) What metrics should we measure? Candidates:
   - `simulate()` wall time vs particle count
   - Broadphase pair generation count (is this accessible?)
   - GPU occupancy / SM utilization (via CUDA profiler)

b) With all tiers present but only one active (via eSelfCollideFilter), does the broadphase index ALL particles or only the interacting ones? If it indexes everything, the total buffer size affects broadphase cost even when most particles are inert. If it only indexes active groups, the standard buffer can be large without penalty.

c) On GTX 1070 (15 SMs, 8GB), what's a reasonable ceiling for total particles per scene across all PBD systems? 100K? 500K? 1M?

### Q5: PBD System Overhead Per Chamber

Each resolution chamber is a separate PBD system within one PxScene.

a) What's the per-system overhead? Is there a fixed cost per PBD system in a scene regardless of particle count?

b) With 50 chambers (50 PBD systems) in one scene, does `simulate()` handle all 50 in one GPU dispatch? Or is there per-system serialization?

c) Is there a practical limit on PBD systems per scene? Memory? GPU scheduling? Dispatch overhead?

**Why this matters:** If per-system overhead is low, many small chambers is fine. If it's high, fewer larger chambers (spatially partitioned internally) may be better.

### Q6: Phase Group Namespace for Concurrent Configurations

Multiple PBM-driven configurations (different envelopes) may run simultaneously in the same scene.

Phase groups are scene-scoped. With 20 bits:
- Bits [0-3]: tier ID (16 tiers per config — more than enough)
- Bits [4-7]: configuration ID (16 concurrent configs)
- Bits [8-19]: type encoding (4,096 types)

a) Is this partitioning compatible with how PhysX evaluates `eSelfCollideFilter`? Does the filter compare the FULL 20-bit group ID for equality, or just a subset?

b) If it's full equality: group 0x00001 (tier 1, config 0) and group 0x00011 (tier 1, config 1) are different groups — they don't interact. Correct?

c) If correct, this means each configuration's tiers are automatically isolated from other configurations' tiers. No spatial separation needed between configurations. Clean namespace partitioning handles everything.

### Q7: Buffer Pool vs. Per-Operation Allocation

For the clone/rescind lifecycle:

a) Is it faster to pre-allocate a pool of standard-size buffers and reuse them (overwrite contents, re-attach to PBD system) than to allocate/deallocate per operation?

b) Can a `PxParticleBuffer` be detached from one `PxPBDParticleSystem` and re-attached to another? Or is the buffer bound to the system that created it?

c) What's the cost of `createParticleBuffer()` + `release()` per operation? If it's < 1ms, pooling may not be worth the complexity.

## Files for Reference

- `Gem/Source/HCPSuperpositionTrial.cpp` — working Phase 1 (all-particle, 200/200)
- `Gem/Source/HCPWordSuperpositionTrial.cpp` — current Phase 2 trial (single-tier, no phase gating)
- `Gem/Source/HCPParticlePipeline.cpp` — GPU scene creation
- `docs/physx-phase2-vocabulary-scanning.md` — updated design doc (phase-gated model)
- `docs/physx-phase2-response.md` — previous specialist response (still valid for scene separation, match detection, promotion)
