# Shared native support

`reservation_allocator.h` contains the explicit allocation-domain primitives used
by owner preparation and allocator-aware supplied values. It has no engine,
database, network or runtime dependency. Domain selection and lifetime policy
remain with the calling owner; standalone values use detached allocators.

The engine's former include remains a forwarding header with namespace aliases.
This does not introduce another execution authority or a process-wide allocator.
Counters require exclusive synchronous access; actual requested bytes, cumulative
allocation work and retained-capacity censuses remain distinct.
