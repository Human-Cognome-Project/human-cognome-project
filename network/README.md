# Kernel network infrastructure

This tree holds infrastructure shared by autonomous HCP kernels.

## endpoint/

`endpoint/` is the built local activation substrate:

- in-memory inbox/outbox boxes;
- endpoint identity/registry;
- scheduler/readiness handling;
- request/reply endpoint contracts.

It is deliberately independent of PostgreSQL, the database/cache manager, and the WAL manager. Those kernels consume this substrate; the substrate does not belong to any one of them.

## Future modules

The following architecture is established but not yet implemented here:

- **configuration/topology** — poll the installed environment, discover available kernels/endpoints, and resolve counterpart paths;
- **local mapping** — pair locally resident inbox/outbox endpoints directly in memory;
- **serialization/transmission bridges** — preserve the same logical endpoint connection when the counterpart is remote or crosses a system boundary;
- **thread manager** — own lower-frequency/system-facing bridge activity and activate less-frequently-needed kernels when relevant inboxes become occupied.

Kernels should remain location-blind. Local versus remote transport is resolved beneath their endpoint contract.
