# PhysX Consultation: Phase 2 — Char→Word Superposition Zones

## Context

Phase 1 (byte→char superposition) is proven — 200/200 bytes settled, 100%, all-particle architecture. Static codepoint particles (invMass=0) + dynamic input particles, PBD self-collision, gravity-driven settlement. Working code at `HCPSuperpositionTrial.cpp`.

Phase 2 design has converged on **superposition zones** (not scanning). See `physx-phase2-vocabulary-scanning.md` for the full design.

## Architectural Update — Simulation Spaces

Phases are **persistent simulation spaces** that data passes through and transforms in.

### Key principles:

1. **Each resolution level is a separate simulation space.** Byte→char is one space. Char→word is another. Each space has its own PBD system (or PxScene) with parameters tuned for that level's discrimination.

2. **Multiple streams coexist in each space simultaneously.** Hundreds of concurrent data streams in various stages of resolution. Phase groups differentiate streams.

3. **Data routes to whichever space it needs.** Binary formats might skip byte→char. Pre-tokenized input enters at word level. Specialized spaces (JSON, XML) feed into common downstream spaces.

4. **Spaces are latent capabilities.** They exist, activate when needed. New spaces added without restructuring the pipeline.

5. **Var resolution gates each transition.** All unresolved elements get var-wrapped before leaving a space. Next space receives only valid typed particles.

6. **Spaces are persistent.** Reference structures (codepoint particles, vocabulary particles) live permanently. Streams flow through, resolve, promote out.

## Phase 2 Design — Superposition Zone Model

### Core Mechanic (Same as Phase 1)

Phase 1: each byte position has 256 codepoints in superposition. Z discriminates. One settles.

Phase 2: each character run has N vocabulary words in superposition. Character sequence discriminates. One settles.

Runs are split at **particle-type boundaries** from Phase 1 (punctuation types define boundaries, not just whitespace). Runs are grouped by exact length + first character. Each group enters a resolution chamber where all matching vocabulary words exist in superposition. Same gravity-driven settlement mechanic as Phase 1 — only the word where ALL characters match simultaneously settles.

### Constraint Hierarchy

1. **Length** (exact) — 5-letter run → 5-letter zone only
2. **First letter** — further partitions within length zone
3. **Frequency tier** — if zone too large, split by frequency. High-frequency words resolve first.

This collapses 1.4M vocabulary to ~50-200 candidates per chamber. Comparable to Phase 1's 256 codepoints per position.

### Spatial Layout per Chamber

Same encoding as Phase 1:
- **X** = position within the run (0, 1, 2, ..., len-1)
- **Y** = collapse axis (vocab word particles start at Y_OFFSET, settle to 0 on character match). Multiple vocab words differentiated by Y lane (word 1 at Y=1×spacing, word 2 at Y=2×spacing, etc.)
- **Z** = character identity (ascii × Z_SCALE)

Run's character particles are static at Y=0 (same as Phase 1 codepoints). Vocab word particles are dynamic. Each word's character particles settle where Z matches the run's character at that X position. Full settlement (all characters) = match.

## Questions for PhysX Specialist

### Q1: Separate PxScene per space, or separate PBD systems within one PxScene?

Each space needs independent parameters:
- Byte→char: Z_SCALE=10, contactOffset=0.4, gravity=(0,-9.81,0)
- Char→word: same Z encoding but different Y lane spacing, potentially different contactOffset

**Option A: Separate PxScene per space.**
- Pro: fully independent gravity, broadphase config, solver iterations
- Pro: `simulate()` called independently per space
- Con: each scene = separate GPU resource allocation

**Option B: Separate PBD systems within one PxScene.**
- Pro: shared GPU broadphase, one `simulate()` call
- Con: shared gravity and scene config
- Con: particles in different PBD systems don't interact (which is what we want — spaces are independent)

Which gives better GPU utilization for concurrent multi-stream operation?

### Q2: Superposition Zone Chamber Mechanics

The resolution chamber uses the same mechanic as Phase 1 (gravity-driven settlement, Z discrimination). Key differences from Phase 1:

