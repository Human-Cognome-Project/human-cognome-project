# Kernel implementations

HCP components are designed as loosely coupled kernels rather than as one centrally clocked process. Each kernel can operate at its own cadence and communicates through active inbox/outbox endpoints.

## db_kernel/

The current `db_kernel/` tree is a preserved DB/cache/WAL development unit. It still contains several concerns that were intentionally developed together to control agent drift:

- PostgreSQL record operations and schema;
- database/cache-manager work;
- WAL-manager work;
- associated plans, handoffs, tests and reviews.

The shared endpoint/box/scheduler substrate has now been promoted to `../network/endpoint/` because it is common kernel-network infrastructure rather than database-specific code.

## System role

The database/cache/record/WAL functions exist to support the future analyst and keep its working surfaces current. They are not part of the field-engine harness.

Future configuration/topology and thread/bridge managers have not yet been built. They are expected to resolve local versus remote endpoint paths and manage lower-frequency or bridged activation without changing kernel logic.

## Reorganization rule

Do not split the remaining DB/cache/WAL bundle merely for cosmetic layout. Its C++ modules still rely on deliberate relative build relationships. Further decomposition should update include paths, build instructions and tests together.
