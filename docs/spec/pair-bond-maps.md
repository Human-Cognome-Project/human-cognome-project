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

A PBM contains enough information to reconstruct the original scoped expression:
1. Any distinct FPB or chain of FPBs can seed reconstruction.
2. FBR values guide ordering when multiple paths exist.
3. For highly similar structures, even a partial FPB chain may be sufficient.

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
