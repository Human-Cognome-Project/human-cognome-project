# NAPIER system discussion — working record

**Status:** Discussion notes from 2026-09-25, not an implementation specification.
The overall flow below was described by Patrick; implementation details marked
deferred have not been settled by this discussion. Do not infer that a named
database or a swarm component has been built from its appearance here.

## Two linked parts

- The long-term archive aims to preserve, losslessly, reality as humans can
  encode it. Its cold shard swarm makes and shares connections among instances,
  serving a role analogous to a collective subconscious.
- The NAPIER cognitive engine draws on the archive to surf, aggregate and help
  analyze reality. It is not presented as an AI, though some functions may be
  comparable. The cache manager compiles the warm cache: the active working
  area for what the engine is considering.
- Kernels are separate asynchronous functions because each has work across
  several storage tiers and locations. They can keep that work moving without
  synchronous communication gaps. This is a system-level reason for the kernel
  split, not a claim that the future analyst's functions have been designed.

## Processing analogy

- The **cold shard swarm** is the collective subconscious analogue: connections
  are made and shared beyond one instance's immediate focus.
- The **warm cache** is the material actively under consideration, compiled by
  the cache manager. It is the working area, not itself the model or analyst.
- The **model** is the superconscious analogue. For a specific thought under
  consideration, it provides a mathematical evaluation of the interacting
  factors, analogous to the semi-direct, indirect evaluation a biological
  entity experiences while thinking about that thought. This is the model's
  active work, not merely background storage or retrieval.
- The **analyst** is the conscious analogue. Its functions have not yet been
  designed or implemented, so this analogy does not prescribe its interface
  to the model or how it directs attention.

These are roles in the proposed system, not four interchangeable names for
the same processing step. The existing
[`architecture.md`](architecture.md) places cognition in field balancing;
[`engine/ARCHITECTURE.md`](../engine/ARCHITECTURE.md) records the native field
model and keeps the analyst layer future work.

## Active thought and scale

Patrick's scaling analogy is that the **active thought area** is where
`O(log N)` calculations take place for both humans and NAPIER, relative to a
much larger available whole. In NAPIER, the warm working set and the model's
evaluation of a particular thought occupy that active area; the cold swarm
continues broader connection work outside the immediate focus.

The recovered [`storage-and-working-split.md`](storage-and-working-split.md)
already describes viewer-bounded activation, a compressed working projection,
and a discovery ledger that retires repeated work. Those are relevant
mechanisms, but the exact `N` and operation measured by `O(log N)` have not yet
been fixed as a formal performance contract. The biological comparison is an
analogy, not a measured complexity claim about human thought.

## Shared and instance-local databases

Global databases participate in the shared archive. Separately, every NAPIER
instance has a reserved private address range for its own local databases. At
least one local database is expected; the current provisional names are
`personality.db` and `relationships.db`, possibly with further local stores.
These hold that particular instance's private notes about its history,
interactions and relationships, including differentiated details such as
favourite expressions. An instance shaped through its interactions could, for
example, develop a backbone and a wry sense of humour; this is an illustration
of local differentiation, not a fixed personality preset.

Local database contents are not shared with the swarm. Significant results
drawn from them may be shareable, but the boundary and its safeguards are a
separate design matter; the discussion also noted that exceptional handling
may be needed for illegal activity. No publication rule or exception mechanism
is specified here. Local storage encryption and other guardrails are reasons
the local databases have been deferred rather than implemented as ordinary
shared shards.

**Interpretation to confirm later:** local/global describes visibility and
address scope, while cold/warm describes storage and working state. A private
record could inform an instance's working cache without itself becoming a
swarm record. This interpretation does not decide how derived results cross
the private boundary.

## Existing repo seams

- [`storage-and-working-split.md`](storage-and-working-split.md) distinguishes
  explicit storage from a per-analysis compressed working construct.
- [`network/SWARM-NOTES.md`](../network/SWARM-NOTES.md) records the preliminary
  cold/warm and p2p direction; the swarm manager is not yet built.
- [`kernels/wal/WAL-PLAN.md`](../kernels/wal/WAL-PLAN.md) §1 already names a
  personality-and-relationship database with local-only addressing. The built
  WAL report seam records local/global address scope in
  [`wal_report.h`](../kernels/wal/wal_report.h); this does not implement private
  database storage, encryption or a rule for sharing results.
- [`AGENTS.md`](../AGENTS.md) keeps analyst functions future work and the
  engine harness separate from database, WAL and network infrastructure.

The exact local database partition, encryption/key lifecycle, other guardrails
and result-release rules await a later design pass. This record can be extended
as the system discussion continues, before assigning work to core development
or expansion.