a) **Multiple vocabulary words at the same X positions.** In Phase 1, one codepoint particle per input byte at the same (X, Z). In Phase 2, many vocab words share the same X positions (character positions within the run), differentiated by Y lane. With 100-200 vocab words, that's 100-200 Y lanes. **What Y-lane spacing is needed** to prevent cross-lane interaction? With contactOffset=0.4 (interaction range 0.8), spacing of 1.0 should isolate lanes. Confirm?

b) **Partial settlement detection.** In Phase 1, every byte settled (every byte has a matching codepoint). In Phase 2, most vocab words WON'T fully settle — only the correct word has all characters matching. The non-matching characters of wrong words stay at Y_OFFSET (nothing to contact at their Z). **Is partial settlement stable in PBD?** A word with 3/5 characters settled and 2/5 floating — do the settled particles pull the floating ones down (intra-word forces), or do they remain independent (no springs)?

c) **Multiple runs per chamber simultaneously.** Several runs of the same length+first-letter can resolve in the same chamber simultaneously. Each run needs its own set of vocabulary particles (different X base offsets). Runs at different X regions of the chamber don't interact (X spacing > contactOffset). Confirm this scales — 50 runs × 100 vocab words × 5 chars = 25K dynamic particles per chamber?

### Q3: Particle Promotion Between Spaces

When a word fully settles (all characters match), the character particles need to become a word particle. Options:

**Option A:** Character particles consumed (set invMass=0 or removed from active buffer). New word particle created in host-side array carrying accumulated type from all constituent characters. Word particle promoted to next space in a new PBD buffer.

**Option B:** The match is recorded host-side only. Character particles stay in the buffer but are tagged as claimed. No physical transformation during the simulation — transformation happens between passes when buffers are rebuilt.

**Option C:** Character particles physically merge — positions converge to centroid. Requires springs or manual force injection.

Option B seems simplest for the trial — record the match, transform between passes. Option A for production. Option C seems unnecessary.

Which preserves type continuity while being mechanically sound?

### Q4: Concurrent Resolution Chambers

Multiple (length, first-letter) zones need to resolve. Options:

a) **Sequential:** One chamber at a time. Simple, but serializes the work.
b) **Multiple PBD systems in the same scene:** One system per chamber. Independent particle sets, shared `simulate()` call. All chambers resolve in parallel.
c) **One PBD system, spatial separation:** All chambers in one system, separated by large X offsets. Broadphase prunes cross-chamber interactions.

Which gives best GPU utilization? Option (b) seems cleanest — each chamber is an independent PBD system with its own particle buffers.

### Q5: Multi-Stream Phase Group Budget

With hundreds of concurrent streams and type encoding in phase groups (20 bits):
- Stream identity needs bits
- Character type needs bits (from Phase 1)
- Word type needs bits (noun, verb, label, etc.)
- LoD level needs bits

Is 20 bits enough? The original recommendation was:
- Bits [0-7]: character class
- Bits [8-15]: capitalization state / modifier flags
- Bits [16-19]: reserved

With concurrent multi-stream and word-level type encoding, how should the budget be allocated?

### Q6: Vocab Duplication for Concurrent Streams

If multiple input streams resolve simultaneously in the same chamber, each stream needs its own copy of the vocabulary particles at its Y-lane set. 100 vocab words × 5 chars × 50 concurrent streams = 25K vocab particles just for one chamber's duplication.

Is there a way to share vocab particles across streams (read-only reference that multiple dynamic particle sets resolve against)? Or is duplication the only path and the particle budget just needs to account for it?

## Files for Reference
- `Gem/Source/HCPSuperpositionTrial.cpp` — working Phase 1 (all-particle, 200/200)
- `Gem/Source/HCPSuperpositionTrial.h` — Phase 1 interface
- `Gem/Source/HCPParticlePipeline.cpp` — GPU scene creation (gravity=-9.81)
- `docs/physx-superposition-response.md` — specialist's revised recommendations
- `docs/physx-phase2-vocabulary-scanning.md` — Phase 2 design doc (superposition zones)
