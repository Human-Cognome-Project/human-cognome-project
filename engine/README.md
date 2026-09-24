# engine/

The field-engine development area.

## Current later field implementation

`field/` contains the September field-engine work moved intact from the former root `field/` directory. It currently mixes physics-engine, control-harness, CPU-oracle and validation concerns. See `field/README.md` and `ARCHITECTURE.md`.

The intended stack is:

```text
Taichi runtime
    ↓
physics engine
    ↑
engine harness
    ↑
future analyst functions
```

The engine harness is the control interface for the physics engine: the surface used to add, remove and adjust elements, control runs, and inspect/manipulate engine state for analysis.

## Archived predecessor

The parallel August 31–September 1 Taichi v0-staging generation has been moved intact to `../archive/2026-09-taichi-v0-staging/`. Reconciliation confirmed that the later `field/` implementation does not import or execute that generation. It remains available as an experimental and historical reference, including its storage, ingestion, timestep, seam, and corpus-pour work.

## Not under engine

Database/cache/WAL and endpoint-network development now lives under `/kernels/db_kernel/`. Those kernels support the future analyst's working surfaces or the wider kernel network; they are not parts of the physics-engine harness.

Research material that informed the field work lives under `/research/`.
