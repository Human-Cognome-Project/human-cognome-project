# Phase 2: Char→Word Resolution — Superposition Zone Architecture

## Prerequisite
Phase 1 (byte→char superposition, all-particle architecture) is proven — 200/200 bytes settled, 100%. Static codepoint particles (invMass=0) + dynamic input particles, PBD self-collision, gravity-driven settlement. Working code at `HCPSuperpositionTrial.cpp`.

**Critical framing:** The engine does not process text, files, or formats. It receives raw byte streams. Phase 1 resolves bytes into identity (character particles). Phase 2 resolves character sequences into vocabulary tokens. Each phase is the byte stream acquiring higher-order structure through physics.

## Core Concept — Superposition Resolution (Same Model as Phase 1)

Phase 1: each byte position has 256 codepoints in superposition. Z discriminates. One settles.

Phase 2: each character run has N vocabulary words in superposition. Character sequence discriminates. One settles.

**Phase 2 is a purer superposition than Phase 1.** In Phase 1 we don't know the length of the construct (1-byte vs 2-byte vs 3-byte UTF-8) — the no-leftovers/longest-coverage rule resolves that ambiguity. In Phase 2, we already know the exact length. That ambiguity is gone.

### Superposition Parking Spots

The character stream is partitioned into runs at whitespace boundaries. Each run has an exact length and a first character. At each run position, **every vocabulary word matching those constraints exists in superposition** — stacked at that position, waiting for resolution.

Think of it as parking spots. Every spot has every matching word in superposition. The run's character sequence is the key. Only the word where **every character matches simultaneously** changes phase and incorporates its constituent character particles. Everything else stays in superposition, untouched, ready for the next run.

Same mechanic as byte codes. Same resolution model. The constraints define which parking lot — the physics inside each spot is identical to Phase 1.

### Why Superposition, Not Scanning

Superposition is the fastest resolution method — all candidates tested simultaneously in one physics step. The match-comparison space explodes without constraints, but **length + first letter** collapse it to a tractable zone. A 5-letter run starting with 't' is in superposition with maybe 50-100 vocabulary entries, not the full 1.4M vocabulary. Comparable to Phase 1's 256 codepoints per position.

No scanning velocity. No step counts. No tunneling concerns. The run is *in* the zone, not *transiting* it. Resolution is instantaneous superposition collapse, not sequential scanning.

## Constraint Hierarchy

Each level partitions the superposition space further. All deterministic, no heuristics.

1. **Length** (exact) — a 5-letter run goes to the 5-letter zone. Only 5-letter vocabulary words exist there. No fuzziness, no or-longer, no sliding windows.
2. **First letter** (exact) — further partitions within the length zone. All words starting with 't' cluster together.
3. **Frequency tier** (phase-gated) — vocab for a (length, first-letter) zone is split into tiers by frequency. All tiers are loaded simultaneously into the same buffer as static particles, each tier assigned a distinct phase group ID. Isolation is inherent: particles only interact with the **same group ID** when `eSelfCollide` (bit 20) is set. Different group IDs = zero interaction, zero broadphase pairs. Stream particles cascade through tiers by phase group reassignment between simulate() calls: tier 1 → tier 2 → tier 3 → var.

The chamber size (how many superposition candidates per zone) is an empirical tuning parameter, not an architectural choice. Profile it: how many simultaneous superposition particles can a resolution chamber handle? That number defines the frequency tier boundaries.

### PBM-Driven Tier Assembly

Frequency tiers are NOT global rankings. They are **context-weighted by aggregated PBMs** from the active envelope.

- A single document's PBM gives that document's frequency landscape (bond counts = frequency).
- Aggregate PBMs across an envelope give the contextual frequency landscape for that domain.
- Tier 1 for a physics textbook envelope: "quantum", "particle", "field". Tier 1 for a legal envelope: "plaintiff", "statute", "jurisdiction".
- The engine tunes itself to the work at hand — same chambers, same routine, different PBM feeding the tier assembly.

No hardcoded frequency tables. The context weights emerge from the bond data the system already has. The PBMs ARE the frequency data — bond counts in PBMs tell you exactly how often each token pair appears. Tier boundaries are threshold cuts on bond count.

## Resolution Rules (Universal Across All LoD Levels)

These rules apply identically at every level of the hierarchy: byte→char, char→word, word→phrase, and beyond.

