# Kernel implementations

HCP components are designed as loosely coupled kernels rather than as one centrally clocked process. Each kernel can operate at its own cadence and communicates through active inbox/outbox endpoints.

## database/

`database/` contains the PostgreSQL record tier and database/cache-manager kernel family. Its system role is to support the future analyst and keep its working surfaces current.
The [working-set and ledger guide](database/WORKING-SET-AND-LEDGER.md) connects
the built reciprocal record graph to the planned warm view composer.

## wal/

`wal/` contains the WAL manager kernel family. It is a peer of the database/cache family: it observes/books obligations and participates in the return-work flow without being owned by the database manager.
The [report-to-work guide](wal/REPORT-TO-WORK.md) maps database reports to
obligations, pending work, and the future live ingress adapter.

## Shared network substrate

`../network/endpoint/` contains the common in-memory box/endpoint/scheduler substrate consumed by both families. Future topology/configuration, serialization/transmission bridges and thread management belong under the network layer rather than under a specific kernel family.
See the [kernel activation guide](../network/KERNEL-ACTIVATION.md) for the
priority tiers and runtime role split.

These are architectural peers connected by endpoint contracts. Filesystem nesting should not be used to imply control ownership between them.
