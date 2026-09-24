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

## Earlier v0-staging line

The following material came from the parallel August 31–September 1 Taichi development line and is retained pending component-by-component reconciliation with the later field implementation:

- `kernel/`
- `timestep/`
- `storage/`
- `ingest/`
- `SEAM.md`

Do not treat "older" as equivalent to "discardable": storage, ingestion, test-harness, addressing and experimental artifacts may still have useful roles even where the physics model itself was superseded.

## Not under engine

Database/cache/WAL and endpoint-network development now lives under `/kernels/db_kernel/`. Those kernels support the future analyst's working surfaces or the wider kernel network; they are not parts of the physics-engine harness.

Research material that informed the field work lives under `/research/`.
