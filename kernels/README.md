# Kernel network

HCP components are designed as loosely coupled kernels rather than as one centrally clocked process. Each kernel can operate at its own cadence and communicates through active inbox/outbox endpoints.

The current `db_kernel/` tree is a preserved development unit. It contains several concerns that were intentionally developed together to control agent drift:

- PostgreSQL record operations and schema;
- database/cache-manager work;
- WAL-manager work;
- the local endpoint/box/scheduler substrate;
- associated plans, handoffs, tests and reviews.

That co-location is not the final logical architecture.

## System role

The database/cache/record/WAL functions exist to support the future analyst and keep its working surfaces current. They are not part of the field-engine harness.

The endpoint/box/scheduler substrate is wider kernel-network infrastructure. Its role is not database-specific even though it was developed inside this bundle.

Future configuration/topology and thread/bridge managers have not yet been built. They are expected to resolve local versus remote endpoint paths and manage lower-frequency or bridged activation without changing kernel logic.

## Reorganization rule

Do not split this subtree merely for cosmetic layout. Its C++ modules currently rely on deliberate relative build relationships. The bundle will be decomposed only in a separate refactor that updates includes/build instructions/tests together.
