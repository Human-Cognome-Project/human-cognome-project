# Current field-engine implementation

These files are runtime development artifacts, not the research material previously stored beside them in `/field`.

The current implementation has not yet been internally refactored to its final architectural boundaries:

- `field_engine.py` contains the later field model and also prototype run/control-harness behaviour.
- `field_engine_v0.py` is the deterministic CPU oracle for the earlier substrate.
- `field_engine_kernel_v0.py` is the Taichi/CUDA twin for that substrate.
- `field_engine_load_v0.py` prepares field elements from the database and is primarily harness-side population/input work.
- `field_engine_review.py` is an experimental validation tool that reads results after a run.

The intended boundary is:

```text
Taichi runtime
    ↓
physics engine
    ↑
engine harness
    ↑
future analyst functions
```

The engine harness is the control surface for the physics engine. Database/cache/WAL and general kernel-network infrastructure are not part of the harness.

These files are intentionally kept together in this first repository-structure pass so moves do not become simultaneous behavioural refactors.
