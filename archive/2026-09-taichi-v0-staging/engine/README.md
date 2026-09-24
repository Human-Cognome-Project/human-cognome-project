# engine/

The field engine: the project's cosmology of thought as runnable physics.
Two halves, one contract: Phase A (kernel/ — diffusion, radiation, valency,
node forcing) x Phase B (timestep/ — the pair-rung clock, bond state,
restoration; storage/ — array-stored compositional addressing + instance
streams). SEAM.md is the living contract and the engineering history.

Doctrine compliance:
- "Python never in the hot path": the hot path is @ti.kernel code,
  JIT-compiled to native by Taichi. The Python here AUTHORS kernels and
  loads arrays; the numpy twins are the executable SPEC, bound to the
  kernels by twin-proofs at ~1e-16 max error.
- "Tests on everything": falsifier suites run in-file — F1-F9 (kernel),
  [1]-[8] (timestep), producer proofs (storage: byte-exact reconstruction,
  fail-closed). A change that breaks physics breaks a named falsifier.
- "Log big results to files": runs write JSON reports + .npz state;
  stdout carries verdicts only.
- Storage tiering: bulk run-states live on Haven (NVMe over gigabit
  outruns local spinners); manifests + chain arrays travel in-repo.
