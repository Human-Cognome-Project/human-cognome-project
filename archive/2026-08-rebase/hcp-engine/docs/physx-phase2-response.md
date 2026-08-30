# PhysX Specialist Response: Phase 2 — Char→Word Resolution Spaces

## Preamble

The simulation spaces architecture is a significant step up from "sequential pipeline stages." Each space as a persistent, independent simulation environment with concurrent multi-stream operation changes several of my earlier recommendations. Verified all claims below against PhysX 5.1.1 headers.

---

## Q1: Separate PxScene per Space vs Shared Scene

### Answer: Separate PxScene per space. Strong recommendation.

**The decisive factors:**

1. **Phase groups are scene-scoped, NOT system-scoped.** Confirmed in `PxParticleSystemFlag.h:82-95`. If PBD system A and PBD system B are in the same PxScene and both have particles in phase group 5, those particles INTERACT across systems. There is no isolation between PBD systems within a scene at the phase group level. This kills Option B for independent spaces — you'd have to manually partition the 20-bit namespace across all spaces, burning bits on space identification instead of type encoding.

2. **With separate scenes, each space gets its own full 20-bit phase group namespace.** Byte→char gets 1M groups. Char→word gets 1M groups. No sharing, no partitioning. Each space uses all 20 bits for stream identity + type encoding.

