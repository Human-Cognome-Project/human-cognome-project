# Research material

This tree holds research and exploratory material that informs HCP development but is not itself the running application.

- `field/` — field/physics research, write-ups, exploratory models and figures.
- `ledger/` — amount-ledger and scale-map research.
- `packages/` — convenience ZIP packages retained for sharing the associated research material.

The canonical runtime engine is the native C++ workspace under `engine/`, backed by the modified Taichi fork under `engine/taichi/`. Keeping that boundary explicit prevents research prototypes—including Python exploratory models—from being mistaken for runtime modules while preserving the research record in Git.
