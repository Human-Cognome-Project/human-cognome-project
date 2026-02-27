# Physics-Based Tokenization — Test Plan

Last updated: 2026-02-25

## Overview

Replace the computational tokenizer with physics-based resolution using PhysX 5 rigid bodies. Three stages of escalating scope, each resolving what it can and var-wrapping the rest. All stages validate against known-good baselines from the existing computational tokenizer.

The token stream is a line in 2D space (a string with cohesion). It is NOT 1D — cohesion between tokens gives it extent in two dimensions. The physics engine operates on this line naturally.

## Core Principle

Each stage does the same thing at its scope:
1. Known bodies attempt to match against the stream
2. What bonds is resolved
3. What doesn't bond gets var-wrapped and passed up
4. Detection/classification only works on the residue — what nothing claimed

## Stage 1: Byte → Char (Superposition)

**Method**: All known Unicode codepoints exist as rigid bodies in superposition at each byte position. Phase-collapse resolves matches.

**Why superposition**: The codepoint set is bounded and small enough (~150 relevant for English, ~150K for full Unicode) that superposition is practical. Every position resolves in parallel.

**Var behavior**: Any byte sequence that doesn't phase-collapse to a recognized codepoint gets wrapped as a byte-level var — a "letter" the system doesn't know.

**Validation**:
- Feed Yellow Wallpaper raw bytes
- Every collapsed codepoint must match current tokenizer char output
- Test with deliberate garbage bytes to verify var-wrapping
- Zero regressions on clean UTF-8 English text

## Stage 2: Chars → Words (Superposition or Ballistic — TBD)

**Method options**:
- **Superposition**: All dictionary entries in superposition at each char position. Phase-collapse for matches. May need successive phases for ~50K vocabulary.
- **Ballistic**: Fire word candidates along the char stream. They bond where they fit, leave copies.

**Optimization — Commonality Staging**: Don't fire/phase the whole dictionary uniformly. Stage by frequency:
- **Pass 1**: Most common words (the, a, is, of, etc.) — resolves majority of positions
- **Pass 2**: Next frequency tier — claims most of what's left
- **Pass 3+**: Progressively rarer words, only checking remaining gaps

Frequency ranking sources (early tests):
- Bond counts per starter from existing ingested documents (already in DB)
- Raw word frequency from position data
- Future: genre-specific PBM profiles (legal, fiction, academic, etc.)

**Var behavior**: Char sequences not resolved by any word body get var-wrapped as new surface forms. These are the docvars.

**Validation**:
- Yellow Wallpaper: must produce 10,482 tokens, 20,200 slots (exact match)
- Var-minted surface forms must match current docvars
- Benchmark superposition vs ballistic for throughput
- Test all 4 proven Gutenberg texts (52KB–807KB range)

## Stage 3: Known Bodies — Ballistic

**Method**: Fire known entity bodies (Dramatis Personae, entity lists) along the resolved word stream. They bond where their token sequence matches, leaving copies at each occurrence.

**Body structure**: Entity rigid bodies have natural joints:
- Strong bonds between core name components
- Weak/flexible joints at bridge words (of, the, von, de)
- If a full entity bounces (no match), it breaks at joints; pieces fire back independently

**Var behavior**: Word-level vars not claimed by any known body remain as docvars for classification.

**Validation**:
- Entities claimed must match current fiction entity cross-reference results
- Remaining unclaimed vars must be the correct residue
- Bridge word handling: "Isle of Man" must resolve whether "of" is present or not

## Stage 4: Post-Pass Classification

Remaining vars (the residue after all known bodies have fired) get classified:
- Capitalized surface form → `proper` (reproduction directive: capitalize)
- Contains digits or special chars → `sic` (literal value, not vocabulary)
- URL patterns → `uri_metadata`
- Lowercase unknown → `lingo` (genuine vocabulary gap)

Note: This classification is currently computational (ClassifyVar). Future: derive category from bond behavior in the physics space rather than surface form pattern matching.

**Validation**:
- Categories must match current backfill results on existing documents

## Stage 5: Cross-Document Boilerplate Detection

**Method**: Two documents as parallel lines in 2D. Align and apply attractive forces. Shared structure (identical token sequences) bonds — documents "stick" where they're the same. Unique content stretches and breaks apart under the tension.

- Position-independent: boilerplate matches regardless of where it sits in each document
- Stretch/break behavior reveals both exact matches and near-matches (minor variations in standard clauses)
- Shared regions can be extracted as template bodies for future ballistic matching

**Validation**:
- Two Gutenberg texts share identical license header/footer — must detect this boilerplate
- Detected boilerplate region must match actual Gutenberg Project license block
- Unique content must be cleanly separated

## Baselines

All stages validate against proven computational results:

| Document | Tokens | Slots | Status |
|----------|--------|-------|--------|
| The Yellow Wallpaper | 10,482 | 20,200 | Lossless round-trip proven |
| A Tale of Two Cities | — | — | Lossless round-trip proven |
| The Adventures of Sherlock Holmes | — | — | Lossless round-trip proven |
| Pride and Prejudice | — | — | Lossless round-trip proven |

## PhysX Primitive Mapping (TBD)

- Body types for each stage (static vs dynamic, shape complexity)
- Solver configuration (iteration count, convergence thresholds)
- Broad-phase strategy (spatial hashing for superposition, sweep for ballistic)
- GPU vs CPU solver selection per stage
- Memory layout for large vocabulary superposition

## Implementation Order

1. Stage 1 (byte→char) — smallest scope, clearest validation
2. Stage 2 (char→word) — benchmark both methods, pick winner
3. Stage 3 (known bodies) — requires entity data, can test with Dramatis entries
4. Stage 5 (boilerplate) — can test as soon as Stage 2 works on multiple documents
5. Stage 4 (classification) — runs on residue, lowest priority for physics conversion