1. **Longest match wins at same level.** If "the" and "there" both match starting at position 45, "there" wins — it claims all 5 positions. No backtracking.
2. **Larger structure subsumes constituents.** A word claims its characters. A phrase claims its words. The claimed elements are removed from the stream for that level.
3. **Earlier pass wins ties.** Frequency-ordered passes establish priority — common words stamp first. If a position is already claimed, later passes skip it.

This is why frequency-ordered cascading works as a correctness strategy, not just a performance optimization. The pass order IS the priority order.

The same rule applies in Phase 1: multi-byte UTF-8 sequences are longer constructs that subsume their constituent bytes.

**In the length-partitioned superposition model:** Runs are exact-length (determined by type-driven splitting). A 5-letter run enters the 5-letter zone; a 3-letter run enters the 3-letter zone. They don't compete — different runs, different zones. Longest-match-wins applies **within compound/morpheme resolution chambers** (Step 5), where unmatched runs are split into sub-word segments and the longest matching segment claims its positions first. It also applies when a run fails its exact-length zone and gets re-evaluated as potential sub-runs.

## Processing Model — Phase-Gated Resolution Cycle

### Step 1: Chunk the Input — Type-Driven Run Splitting
Split input stream into runs using **particle types from Phase 1**, not just whitespace. Punctuation particles carry type attributes that define boundaries:
- Whitespace particles → run boundary
- Period, comma, semicolon, colon → run boundary (punctuation particle stays as boundary context)
- Apostrophe within alpha run → keeps run intact (contractions: "don't" = one run)
- Hyphen within alpha run → keeps run intact (compounds: "well-known" = one run)
- Quote marks → run boundary (quote particle carries dialogue-context type)
- Digits adjacent to alpha → run boundary (type transition)

The splitting logic is driven by particle types, not character values. Phase 1 already classified every byte — the types ARE the boundary rules. No regex, no character-class lookup tables.

Group runs by length and first character. Each group enters its corresponding resolution chamber.

### Step 2: Chamber Assembly — Clone, Populate, Resolve

Each (length, first-letter) group gets a resolution chamber. The chamber is a **standardized PBD buffer** — empirically determined optimal size, cloned from a template.

**Chamber contents:**
- **Vocab particles (static, invMass=0):** All tiers loaded simultaneously, each tier at a distinct phase group ID with `eSelfCollide` flag set. Tier isolation is inherent — different group IDs never generate collision pairs. Vocab content populated from context PBMs and secondary searches based on the work being done.
- **Stream particles (dynamic, invMass=1):** The runs for this (length, first-letter) group, positioned at the chamber's X region. `eSelfCollide` flag set, group ID matches current active tier.

**All chambers are separate PBD systems within one PxScene.** One `simulate()` call processes all chambers in parallel. The GPU distributes work across SMs regardless of per-chamber particle count.

### Step 3: Phase-Gated Tier Cascade

Within each chamber, the stream cascades through frequency tiers via phase group reassignment:

1. **Stream enters at tier 1's phase group.** Same group ID + `eSelfCollide` means stream particles ONLY interact with tier 1 vocab. Different group IDs = zero broadphase pairs. Simulate.
2. **Host checks matches.** Full-match runs (all N characters settled) are confirmed and marked inert (phase set to 0, moved aside).
3. **Unresolved stream particles flip to tier 2's phase group.** Partial H→D write (phase array only). Simulate again — now they see tier 2 vocab.
4. **Repeat** through tier 3, tier 4, etc.
5. **Exhausted = var.** Runs that pass through all tiers without full match get var-wrapped.

The entire cycle is **one routine** — same logic for every chamber regardless of content. The only variable is what vocab the PBM puts into each tier.

**Between tier steps:** Only the phase group array is updated for unresolved particles — a few hundred bytes of H→D transfer. No buffer rebuild. No vocab swapping. No position changes. The vocab tiers are all present simultaneously; the phase group is the gate that controls which tier the stream currently resolves against.

### Step 4: Type-Based Routing (Post-Resolution)

After standard vocab resolution, residual runs route by particle type:
- **Label-type particles** (capitalized, non-sentence-start): route to proper noun chambers
- **Plain alpha**: standard vocabulary chambers (already tried — route to compound/morpheme)
- **Digits**: number chambers
- **Punctuation at run boundaries**: context (quotes = dialogue, period = sentence-start)

No heuristics. Types on the particles tell you where to route.

