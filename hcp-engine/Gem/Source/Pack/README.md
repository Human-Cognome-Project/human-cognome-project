# Pack slice — the compact-ID packer (vocab.lmdb correction)

Standalone, GPU-free reference for the corrected GPU-facing vocab store. This is
the core job of the entry-point staging kernel (graph claim 606): turn a bounded
window of warm-set entries into the **exact** shape the GPU runs on, and nothing
more.

## What the GPU actually needs

A matcher needs two things: the **characters** to match, and the **ID** to
return when they settle. That is all. Everything else is CPU-side bookkeeping.

So the store is one map: **`compact-id → chars`**.

## Files

| file | role |
|------|------|
| `PackKernel.h` | portable CPU reference/oracle: assigns dense window-local compact ids in slot order (length-ascending), builds per-length fixed-stride char blobs, and the CPU-side `compact→canonical` ledger |
| `PackStore.h`  | the GPU-facing LMDB store: a single `MDB_INTEGERKEY` sub-db `chars` (length → contiguous fixed-stride blob). Holds chars only |
| `test_pack.cpp`| deterministic round-trip + minimality + identity-by-position checks (13, all green) |

## What it enforces (doctrine, not invented here)

- **56 see-it/mint-it** — every surface form is its own token; no runtime
  morphological reconstruction. `dog`/`dogs`, `run`/`running` are distinct
  tokens with distinct canonical ids. Morphology lives in the **definition**,
  never in this store.
- **444** — compressed token-ids in arrayed, fixed-stride layouts indexed by ID;
  no surface-form keys, no per-entry pointer/hash overhead.
- **91** — engine-shaped, not Postgres-symmetric: caps (per-length count) are
  **derived** from `blob/length`, not stored; no reverse maps or indexes.
- **606 / 613** — the GPU side is content-blind; the large canonical ids live
  ONLY in the CPU-side ledger and are reattached **by position (slot)** on
  readback. Position is the join key.

## What this replaces

The drifted `data/vocab.lmdb` carried `w2t / c2t / t2c / t2w / l2t / forward /
entities_* / _manifest / vbed_*` and stored a **14-byte token-id string +
morpheme byte** per entry. All of that is either CPU-side bookkeeping or the
superseded morph-bit design (claim 56). A correct store has **one** sub-db and
spends bytes only on characters.

## Status

- **Done + verified (this slice, in isolation):** packer, store, round-trip
  test green. `ctest` from `build/`.
- **Not done:** not yet wired into the live engine. `EnvelopeManager` still
  writes the old multi-sub-db format and `BedManager::RebuildVocab` still reads
  the 14-byte-string `w2t`. The wrong 186 MB `data/vocab.lmdb` is still on disk.
  Those are live-engine changes — next, after this oracle is signed off.

## Build / run

```sh
cd hcp-engine/Gem/Source/Pack
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make && ctest --output-on-failure
./test_pack          # per-check output
```
