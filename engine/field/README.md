# Current field-engine implementation

The active September field engine is now separated by architectural responsibility while preserving `field_engine.py` as the stable CLI/import facade.

## Current modules

- `field_engine_physics.py` — the physics engine and physics-owned internal state: particle pool, deposit/relax/read/move flow, resident selection, condensation, exclusion, deterministic NumPy oracle, and Taichi accelerated twin.
- `field_engine_harness.py` — the engine control surface: population loading, run configuration, coupling controls, tick lifecycle, checkpoint/resume, observation, and mint/readout emission.
- `field_engine_validation.py` — regression/equivalence checks. Validation observes the engine but is not engine behaviour.
- `field_engine.py` — stable facade and CLI. It re-exports existing public names used by review tooling.
- `field_engine_load_v0.py` — current DB-backed population adapter feeding the harness.
- `field_engine_review.py` — experimental result review/inspection tool.
- `field_engine_v0.py` and `field_engine_kernel_v0.py` — earlier validated substrate/oracle material retained as regression references.

The architectural boundary is:

```text
Taichi runtime
    ↓
physics engine
    ↑
engine harness
    ↑
future analyst functions
```

The engine harness is specifically the control panel for the physics engine. Database/cache/WAL kernels and general kernel-network infrastructure are separate systems.

Analyst functions have not yet been designed or implemented.

## Tests

`tests/physics_smoke.py` exercises the current NumPy physics engine without a database: two identical co-resident configurations must condense deterministically into one surviving entry. CI also syntax-compiles all field-engine modules.