### Step 5: Specialty Chambers (Same Routine, Different Content)

Unmatched runs enter specialty chambers — **same phase-gated routine**, different vocab loaded:

- **Compound detection:** Chamber loaded with morpheme vocab (prefixes, suffixes, roots) from Postgres. Same superposition mechanic, same tier cascade.
- **Proper noun chamber:** Chamber loaded from entity lists and dramatis personae. Same routine.
- **Novel compound track:** Runs containing known sub-words in new combinations get flagged as potential new compounds for the vocabulary pipeline.
- **Truly unknown:** Runs that exhaust all chambers get var-wrapped. Genuine unknowns — candidates for vocabulary expansion or cache-miss resolution from Postgres.

No dead ends. Unmatched runs get re-routed until resolved or confirmed unknown.

### Why frequency ordering matters for correctness
Common words are shortest and most ambiguous (substring overlaps). Resolving them first removes the most ambiguity. Rare words tend to be longer, more distinctive — by the time you reach them, surrounding context is mostly claimed, fewer runs to test.

## Buffer Lifecycle — Clone / Resolve / Rescind

### Standard Buffer Sizes

The buffer size is an empirically determined constant — the particle count where broadphase + solver cost per simulate() is optimal. Every chamber uses this standard size. GPU memory / unit size = max concurrent chambers. Simple capacity planning.

### Clone

Vocab templates are host-side arrays precomputed from PBMs. Clone = allocate buffer at standard size + memcpy template vocab into first V slots + write stream particles into slots V..V+S + one H→D transfer. All tiers' vocab is written once; tiers are differentiated by phase group assignment, not spatial separation.

### Resolve

The phase-gated tier cascade runs within the cloned buffer. Between tier steps, only the phase array is partially updated (unresolved particles get new group ID). No allocation, no deallocation, no position changes for vocab.

### Rescind

Buffer released when the chamber's work is done. The PBD system can persist (reusable for next batch) or release (if chamber type is rare/on-demand).

### Reuse

For persistent chambers (common length/first-letter combos), the buffer can be reused: overwrite stream portion with new runs, reset phase groups, simulate again. Vocab portion unchanged. Zero allocation overhead for subsequent batches.

## PhysX Primitives

| Component | Primitive | Notes |
|-----------|-----------|-------|
| Character runs | Dynamic PBD particles (invMass=1) in resolution chamber | Phase group = tier selector (cascades through tiers) |
| Vocabulary words | Static PBD particle clusters (invMass=0) | Phase group = tier ID + `eSelfCollide` flag (all tiers coexist, different IDs = zero interaction) |
| Tier gating | Phase group reassignment between simulate() calls | Stream sees only one tier at a time; partial H→D update |
| Match detection | Broadphase contact + host-side N-element AND | Broadphase prunes by Z (character identity); host confirms all-character match |
| Phase change | Matched word incorporates constituent character particles | Character particles set inert, word particle carries accumulated type |
| Claiming | Resolved positions tagged in host-side array | Subsequent tier steps skip claimed positions |
| Tier content | PBM bond counts → tier boundaries | Context-weighted, not global frequency ranking |

### Why PBD particles, not kinematic rigids
- Particles carry phase groups — type encoding (grammatical class, frequency tier, constituent types) propagates through the LoD chain
- Labels are particles under the noun type — they can be spotted by type, not by iterative search
- Every classification in the system is a particle class: search is traced along force lines, not iterated over records
- Breaking out of physics at any stage (rigid body = geometry, not particle) loses type continuity

### Why phase-gated tiers, not buffer-swapping tiers
- All vocab loaded once — no buffer rebuild between tiers
- Phase group update is a targeted device write (~800 bytes for 200 particles) vs. full buffer teardown/rebuild
- Group ID isolation is inherent to PBD — different IDs never interact, no spatial tricks needed
- Tiers can overlay at identical spatial positions — no Y-lane separation between tiers
- Multiple concurrent configurations can run different PBM-driven tier sets simultaneously
- The same routine handles every chamber — standardized, predictable, profileable

## Resolution Chamber Sizing

The key engineering parameter: how many superposition candidates can one resolution chamber handle simultaneously?

**Phase 1 baseline:** 200 codepoint particles + 200 input particles = 400 total PBD particles (all-particle architecture, one codepoint per input byte position). Resolved at 119ms on GTX 1070 with substantial headroom. The GPU routinely handles 100K+ PBD particles — Phase 1's 400 barely registers. This establishes the floor.

