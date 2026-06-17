# byte-floor

Stage-1 of the resolution system: a raw **byte stream → characters** resolver. This is the
floor the rest of resolution stands on, and the piece the production tokenizer didn't have
(it was ASCII-byte-level with hardcoded UTF-8 patches; `LookupChar` was single-byte).

Doctrine: HCP claims 569 (candidate-filter cascade, errors-as-granularity) and 570 (stage-1
self-discrimination, broad-phase by mutual exclusion). Standalone C++ reference; the hot
passes are AZSL-port candidates (claim 579) for the GPU compute path.

## What it does

Discrimination runs on an **adaptive sample** (claim 617): the probe grows (256→1K→4K→…→cap)
only until the structure is decisively resolved — multibyte nulls, or a clean 1-byte table. A
pure-ASCII prefix (table still a 3-way superposition) or mixed/min-violation evidence isn't
confident yet, so the probe keeps growing; at the cap it settles whatever it has (= a full scan).
The decode always covers the whole buffer. This is the philosophy's resolved fork — narrow by
structure on a growing sample, don't eagerly scan everything.

Given a raw byte buffer, with **no trust in headers**:

1. **Size + endianness** from the null histogram — 0x00 is the key discriminator (excluded
   from text content, so its presence and position are pure structural signal): null
   fraction → 1/2/4-byte units; null position parity / mod-4 offset → endianness.
2. **Table** by self-discrimination / mutual exclusion — a UTF-8 validity scan separates
   ASCII (decode-identical superposition held, not forced) / UTF-8 / Latin-1; 2-byte → UTF-16,
   4-byte → UTF-32. Min-violation settle when evidence is mixed.
3. **Decode** to Unicode codepoints. Undecodable bytes are **not errors** — they are emitted
   as byte-granularity **residue** (the granularity gradient), resync-and-continue, never dropped.
   Each object carries its **source span** (offset+len) — the positional map (claim 569 output 2):
   spans tile the stream exactly, so objects are addressable and the source reverse-walks losslessly.

`resolve()` returns the single best manifest. **`resolveManifests()`** is the paint-all fallback
(claim 617): when structure genuinely can't separate interpretations that *decode differently* (a
real endianness tie, or a balanced UTF-8-vs-Latin-1 split) it returns the **bounded set** of
manifests — one full decode per surviving interpretation — to be collapsed downstream by match.
Decode-identical ties (e.g. pure ASCII) stay one manifest carrying the candidate tags.

Every discrimination is a uniform predicate over a flat array (null histogram, validity scan)
— memory-bandwidth-bound reductions that map directly to AZSL ComputePasses. The CPU reference
keeps them as discrete passes so the GPU port is mechanical. `std::vector`/`std::string` appear
only in CPU-side result/evidence reporting, never in a hot pass.

## Build & run

```
cmake -S . -B build && cmake --build build
./build/test_bytefloor      # crafted streams, known answers (25/25)
./build/bf <file>           # resolve a real file, report discrimination + decode
```

## Vet status

`test_bytefloor`: 25/25 — the 11 self-discrimination/decode cases (ASCII, UTF-8 2/3-byte, UTF-16
LE/BE, UTF-32 LE/BE, Latin-1, residue, content-BOM); 4 adaptive-sample checks (stop early on clean
type, grow past an uninformative ASCII prefix, all-ASCII to the cap, deterministic); 6 positional-map
checks (spans tile + reverse-walk losslessly across every path, é→bytes[3,2)); and 4 paint-all
checks (confident→1 manifest, balanced UTF-8/Latin-1 and ambiguous endianness each→2 differing
manifests, pure ASCII→1 tagged). Verified on real markdown docs and a synthetic UTF-16LE file.
Deterministic by construction (pure function, no ordering).

## Not built here (next stages)

Word resolution (chambers), the positional map, the GPU/AZSL port of the hot passes, and the
parallel-prefix boundary-finding the UTF-8 decode needs for a GPU dispatch (the CPU reference
decodes sequentially).
