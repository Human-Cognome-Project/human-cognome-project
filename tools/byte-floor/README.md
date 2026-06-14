# byte-floor

Stage-1 of the resolution system: a raw **byte stream → characters** resolver. This is the
floor the rest of resolution stands on, and the piece the production tokenizer didn't have
(it was ASCII-byte-level with hardcoded UTF-8 patches; `LookupChar` was single-byte).

Doctrine: HCP claims 569 (candidate-filter cascade, errors-as-granularity) and 570 (stage-1
self-discrimination, broad-phase by mutual exclusion). Standalone C++ reference; the hot
passes are AZSL-port candidates (claim 579) for the GPU compute path.

## What it does

Given a raw byte buffer, with **no trust in headers**:

1. **Size + endianness** from the null histogram — 0x00 is the key discriminator (excluded
   from text content, so its presence and position are pure structural signal): null
   fraction → 1/2/4-byte units; null position parity / mod-4 offset → endianness.
2. **Table** by self-discrimination / mutual exclusion — a UTF-8 validity scan separates
   ASCII (decode-identical superposition held, not forced) / UTF-8 / Latin-1; 2-byte → UTF-16,
   4-byte → UTF-32. Min-violation settle when evidence is mixed.
3. **Decode** to Unicode codepoints. Undecodable bytes are **not errors** — they are emitted
   as byte-granularity **residue** (the granularity gradient), resync-and-continue, never dropped.

Every discrimination is a uniform predicate over a flat array (null histogram, validity scan)
— memory-bandwidth-bound reductions that map directly to AZSL ComputePasses. The CPU reference
keeps them as discrete passes so the GPU port is mechanical. `std::vector`/`std::string` appear
only in CPU-side result/evidence reporting, never in a hot pass.

## Build & run

```
cmake -S . -B build && cmake --build build
./build/test_bytefloor      # crafted streams, known answers (11/11)
./build/bf <file>           # resolve a real file, report discrimination + decode
```

## Vet status

`test_bytefloor`: 11/11 — ASCII, UTF-8 (2/3-byte), UTF-16 LE/BE, UTF-32 LE/BE, Latin-1
(mutual-exclusion vs UTF-8), UTF-8-with-residue, and two BOM cases reached from *content*
(not trusted as authority). Verified on real markdown docs (UTF-8, multibyte decoded, zero
residue) and a synthetic UTF-16LE file. Deterministic by construction (pure function, no ordering).

## Not built here (next stages)

Word resolution (chambers), the positional map, the GPU/AZSL port of the hot passes, and the
parallel-prefix boundary-finding the UTF-8 decode needs for a GPU dispatch (the CPU reference
decodes sequentially).
