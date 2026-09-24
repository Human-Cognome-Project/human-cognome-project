# June 2026 worktree-agent record

This directory preserves the one unique commit that remained on the historical
`worktree-agent-a9f49486` branch after the September repository reconciliation.

The two documents are retained as design/history, not current architecture:

- `prime-db-functions.md` — a June concept-substrate design snapshot expressing
  NSM primes as database-function signatures. It explicitly described itself as
  design-not-built at the time and predates the current kernel/database structure.
- `skavysh-physics-lens.md` — a tailored physics-first project introduction from
  an earlier O3DE/PhysX-era architecture. Several implementation claims in it are
  superseded by the current Taichi field-engine architecture.

They are archived verbatim so the historical branch can be treated as redundant
without losing its unique authored material. Current architecture is documented at
the repository root and under `engine/`, `kernels/`, and `network/`.