3. **Per-space parameters.** Separate scenes give independent:
   - Gravity (mutable at runtime via `setGravity()`, but with one scene you'd need to pause all spaces to change it)
   - `broadPhaseType` (NOT mutable after creation — `PxSceneDesc.h:534`)
   - `gpuDynamicsConfig` memory allocation tuned per space
   - `gpuMaxNumPartitions` tuned per space

4. **PBD systems within each scene still give per-system tuning.** Contact offsets, grid sizes, solver iterations — all per-PBD-system (`PxPBDParticleSystem.h:67-129`, `PxParticleSystem.h:141-408`). If a space needs multiple PBD systems (e.g., different contact parameters for different tiers within a space), that works within one scene.

5. **Independent simulate() calls.** Each scene calls `simulate()` / `fetchResults()` independently. You can sequence them (byte→char finishes before char→word starts processing its output) or run them in parallel on separate threads if the CUDA context allows.

**GPU memory cost:** Each GPU-enabled scene allocates ~80MB baseline (`PxgDynamicsMemoryConfig` in `PxSceneDesc.h:376-414`: 16MB temp buffer + 64MB heap). With 4-5 active spaces = ~400MB out of 8GB on the GTX 1070. Acceptable. The `maxParticleContacts` default (1M per scene) may need tuning for high-throughput spaces.

**CUDA context sharing:** `PxSceneDesc::cudaContextManager` is a pointer field. Multiple `PxSceneDesc` instances can point to the same `PxCudaContextManager`. GPU work serializes through the shared context but this is fine — you're sequencing simulate() calls anyway. No need for separate CUDA contexts per scene.

**The architecture:**
```
PxCudaContextManager (one, shared)
├── PxScene: byte→char space
│   └── PxPBDParticleSystem (256 codepoints + N input streams)
├── PxScene: char→word space
│   └── PxPBDParticleSystem (vocab zones + M character streams)
├── PxScene: word→phrase space
│   └── PxPBDParticleSystem (phrase patterns + K word streams)
└── PxScene: [future spaces as needed]
```

Each space is a latent capability — create the PxScene when the space first activates, release when dormant. No restructuring needed.

---

## Q2: Switching Yard Zone Mechanics

### Q2a: Zone Layout

Sequential along the stream's travel axis (X). The stream enters at X=0, travels in +X direction.

```
Zone 0 (metadata/dramatis)    Zone 1 (top-1K freq)    Zone 2 (next tier)    ...
X: [0, W₀]                   X: [W₀, W₁]             X: [W₁, W₂]
```

Within each zone, vocabulary words are **stacked on Y** — each word at a different Y offset, all within contactOffset of the stream's Y level. Words can be densely packed because **Z discrimination handles character matching regardless of X/Y proximity.**

Key insight: with Z_SCALE=10 and contactOffset=3.0, two particles at identical X and Y but different Z (minimum Z-diff = 10.0) have 3D distance ≥ 10.0 > 2×contactOffset = 6.0. No contact. Z always discriminates, no matter how close particles are on other axes. This means:
- Words can overlap on X within a zone (different Y offsets)
- Hundreds of vocab words coexist in the same zone region
- The spatial hash prunes by Z first — only same-character pairs generate broadphase work

**Zone width** is determined by the longest word in the zone. A zone of 3-4 letter words is 4 units wide on X. A zone of 7+ letter words is wider. The stream transits each zone in `zone_width` steps (at SCAN_SPEED * DT = 1.0).

### Q2b: Stream Velocity

At `SCAN_SPEED * DT = 1.0`, the stream advances one character spacing per step. For a full 100K text through 4 zones of average width 10: 4 × 10 = 40 steps total. That's fast.

But wait — the stream isn't 1 particle. It's N characters, each spaced 1.0 apart on X. The STREAM itself is ~N units wide. As the stream transits a zone, different portions of the stream are in the zone at different times. The stream needs `N + zone_width` steps to fully pass through a zone. For N=100K and zone_width=10: ~100K steps per zone. At DT=1/60, that's ~28 minutes per zone.

**This is the scaling problem for the switching yard.** A long stream needs many steps to fully transit each zone.

**Scan rate: one car length per step.** `SCAN_SPEED * DT = 1.0`. The chunk advances one character position per step. The arch evaluates whatever's under it. Every position gets assessed — no tunneling (PBD has no CCD for particles), no host-side interpolation. The arch width IS the assessment window.

**Chunking solves the scaling problem.** Split 100K characters into 1K-char chunks at whitespace boundaries. Each chunk enters the yard on a different Y lane. 1,000 steps per chunk per arch pass. All chunks evaluated in parallel per simulate() call. After each pass, residuals merge and shrink — subsequent passes are faster.

### Q2c: Multi-Particle Match Detection

**This is fundamentally a host-side N-element AND check.** PBD contact resolves individual character pair proximity, and the broadphase prunes massively on Z. But "all N characters in a word matched simultaneously" must be confirmed on the host.

The physics contribution: for a vocab word of length N, the broadphase generates contact pairs ONLY for characters where Z matches. If 3 of 5 characters match, you get 3 contact pairs. If all 5 match, you get 5. The host reads back which pairs are in contact (position proximity < threshold) and checks if all N are satisfied.

This is an O(K) check where K = number of vocab words with at least one character in contact. K << total vocab because the broadphase prunes by Z. For a zone with 1,000 words, maybe 10-50 have any Z-matches at a given stream position. Checking 10-50 words for full-match is trivial.

**Not a problem.** The GPU does the O(M×N) broadphase work, the CPU does the O(K) confirmation.

### Q2d: Concurrent Streams

**This works with spatial separation.** Setup:

- Vocab words: invMass=0 (static), phase group 0, eSelfCollide flag on
- Stream A: invMass=1 (dynamic), phase group 0, enters at Y=0
- Stream B: invMass=1 (dynamic), phase group 0, enters at Y=100
- Stream C: invMass=1 (dynamic), phase group 0, enters at Y=200

All in the same phase group — all particles collide with all particles. But:
- Streams at Y=0 and Y=100 are 100 units apart — well beyond 2×contactOffset. No broadphase pairs generated. Zero interference.
- Vocab words exist at all Y levels (or at Y_offset levels within contactOffset of each stream lane)
- Each stream independently contacts the vocab words at its Y level

**Why same phase group?** Because vocab (invMass=0) must interact with ALL streams. If streams were in different phase groups with eSelfCollideFilter, each stream would only collide within its own group — and vocab in a different group wouldn't interact with any stream.

**Vocab duplication:** Each Y lane needs its own copy of the vocab particles (at that lane's Y offset). For 1,000 words × 5 chars × 100 concurrent streams = 500K vocab particles. Add 100K stream particles per stream × 100 streams = 10M stream particles. Total: 10.5M particles. That's within GPU budget but getting heavy. May need to tier: 10-20 concurrent streams per simulate() call, not hundreds.

**Or use separate PBD systems per stream** within the same scene. Each PBD system has its own vocab copy and one stream. Phase groups are scene-scoped, so you'd need unique group IDs per system. With 20 bits: up to 1M unique streams. But each PBD system = separate solver work. Trade-off between isolation and GPU utilization.

**Practical answer:** Start with spatial separation (same phase group, Y-offset lanes). Profile at 10, 50, 100 concurrent streams. If particle count gets too high from vocab duplication, switch to batched processing (process 10 streams per simulate() call, rotate through batches).

---

## Q3: Particle Promotion Between Spaces

### Answer: Option A variant — consume in source space, create in destination space.

**The mechanism:**

1. **Match detected** in char→word space (host-side AND check confirms all N characters aligned).
2. **Character particles marked inactive** — set invMass=0 and move to a "dead zone" far from active simulation (can't actually remove from PBD buffer — buffers don't resize).
3. **Host-side records** the match: position, token ID, accumulated type properties from constituent characters.
4. **New word particle created** in the word→phrase space's PBD buffer. Carries accumulated type in its phase group. Positioned at the word's sequence position in that space.
5. **Var gate:** any character positions NOT matched by any word get var-wrapped on the host before anything promotes to the next space. Only valid typed particles cross space boundaries.

**Why not Option B (host-side record only):** The word must physically exist as a particle in the next space for type continuity. A record can't be traced, can't be searched by force lines, can't carry phase group type encoding.

**Why not Option C (convergence to centroid):** PBD doesn't support merging particles. You can move them close together but they remain N particles, not 1. Maintaining N particles where 1 is needed wastes particle budget and confuses the next space's match detection.

**Buffer management:** PBD buffers can't resize, so "inactive" particles persist as dead weight. Over a long session, this accumulates. Options:
- Recreate the PBD system periodically (flush inactive particles, compact the buffer). This is what the engine already does — "fresh PBD system per operation."
- Size the initial buffer generously (max expected active + inactive during one pass).
- The var gate between spaces is a natural compaction point — after all chars promote or var-wrap, rebuild the buffer for the next batch.

---

## Q4: Vocabulary Reference Structure in Char→Word Space

### Spatial Layout

Each vocab word = N static particles (invMass=0), one per character, at:
```
(zone_X_offset + char_index, word_Y_lane, char_ascii * Z_SCALE)
```

Where:
- `zone_X_offset` = the X position of this zone within the space
- `char_index` = 0..N-1 position within the word
- `word_Y_lane` = unique Y offset for this word within the zone (prevents inter-word interference)
- `char_ascii * Z_SCALE` = character identity on Z (same encoding as Phase 1)

**Discrimination is on Z (character identity).** The X axis encodes position-within-word. The Y axis separates different words. Z does all the matching work — same as Phase 1, just more particles.

**Grouping within zones:**

Words in a zone should be sorted by **first character** so the spatial hash can prune efficiently. All words starting with 't' cluster in the same Z cell for their first character. When the stream's character at position P has Z_t, the broadphase only generates pairs with vocab words whose first particle is also at Z_t. Massive pruning.

Within that first-character cluster, second characters further prune. The spatial hash handles this automatically if the grid cell size matches Z_SCALE.

**Word length separation:** Words of different lengths occupy different X spans within the zone. A 3-letter word occupies X=[offset, offset+2]. A 7-letter word occupies X=[offset, offset+6]. They can coexist at the same X offset (different Y lanes) since the match check is per-word, not per-zone.

**Particle count:** 1,967 words × average ~5 chars = ~10K static vocab particles per zone. With 4 zones = ~40K total vocab particles. Plus stream particles. Well within budget.

---

## Q5: Multi-Stream Phase Group Budget

### Answer: 20 bits is sufficient with separate scenes.

With separate PxScene per space (Q1 recommendation), each space gets its own independent 20-bit namespace. No sharing across spaces.

**Within a space (e.g., char→word):**

If using spatial separation for stream isolation (same phase group, Y-offset lanes):
- All particles in group 0 with eSelfCollide
- No phase bits needed for stream identity (Y separation handles it)
- All 20 bits available for type encoding
- Bits [0-7]: character/word class (256 classes)
- Bits [8-15]: modifier flags (capitalization state, punctuation propagation, etc.)
- Bits [16-19]: sub-type or tier (16 values)

That's 1M possible type combinations per space. Far more than needed.

**If using separate phase groups per stream** (instead of spatial separation):
- Bits [0-9]: stream identity (1,024 concurrent streams per space)
- Bits [10-19]: type encoding (1,024 type combinations)
- More constrained but still workable for expected concurrency levels

**My recommendation:** Use spatial separation (Y-offset lanes) for stream isolation. Reserve all 20 phase group bits for type encoding. This maximizes the type system's expressiveness — which matters for traced search ("find all particles of type X") where the broadphase spatial hash indexes by phase group.

---

## Summary of Key Architecture Decisions

| Decision | Recommendation | Rationale |
|----------|---------------|-----------|
| Scene per space | Separate PxScene | Independent phase groups, gravity, broadphase config |
| CUDA context | Shared PxCudaContextManager across scenes | One GPU, serialized access |
| Stream movement | Bullet-train model (vocab or stream moves, not both) | Scales better for long streams |
| Match detection | GPU broadphase prunes, host confirms N-element AND | O(M×N) on GPU, O(K) on CPU |
| Concurrent streams | Same phase group, Y-offset spatial separation | All 20 bits for type encoding |
| Particle promotion | Consume in source space, create in destination space | Type continuity preserved |
| Vocab layout | Z = character identity, Y = word lane, X = char position | Z discrimination handles matching |
| Phase budget | Full 20 bits for type per space | No stream identity bits needed |

---

## Refined Model: Chunked Switching Yard with Run Sorting

Developed in discussion with Patrick after the initial response.

### The Scaling Problem
A 100K character stream through a single arch set needs ~100K steps. Too slow. Linear transit time is the bottleneck.

### The Solution: Chunk, Paint, Merge, Repeat

1. **Split** the input stream into scannable chunks (~1,000 chars) at whitespace boundaries. Each chunk enters the yard simultaneously on a different Y lane.

2. **First arch pass** = metadata-known terms (Pass 0) + high-frequency common words. These arches paint the most characters. For a 1,000-char chunk, the top common words might paint 400-500 characters.

3. **Painted cars shunt to assembly track.** Unpainted residuals from adjacent chunks merge back to scannable size (they carry original position offsets from the first split point).

4. **Next arch pass** operates on the merged, much smaller residual. Repeat until resolved or var-wrapped.

Each subsequent pass processes dramatically fewer particles. The problem collapses on itself.

### Run Sorting Between Passes (CPU-Side Bookkeeping)

Between arch passes, the CPU sorts residual runs by different criteria — like reading different tags on the first car to decide which siding the consist goes to. **Runs stay intact. The tag is a shunting marker, not a separation point.**

| After this pass | Sort by | Why |
|----------------|---------|-----|
| Initial split | **Run length** | Which arch set can match this run |
| Within length groups | **First character of run** | Which arches to activate (broadphase pre-filter) |
| Common-terms pass | **Particle type** | Label→proper noun track, alpha→vocab track, digit→number track |
| Proper noun pass | **Remaining length** | Regroup residuals for rare-vocab arches |

### Particle Type Routing

After common-terms arches paint the bulk, residual runs contain a mix. Particle types from Phase 1 ARE the routing logic:

- **Label-type particles** (capitalized, non-sentence-start): route to proper noun arches. Proper nouns are detectable as Labels — a particle class, not a detection heuristic.
- **Plain alpha particles**: route to standard vocabulary arches.
- **Digit particles**: route to number arches.
- **Punctuation type at run boundaries**: informs context. Quotes = dialogue candidate. Period before run = sentence-start (capitalize-next is positional, not a label — filter out of proper noun track).

No heuristics. The types on the particles tell you where to route.

### Spaces as System Services

Each space is permanent infrastructure — create once, persist, serve every stream:
- **Byte→char scene**: 256 codepoint particles. Spins up on first byte stream, stays alive.
- **Char→word yard**: Vocabulary arches loaded from DB. Spins up on first character stream, persists.
- **Word→phrase space**: Phrase pattern infrastructure. Same model.

New vocabulary = add arches to existing yard. Don't rebuild. Next stream sees the new arches automatically. New language = new yard instance. The architecture doesn't change.

### Hardware Context

GTX 1070 (8GB, 15 SMs) is the development floor. Architecture decisions driven by correctness and scalability, not 1070 performance. Any newer hardware makes everything faster without code changes — more SMs = more parallel arch evaluation, more VRAM = more concurrent streams.

Design the parallelization trees correctly and the GPU promotes parallelism as fast as the hardware allows.

## Open Questions

1. **Vocab duplication for concurrent streams.** If streams are at different Y offsets, vocab needs copies at each Y level (contactOffset can't span large Y gaps without destroying Z discrimination). Max concurrent streams per simulate() call = function of particle budget. Profile at 10, 50, 100 streams.

2. **Buffer compaction timing.** When to rebuild PBD buffers to flush inactive (painted) particles? Natural point: at the merge step between arch passes. Merge = rebuild buffer with only unpainted particles.

3. **Chunk overlap at boundaries.** Words spanning a chunk split boundary will fail to match in both chunks. Options: overlap chunks by N characters (max word length), or catch boundary-spanning words in a cleanup pass from residual unpainted positions at chunk edges.

4. **Performance validation.** This architecture is novel. It may perform well or may have unforeseen bottlenecks. The proof is in implementation. The computational (CPU) pipeline exists as the correctness oracle — physics output must match computational output for every document.