**Phase 2 per chamber:** All frequency tiers coexist in the same buffer. A (length=5, first='t') chamber might have:
- Tier 1: 50 high-freq words × 5 chars = 250 static particles (phase group 1)
- Tier 2: 100 mid-freq words × 5 chars = 500 static particles (phase group 2)
- Tier 3: 200 rare words × 5 chars = 1,000 static particles (phase group 3)
- Stream: 80 runs × 5 chars = 400 dynamic particles
- Total: 2,150 particles in the buffer

Per simulate() call, only particles sharing the same group ID interact. So the active broadphase work per step is 400 stream + 250 tier-1 = 650 particles. Other tiers' particles are in the spatial hash but generate zero pairs (different group ID = instant rejection). Well within capacity.

**Standard buffer size:** Determined empirically — the particle count where broadphase + solver cost is optimal. All chambers use the same standard size. Oversized chambers waste GPU memory; undersized chambers miss vocab. Profile to find the sweet spot, then standardize.

**Particle budget per standard buffer:**

| Component | Particles | Phase group ID | Notes |
|-----------|----------|----------------|-------|
| Tier 1 vocab | ~250-500 | 1 | High-freq from context PBMs |
| Tier 2 vocab | ~500-1,000 | 2 | Mid-freq |
| Tier 3 vocab | ~1,000-2,000 | 3 | Rare/long-tail |
| Stream runs | ~200-500 | Cascades: 1→2→3 | Dynamic, rebuilt per batch |
| Graveyard | (resolved) | 0 | No other particles in group 0 = zero broadphase cost |
| **Total buffer** | **~2,000-4,000** | | All tiers present simultaneously |
| **Active per simulate()** | **~500-1,500** | | Stream + one tier only |

Peak active particle count well within GPU capacity. Total buffer size is the memory constraint, not the compute constraint.

## PhysX Specialist Notes

### Multi-byte UTF-8 at Phase 1→2 boundary
In Phase 1, each byte gets its own codepoint particle. A 3-byte UTF-8 sequence (e.g., 0xE4 0xB8 0xAD for '中') produces 3 settled particles. For Phase 2, these promote as a **single combined codepoint particle** — the byte→char bond table resolves multi-byte sequences into their character identity. The 3 byte particles are consumed; one character particle is created with the resolved codepoint's Z encoding. This is part of the Phase 1→2 transition at the var gate: byte-level resolution completes, character-level particles emerge. Unresolved multi-byte sequences get var-wrapped.

### Superposition vs scanning (architecture decision)
The earlier "switching yard / arch" model had the stream scanning through static vocabulary structures — advancing one position per step, generating step counts proportional to stream length. The superposition zone model eliminates this: runs drop INTO resolution chambers and resolve simultaneously, same as Phase 1. No scanning velocity, no tunneling, no step-count scaling problem. Chamber capacity is the constraint, not transit time.

### Contact detection mechanics
Same as Phase 1. Z-axis encodes character identity (Z = ascii × Z_SCALE). All vocabulary words at a parking spot have their character particles at the same X positions as the run. Broadphase spatial hash prunes on Z — only same-character pairs generate contact. Host confirms N-element AND (all characters matched simultaneously).

### Phase-gated tier cascade — VERIFIED against PhysX 5.1.1 headers

**Critical terminology correction:** The original design referenced `eSelfCollideFilter` (bit 21) for tier isolation. This is WRONG. `eParticlePhaseSelfCollideFilter` is a rest-position proximity filter for cloth-like setups — it requires `setRestParticles()` and has nothing to do with inter-group isolation. Do not use it.

**Actual isolation mechanism:** Different phase group IDs inherently don't interact. `eParticlePhaseSelfCollide` (bit 20) means "interact with particles of the SAME group." Different group IDs = zero collision pairs. This is the DEFAULT behavior — tier isolation is built into the phase group system, not added by a filter flag.

Verified from `PxParticleSystemFlag.h:82-93`:
```
eParticlePhaseGroupMask = 0x000fffff        // bits [0,19] = group ID
eParticlePhaseSelfCollide = 1 << 20         // interact with SAME group
eParticlePhaseSelfCollideFilter = 1 << 21   // rest-pose proximity filter (NOT for tier isolation)
eParticlePhaseFluid = 1 << 22               // fluid density constraints
```

