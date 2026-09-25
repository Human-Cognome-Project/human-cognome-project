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
  comparable. The cache manager prepares the warm cache; the analyst assembles
  an active hot SNode structure from that prepared material for the model to
  evaluate.
- Kernels are separate asynchronous functions because each has work across
  several storage tiers and locations. They can keep that work moving without
  synchronous communication gaps. This is a system-level reason for the kernel
  split, not a claim that the future analyst's functions have been designed.

## Processing analogy

- The **cold shard swarm** is the collective subconscious analogue: connections
  are made and shared beyond one instance's immediate focus.
- The **warm cache** holds prepared SNode pieces and trees oriented toward a
  field of study. The analyst composes from them the hot structure for the
  thought under consideration.
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

## Three data forms and their connections

An **SNode tree is an object definition**. It may have a self-contained
description and may connect with other definitions in arbitrarily complex
combinations. The tiers differ in the shape and preparation of the structures
they hold:

| Tier | Structure and role |
|---|---|
| Main databases | Durable object pieces and combinations created and stored by this or other processes; analogous to a library of raw objects in a game engine. |
| Warm cache | SNode pieces and trees pre-assembled by the cache manager, with assembly slanted toward the relevant field of study. |
| Hot memory / GPU transfer cache | The active SNode structure the analyst assembles from warm pieces for the particular consideration. |

Whatever defines the topic under consideration supplies the directionality
of an SNode root. The levels feed each other; the table describes their
distinct structural forms, not a claim that their exchange is only one-way.
The analyst's assembly and the exact handoffs among these tiers remain design
roles, not an implementation claim.

### Perspective-relative LoD in the working set

The SNode tree controls the level-of-detail (LoD) rollup. As the view zooms
out, its tree levels determine which perspectives the working structure
exposes. The cache manager's assembly of the working set determines which
constructs are **nested SNodes**, whose components can be broken down along
the present line of inquiry, and which are **compressed SNodes**, treated as
rollups along that line. This distinction depends on the inquiry and the
working assembly; compression here does not mean that the main database has
discarded the underlying object definition or its relationships.

Both system factors and an **analyst-defined need** guide that choice. The
form in which the analyst expresses its need is still to be determined; this
record does not prescribe an interface or selection algorithm. System
parameters also restrict how many LoD levels can effectively be kept **hot**
at once. That active residency limit affects both the granularity available
within a construct and the overall scope of data present for an analysis. It
does not set the depth of the durable object definitions. No numeric hot-set
limit or policy for allocating it is specified here. Once the base engine can
exercise this working-set behaviour, measure effective hot LoD depth alongside
granularity, scope and total data load on the available hardware. Those
measurements will help establish how and why Taichi suits this use case before
choosing operating limits. The existing
[`engine/docs/OPERATIONAL-PLAN.md`](../engine/docs/OPERATIONAL-PLAN.md) §3.3
already describes the tree as the mechanism for exposing LoD relative to a
moving observation root, and
[`storage-and-working-split.md`](storage-and-working-split.md) describes the
working tree as a per-analysis compression. This clarification connects those
mechanisms to the warm cache manager's assembly and to whether a construct is
expandable for the active inquiry.

### Address recommendations from a partial view

The analyst has only some of the direct data available while considering a
topic. Its suggested address is therefore provisional: its accuracy depends
on how much relevant data the current view contains. The existing
[`kernels/database/NOTES.md`](../kernels/database/NOTES.md) already states that
an analyst-proposed address is a position **within the composed view**, while
the cache manager disposes the actual cold-store placement. This distinction
is especially important when the warm assembly and hot structure are
selectively oriented toward one field of study. No numerical confidence or
address-revision procedure is settled by this note.

## Active thought and scale

Patrick's scaling analogy is that the **active thought area** is where
`O(log N)` calculations take place for both humans and NAPIER, relative to a
much larger available whole. In NAPIER, the analyst's hot SNode structure and
the model's evaluation of a particular thought occupy that active area; the
warm cache supplies prepared pieces, while the cold swarm continues broader
connection work outside the immediate focus.

The recovered [`storage-and-working-split.md`](storage-and-working-split.md)
already describes viewer-bounded activation, a compressed working projection,
and a discovery ledger that retires repeated work. Those are relevant
mechanisms. **`N` is defined in the calculation streams**; this discussion
record does not redefine it. We will walk those streams to check what is
already correct and prepare clarified elements for restructuring. The
biological comparison here is an analogy, not a measured complexity claim
about human thought.

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
  explicit storage from a per-analysis compressed working construct. Its older
  two-construct description does not yet spell out the warm preparation stage.
- [`engine/docs/HARNESS-STRUCTURE.md`](../engine/docs/HARNESS-STRUCTURE.md)
  provisionally describes composing an SNode tree directly from the flat store;
  its assembly path needs to be read through the main DB → warm cache → hot
  structure distinction before it is implemented as the wider harness.
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
