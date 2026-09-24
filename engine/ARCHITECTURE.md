# Engine architecture boundary

This file records the architectural boundaries preserved during the repository reorganization.

## Field-engine stack

1. **Taichi runtime** — the execution runtime used by the accelerated implementation.
2. **Physics engine** — the field dynamics themselves: the rules that determine what elements in the field do.
3. **Engine harness** — the control interface around the physics engine. It controls what the engine is asked to do and is the surface the future analyst will use to add, remove, adjust, run, reset, and inspect elements for analysis.
4. **Analyst functions** — future work. No analyst implementation exists yet.

The harness is specifically the physics engine's control surface. It is not the database manager, WAL manager, endpoint network, topology resolver, bridge layer, or thread manager.

The active `engine/field/` implementation now separates physics (`field_engine_physics.py`), harness/control lifecycle (`field_engine_harness.py`), and validation (`field_engine_validation.py`) while retaining `field_engine.py` as the stable facade. The earlier v0 substrate remains as regression/reference material.

## Analyst-supporting database kernels

The PostgreSQL, cache, record-operation, and WAL functions exist to support the future analyst and keep its working surfaces current. They are autonomous kernels in the wider kernel network, but their system role is analyst support rather than part of the field-engine harness.

Database/cache work lives under `/kernels/database/`; the WAL manager is its peer under `/kernels/wal/`. Shared endpoint/box/scheduler infrastructure lives under `/network/endpoint/`.

## Kernel network

System components are designed as loosely coupled kernels that operate at their own cadence and communicate through active inbox/outbox endpoints.

- Local counterpart connections can resolve directly to memory boxes.
- Remote/system-boundary connections use serialization/transmission bridges while preserving the same logical endpoint relationship.
- Kernels know which counterpart they address, but not whether that counterpart is local or remote.
- Data arriving in an inbox is the activation.
- A future configuration/topology routine will poll the installed environment and establish the appropriate endpoint paths.
- A future thread manager will own lower-frequency/system-facing bridge activity and activate less-frequently-needed kernels when relevant inboxes become occupied.

The physical repository layout will be moved toward these boundaries in reviewable stages rather than by rewriting working code in one step.
