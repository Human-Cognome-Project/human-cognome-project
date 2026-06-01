# Pair-Bond Maps

## Overview

A Pair-Bond Map (PBM) is the storage representation for any scoped expression. It records what tokens appear, how they pair, and how often — enough information to reconstruct the original structure losslessly.

## Structure

A PBM is a set of entries:

```
TokenID(0).TokenID(1).FBR
```

- **TokenID(0)** — the current token
- **TokenID(1)** — the next relevant token in sequence
- **FBR** (Forward Bond Recurrence) — how many times this exact pairing occurs in the scope

Each unique ordered pair of tokens is a **Forward Pair-Bond (FPB)**. The FBR is the count of that FPB within the scope.

## What counts as "relevant"

- Whitespace and structural non-meaning-bearing tokens are skipped when determining the next token.
- Formatting tokens (punctuation, markup) are always relevant.
- The definition of "relevant" may vary by modality but must be declared per mode.

## Reconstruction

A PBM contains enough information to reconstruct the original scoped expression. The first and last tokens of the scope are stored alongside the PBM as anchors.

### Particle Physics Model

Reconstruction is a soft body alignment operation:

1. Each bond pair (TokenA → TokenB, count) spawns as a paired particle unit.
2. Every pair seeks matches at both ends — the token at each end looks for another pair with a matching token.
3. When matches meet, pairs merge at the shared token and separate at the junction.
4. The separation event **is** the whitespace (spacebar character, 0x20). Whitespace is not stored or inserted by rule — it falls out of the merge/separate physics.
5. Punctuation particles are "sticky" — they suppress the separation gap (no whitespace inserted adjacent to punctuation).
6. First token anchors the start, last token anchors the end. The pairs chain between them and fit together only one way.

### Decomposition (Reverse)

Decomposition is the same operation in reverse:

1. Every token in the input splits into two instances of itself — one as the right end of its left pair, one as the left end of its right pair.
2. Adjacent tokens form directed pairs (TokenA → TokenB).
3. Identical pairs are aggregated; the count becomes the FBR.
4. Whitespace (0x20 only) is stripped — it would appear in nearly every pair and make compression useless. All other whitespace characters (newlines, tabs, CR) are structural tokens preserved as particles.
5. First and last tokens are recorded as anchors.

## Compression

Common TokenID prefixes within a PBM can be factored out, noted once, and assumed for all entries — reducing per-entry storage to only the varying suffix pairs.

## Encoding Table Storage

For each modality, a covalent bonding table maps atomic-level byte codes to the modality's format system:
- **Text:** byte codes → Unicode/ASCII codepoints
- **Audio:** byte codes → frequency/amplitude representations (TBD)
- **Visual:** byte codes → spatial/color representations (TBD)

These tables are stored in `sources/` and fetched as needed.

## Use in Bridging and Error Correction

FBR data from training corpora creates bonding-strength guides at each LoD level. When a token sequence doesn't match a known rigid body (e.g., a misspelled word), the system:

1. Relaxes bonding order (letter rearrangement)
2. If unsuccessful, makes boundaries permeable
3. Uses covalent patterns to find the lowest energy-loss state (most likely correction)

This applies at every LoD level as a stacking method. See [architecture.md](architecture.md).