**Tier cascade is stronger than expected.** Isolation is inherent, not dependent on a filter flag. All tiers coexist in the same buffer at identical spatial positions. The group ID is the ONLY thing that determines which tier the stream sees.

**Phase group update between simulate() calls — VERIFIED:**
- `getPhases()` returns a device pointer (`PxU32*` on GPU) — phases live on device
- `raiseFlags(PxParticleBufferFlag::eUPDATE_PHASE)` marks phase data as dirty
- No dirty-range support (flag is per-type, not per-range), but phases are already on device
- Direct device write path: `PxScopedCudaLock` acquires CUDA context → write to specific offsets in device phase array → release lock → `raiseFlags(eUPDATE_PHASE)` → next simulate() uses updated values
- Cost for 200 stream particles: 200 × 4 bytes = 800 bytes targeted device write. Negligible.
- Alternative: `PxScene::applyParticleBufferData()` for batch GPU-side updates across multiple buffers with CUDA event synchronization

**Spatial overlay — VERIFIED:**
- Tiers at identical (X, Y, Z) positions: different group IDs → zero collision pairs
- Particles share spatial hash cells (position determines cell) but group ID check rejects cross-group pairs during pair generation
- No Y-lane separation needed between tiers. Each tier is a transparent overlay.
- Only overhead: slightly more particles per hash cell (all tiers contribute to cell occupancy), but group ID rejection is the first check — trivial cost

### Unmatched run handling
Compound detection and specialty chambers are mechanically sound. "Route to morpheme chamber" = load residual particles into a different PBD resolution chamber (affix/morpheme vocabulary in superposition). Same PBD mechanics, different vocabulary set, **same phase-gated routine**. No new PhysX primitives needed.

### Runtime context
This pipeline is the universal intake — every piece of data entering the system for any purpose (ingestion, query, interaction, training data, metadata) passes through it. Concurrent load isn't a peak scenario, it's the baseline. Hundreds of simultaneous source streams, concurrent with mesh modeling forces and output stream assembly — all as physics operations. Multiple PBM-driven configurations can run simultaneously — different envelopes with different context-weighted tiers processing concurrently in the same scene.

## Manifest Structure — Yard Bookkeeping

The yard operates on cars, not trains. Multiple input streams (trains) are processed concurrently — the yard doesn't know or care which train a car belongs to. Manifests keep the trains straight.

### The Car Model
- **IMS (intermodal) car** = a word-length unit. A 5-letter word is a 5-platform car. Each platform carries a character container (resolved from Phase 1). The platforms ARE the character particles.
- **Specialty cars** = punctuation, structural tokens. Different rolling stock, different handling, same manifest tracking.
- **Car number** = reusable index key (position in stream). Not content-derived. "Car 47 is currently in zone 3" — that's all the yard needs.
- **Container numbers** = character values (scanned in Phase 1). The car's contents are known, but routing is not yet assigned.

### The Manifests
- **Master manifest (per train):** Car ID → position in original consist. Immutable. This is how the train gets reassembled after the yard is done. One per input stream.
- **Yard working manifest:** Car ID → current state (unscanned, in zone, resolved, residual, rerouted). Updates as cars move through the yard. Shared across all trains.
- **Resolution manifest:** Car ID → assigned token ID + routing. Empty on arrival, filled by the xray (superposition resolution) step. This is the result.

### Train-Agnostic Processing
The yard processes cars, not trains. A resolution chamber can have cars from 20 different trains simultaneously. It xrays whatever's in front of it, assigns routing based on contents, moves on. The master manifests per train handle reassembly — the yard never needs to think about train identity.

This is the concurrency model: any active zone can process from any available stream at any time. The car ID + master manifest is what eliminates coordination overhead.

## Persistent Scenes — Standing Infrastructure

Resolution chambers are NOT created per operation. They are persistent PBD systems within the char→word PxScene that sit latent and take what comes in.

- A (length=5, first='t') chamber exists with its PBM-populated vocabulary tiers loaded as static particles across phase groups. It waits for matching runs. Processes them via phase-gated cascade. Goes back to waiting.
- Vocabulary loading is a **one-time infrastructure cost per envelope switch**. First stream in an envelope pays for PBM-derived tier setup. Every subsequent stream in that envelope gets it for free.
- Envelope switch = tier contents update from new aggregate PBMs. Same chambers, same buffers, different vocab content. The phase group structure doesn't change — only what's in each tier.
- New vocabulary added to DB = new particles loaded into the existing chamber's appropriate tier. No rebuild of the whole buffer.
- Rare combinations spin up on demand and persist as long as they're seeing traffic.
- The yard builds itself out based on what's actually coming through.

