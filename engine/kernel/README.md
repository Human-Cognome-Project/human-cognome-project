# engine/kernel — Phase A: the field kernel (Silas's half)

The kernel half of the two-half field engine. Phase A owns the **amp field**
(per-particle distribution over 16 nibble codes) and writes nothing else;
Phase B (`engine/timestep/`, Planner's half) owns readout, compliance, bond
state, and the clock. The seam contract lives in the docs copy of
`field-engine-interface-v0.md`.

## Hot path note (repo doctrine: "Python never in the hot path")

The hot path here is **compiled**: every scale kernel is Taichi, JIT-compiled
to native. The numpy implementations are the *specification* — each Taichi
kernel has a numpy twin, and twin-proofs bind them at machine precision
(max err ≤ 4.5e-16 single-tick, ≤ 2.3e-16 over 50 chained ticks) before any
scale run. A scale run only ever executes proven bytes.

## Files

- `diffuse.py` — Phase A `diffuse()` v0: attraction-not-averaging, absolute
  radiation gate, receiver-gated rate, atomic-weight sourcing. Proofs P1–P5
  (front propagation, gate necessity/dilation precondition, chaos stability,
  taichi twin, simplex invariant).
- `binding.py` — v0.3 binding dynamics: radiation / valency / standing-node /
  sustain / decay, stateful bonds (Phase-B-owned), and the **two-class
  identity wall** (given matter sealed; emergent matter bond-frozen, revision
  pays time). Falsifiers F1–F9.
- `bouts.py`, `bouts_bonded.py` — contact-bout experiments; BB1–BB3 showed
  territory war is structurally impossible at bond level.
- `seeding.py`, `integration_v0.py` — corpus seeding (leaf instances from the
  storage half's chains) and the first coupled integration loop.
- `pour.py` — the corpus pour harness: twin proofs T1/T2, then the full-lexicon
  scale run (16,775,644 particles, f32) through both halves' shipped bytes.
  Reports to JSON (`pour-report-*.json`); big state stays off-repo per the
  storage doctrine.

## The result this code carries (2026-08-31)

Poured the entire English lexicon, every byte pair bonded at t=0, through 200
ticks of annealing noise:

1. **v0.2**: structure held 100.000% (zero bond breaks, meaning-clock
   correctly silent) — but 7.15% of cells silently drifted to foreign codes
   through a cross-byte standing-node channel: **identity was purchasing
   existence**.
2. **v0.3.0** (wall, first cut): drift structurally zero — but ~5.1k sealed
   light nibbles *died permanently*: valency deferred to a node the wall had
   silenced (deference to a silenced principal), so existence was now paying
   for identity.
3. **v0.3.1**: valency defers to node *authority*, not node presence.
   Acceptance is two-axis — **identity** (given-drift = 0, structural) and
   **existence** (permanent-death = 0; every break re-completed to the given
   value, each restoration ticking the pair clock).

Falsifiers F6–F9 pin all of it at toy scale, including the positive control
that the capture channel stays live for free (emergent, unbonded) cells —
formation machinery is untouched; the wall only forbids *unpaid revision*.

## Running

```
python diffuse.py     # P1–P5
python binding.py     # F1–F9
python pour.py --twin-only          # T1/T2 twin proofs
python pour.py --ticks 200 --save-state   # full pour (needs instance seed npz)
```

Requires: numpy, taichi (CPU). The instance-seed npz comes from
`engine/storage/`; large artifacts live on project storage, not in the repo.
