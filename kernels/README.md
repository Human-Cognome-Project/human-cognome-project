# Kernel implementations

HCP components are designed as loosely coupled kernels rather than as one centrally clocked process. Each kernel can operate at its own cadence and communicates through active inbox/outbox endpoints.

## database/

`database/` contains the PostgreSQL record tier and database/cache-manager kernel family. Its system role is to support the future analyst and keep its working surfaces current.

## wal/

`wal/` contains the WAL manager kernel family. It is a peer of the database/cache family: it observes/books obligations and participates in the return-work flow without being owned by the database manager.

## Shared network substrate

`../network/endpoint/` contains the common in-memory box/endpoint/scheduler substrate consumed by both families. Future topology/configuration, serialization/transmission bridges and thread management belong under the network layer rather than under a specific kernel family.

These are architectural peers connected by endpoint contracts. Filesystem nesting should not be used to imply control ownership between them.