**Multiple configurations can run simultaneously.** Different envelopes with different PBM-weighted tiers can process concurrently in separate chambers or separate PBD systems within the same scene. One `simulate()` call processes all of them.

Scaling is by adding track capacity (more chambers, more concurrent lanes), not by running faster. More streams = more cars in the yard simultaneously, same chambers processing them. The engine reads the manifests, knows what's loaded, knows what's waiting, dispatches accordingly.

Dynamic load balancing follows from the manifests: the engine monitors chamber utilization, resolution rates, and residual counts. These are the factors that determine optimal chamber configuration at runtime — not hard-coded parameters.

## Relationship to Phase 1
Phase 1 (byte→char superposition, all-particle) produces typed character particles. Those particles — carrying phase group type encoding — become the runs that enter Phase 2 resolution chambers. The output of Phase 1 IS the input of Phase 2. No serialization boundary. Particles carry type forward through every LoD level. The resolution mechanic is the same at every level — superposition, constraint, collapse.

## Answered Questions (PhysX Specialist Verification)

**Q1: Phase group isolation — CONFIRMED.** Different group IDs inherently don't interact. `eSelfCollide` (bit 20) means "interact with SAME group." No filter flag needed. Isolation is the default. Broadphase pair generation checks group ID within spatial hash cells — cross-group pairs rejected at cheapest possible point.

**Q2: Phase group update on live buffer — CONFIRMED.** `getPhases()` returns device pointer. Write directly to device phase array via `PxScopedCudaLock` + targeted CUDA memcpy. `raiseFlags(eUPDATE_PHASE)` marks dirty. Next simulate() uses updated values. Cost: ~800 bytes for 200 particles. Negligible.

**Q3: Spatial overlay — CONFIRMED.** Tiers at identical positions with different group IDs generate zero collision pairs. No Y-lane separation needed between tiers. Transparent overlay. Minimal spatial hash overhead (more particles per cell, but instant group ID rejection).

**TERMINOLOGY CORRECTION:** All references to `eSelfCollideFilter` in the design were wrong. That flag (bit 21) is for rest-position proximity filtering (cloth). The correct mechanism is simply different group IDs + `eSelfCollide` (bit 20). The isolation is simpler and stronger than originally described.

## Open Questions for Implementation

1. **Standard buffer size profiling.** What particle count optimizes broadphase + solver cost per simulate() on GTX 1070? Phase 1 baseline (400 particles, 119ms) establishes the floor. Need to profile at 1K, 2K, 5K, 10K, 20K particles to map the cost curve and find the sweet spot for the standard buffer size.

2. **Multiple PBD systems per scene.** With 20-50 chambers as separate PBD systems in one PxScene, does one `simulate()` call process all of them? What's the per-system overhead? Is there a practical limit on PBD systems per scene?

3. **Buffer cloning performance.** Is there a faster path than allocate + H→D copy for populating a new buffer from a host-side template? D→D copy between buffers? Pre-allocated buffer pool with overwrite?

4. **Concurrent configurations.** Multiple PBM-driven tier sets (different envelopes) running simultaneously in the same scene. Phase groups are scene-scoped — need to partition the 20-bit namespace across concurrent configurations. Bits [0-3] = tier (16 tiers), bits [4-7] = configuration (16 concurrent configs), bits [8-19] = available. Viable? Note: `eSelfCollide` checks FULL 20-bit group ID for equality, so (tier=1, config=0) and (tier=1, config=1) are different groups — automatic isolation between configurations.

5. **Run boundary handling.** Words spanning a chunk split boundary. Options: overlap chunks by max word length, or catch boundary-spanning words in a cleanup pass from residual unmatched positions at chunk edges.

6. **Performance validation.** Physics output must match computational tokenizer output for every document. The CPU pipeline is the correctness oracle.

7. **Continuously cycling chambers.** Persistent chambers that cycle through tiers continuously, processing whatever cars are present. Cars enter/exit asynchronously. Purge condition: car has been through full rotation without matching = var-wrap. What's the cost of simulate() on a chamber with no dynamic particles (vocab-only, all static)? If near-zero, continuous cycling is free when idle.
