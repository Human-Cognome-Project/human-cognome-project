# Engine architecture boundary

This file records the architectural boundaries being preserved during the repository reorganization. It is not a claim that the current files are already cleanly separated.

## Field-engine stack

1. **Taichi runtime** — the execution runtime used by the accelerated implementation.
2. **Physics engine** — the field dynamics themselves: the rules that determine what elements in the field do.
3. **Engine harness** — the control interface around the physics engine. It controls what the engine is asked to do and is the surface the future analyst will use to add, remove, adjust, run, reset, and inspect elements for analysis.
4. **Analyst functions** — future work. No analyst implementation exists yet.

The harness is specifically the physics engine's control surface. It is not the database manager, WAL manager, endpoint network, topology resolver, bridge layer, or thread manager.

The current `engine/field/` implementation still mixes physics, harness, oracle, and validation concerns in several files. They are kept together during the first structural pass so history and working behaviour remain intact. Internal separation is a later code refactor.

## Analyst-supporting database kernels

The PostgreSQL, cache, record-operation, and WAL functions exist to support the future analyst and keep its working surfaces current. They are autonomous kernels in the wider kernel network, but their system role is analyst support rather than part of the field-engine harness.

The preserved development bundle currently lives at `/kernels/db_kernel/`. Its internal co-location reflects development isolation and existing relative build relationships, not the final logical structure.

## Kernel network

System components are designed as loosely coupled kernels that operate at their own cadence and communicate through active inbox/outbox endpoints.

- Local counterpart connections can resolve directly to memory boxes.
- Remote/system-boundary connections use serialization/transmission bridges while preserving the same logical endpoint relationship.
- Kernels know which counterpart they address, but not whether that counterpart is local or remote.
- Data arriving in an inbox is the activation.
- A future configuration/topology routine will poll the installed environment and establish the appropriate endpoint paths.
- A future thread manager will own lower-frequency/system-facing bridge activity and activate less-frequently-needed kernels when relevant inboxes become occupied.

The physical repository layout will be moved toward these boundaries in reviewable stages rather than by rewriting working code in one step.
