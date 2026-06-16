# HCP Settle — AZSL port (slice 1)

The particle **settle** ported off PhysX-CUDA to portable AZSL compute. Motive:
portability (escape NVIDIA lock-in) — graph claim
`azsl-port-methodology-fork-577-579-settled-...`. Blueprint: AtomTressFX
`HairSimulationCompute` (a PBD/Verlet constraint solver in pure compute, no
PhysX) — training corpus `docs/azsl-training-corpus/03-tressfx-settle-blueprint.md`.

## What's here

| File | Role |
|---|---|
| `SettleKernel.h` | Portable **CPU reference** — the deterministic ORACLE the shader is validated against, and the Stage-1 fabric fallback. Pure C++, no AZ/PhysX/O3DE deps. |
| `HCPSettleCompute.azsl` | The **GPU kernel** — line-for-line mirror of `SettleKernel.h`. One dispatch = one settle step over all particles. |
| `HCPSettleSrg.azsli` | The Shader Resource Group (buffers + params). |
| `test_settle.cpp` | Deterministic, GPU-free tests of the port logic. |
| `CMakeLists.txt` | Standalone test build (`ctest`). |

## Build & test

```bash
cmake -S . -B build && cmake --build build --target test_settle && ./build/test_settle
# or: ctest --test-dir build
```

## The two port-units (slice 1)

1. **Settle step** (`SettleStepOne` / `SettleStep`) — Verlet integrate movable
   (`w>0`) particles under gravity with per-step damping `exp(-damping·dt)`;
   bed (`w<=0`) particles are pinned. Contact against a per-particle `restY`
   floor clamps position and rewrites Verlet history so contact injects no
   phantom velocity (TressFX trick), with `friction` bleeding the tangential
   component.
2. **Run gate + freeze** (`RunGate`/`FreezeRun`) — a run resolves iff **all** its
   `charCount` particles have L1 velocity below the gate (exact port of the
   readback loop at `HCPVocabBed.cpp:431-453`). A settled run is frozen by
   flipping each `w<=0` so it **joins the bed**.

## Faithful constants (from live engine)

| Constant | Value | Source |
|---|---|---|
| gravity | `-9.81` Y | `HCPParticlePipeline.cpp:90`, `HCPVocabBed.cpp:138` |
| PBD damping | `0.5` | `HCPVocabBed.cpp:164` |
| PBD friction | `0.2` | `HCPVocabBed.cpp:164` |
| `DT` / settle budget | `1/60` / `60` | `HCPResolutionChamber.h:69-70` |
| gate (L1 velocity) | `< 0.5` | `WS_VELOCITY_SETTLE_THRESHOLD`, `HCPVocabBed.h:45` |

## Scope & non-goals (read before extending)

- These tests pin the port **logic** (gate, freeze, bed immobility, determinism)
  and integrator **stability** — NOT bit-exact PhysX trajectories. Per claim 608
  finding (2) the validation oracle is **resolution-equivalence** (same run →
  same `token_id` + `morphBits`), not float-identity. `token_id`/`morphBits` are
  the deterministic `runText` hash/LMDB lookup (`HCPVocabBed.cpp:441-451`);
  physics is only the binary settle gate.
- **Broad-phase is deferred.** Slice 1 takes contact as a host-supplied
  per-particle `restY`. Particle-vs-bed broad-phase (which bed particle a run
  lands on) is the next unit — blueprint `LightCulling.azsl` grid-stride +
  group-shared compaction (corpus 04 / claim 611).
- **Settle-dynamics calibration vs the live engine** (placement heights, exact
  damping/contact tuning so the AZSL gate decision matches PhysX per run) is a
  later slice — it needs the golden corpus of run→gate decisions.

## Next slices

1. Broad-phase contact (`LightCulling.azsl` blueprint) to replace host `restY`.
2. Wire `HCPSettleCompute.azsl` into an `AZ::RPI::ComputePass` + windowless
   `RenderPipeline` (corpus 02) and assert GPU output == CPU reference.
3. Settle-gate equivalence vs the live engine over the golden corpus.
