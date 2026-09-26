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
- The **analyst** is the conscious analogue. It is intended to analyze the
  physics engine's raw numerical output, without relying on rendered views.
  Its functions and model-facing interface have not yet been designed or
  implemented.

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

### Parent and membership connections across LoD

For a particle, **every exposed field it belongs to has an effect on some
level on every tick**. Temporary exclusion from repeated calculation does not
remove that standing relationship or its already resolved contribution. The
two relationship axes are:

| Connection | Stored direction and meaning |
|---|---|
| Parent | `token_parent` lists the direct parent particles **in order**, with a mass for each occurrence in the piece under consideration. A parent field acts through those constituent masses and their positions; repeated occurrences retain distinct ordinals. `token_child` is the stored reverse walk from a constituent to pieces that use it. |
| Membership | `member_of` lists **all field groups a particle directly participates in**. Each group's `members` list is the reciprocal fast walk to its direct participants, analogous to `token_child` for parents. A field group is itself a token and can have its own `member_of` groups. |

**Membership is the stored group listing, not a synonym for sibling or a
declaration that every listed relationship engages the particle's whole
mass.** Sibling has a specific automatic meaning in the active model:

**`particle_id`, `token_id`, and sibling identity (Patrick, 2026-09-25):**
`particle_id` identifies a particular allocated particle instance in the
model; `token_id` identifies its base form. One `token_id` may have multiple
simultaneously exposed `particle_id` instances. Every exposed instance is
automatically grouped with **all other exposed instances of the same
`token_id`** in an always-applicable sibling field. Each instance participates
as its whole mass, and the field centroid includes the subject as it does for
every other group. This uses the same particle-to-centroid force formula; it
does not introduce a separate sibling force law. The automatic same-token
field should not be confused with the database's explicit `member_of` table
relationships. The recovered parent-structure notes already describe exact
`token_id` matches as an unconditional self-identity field, but the current
active harness does not create it automatically.

The byte-couplet rebuild provides the operative example. The value `01` can
occur across encoding tables, often by itself and sometimes in a larger
configuration. A table in which it has a **direct representation** appears in
`01`'s `member_of` list. That table can, in turn, be `member_of` groupings for
its vendor/source and format. The table's `members` list lets a traversal
follow directly back to the values it represents. The direct edge and the
recursive classification chain are distinct; the example does not imply that
every ancestor is stored as a direct `member_of` edge on `01`.

**Membership composability (Patrick, 2026-09-25):** memberships relevant to a
study must be composable into an SNode tree rooted in the concept that defines
that study. For an encoding-table analysis, **“Encoding tables”** is the root:
the included membership relationships must fit beneath it through
progressively more specific tree steps. Tables, their vendor/source and format
groupings, and represented values can therefore be reached in the context of
that root. This does not prescribe a single fixed order for those breakdowns
or require unrelated studies to sit under the same root. The cold database
keeps its direct links at each level; the warm cache assembles the relevant
paths and aggregates for the chosen study without discarding those links.

**A composed SNode element can attach at more than one locus of any SNode
tree.** Each attachment can expose it at a different point in the nested
representation, so a study-rooted view need not assign an element one unique
path or depth. Reusing composed elements across attachment points permits
arbitrarily nested levels of representation without requiring a separate
underlying object definition for each appearance.

The current schema and `Controller::add_membership` store reciprocal direct
pairs but do not check composability to a named study root. The warm assembly
and the point at which this qualification is checked remain to be developed.

**Parent field effects and repetition (Patrick, 2026-09-25):** each exposed
parent field acts on the mass of the parent occurrence it reaches. That
partial force contributes to the total motion vector of the whole construct,
distributed across its total mass. The ordered positions also permit a
**rotary alignment expression**, even when the force does not shift the
construct as a whole. For this effect, the ordered parents act **as if they
occupy a straight line across the particle**, with the construct able to
reorient that line about its centre. Rotary effects can occur in a tick,
but **rotary velocity is not preserved between ticks**. This does not prevent
an orbit produced by the continuing field forces: the particle's position and
translational motion can follow those forces over successive ticks. Parent-line
alignment responds to the current interaction without retaining a separate
spin that would keep the construct rotating on its own. If the same parent
characteristic appears at two positions, both occurrences act **distinctly**:
both contribute to the resultant vector and to alignment of the whole as
units. They must not be collapsed into one operative occurrence simply
because they share a token.

The database already preserves each ordinal and per-occurrence mass in
`token_parent`, while `token_child` correctly keeps one reverse navigation
link per distinct constituent/composite pair.

The **cold** structure spells out the relationships at every layer. The
study-oriented **warm cache** composes appropriate aggregates of their field
effects. For any particle represented at the current level, its ordered
parents supply the **next available level of LoD** if the inquiry expands that
particle. The aggregated effects remain available at the coarser level; their
underlying connections remain explicit in cold storage. This is a connection
between structural expansion and field participation, not a different force
law for each level.

The database schema and controller already store/read `token_parent` with
`token_child`, and `member_of` with `members`, as reciprocal relations. The
current C++ `field::Harness` streams generic particle-to-group edges with a
mass share and offset; it does not itself traverse those database lists or
compose cold records into warm LoD aggregates. Separate edge entries can
contribute separate force vectors to whole-mass translation, but the harness
has **no rotary-alignment calculation or orientation state**. Its existing
no-rotation comments and translation-only offset test therefore describe
what is built, not the complete parent-field behaviour clarified here. The
older drift audit rejects carried rigid-body spin; directed reorientation of
the ordered parent configuration is a different, still-unbuilt operation.
The harness uses particle array slots and group edges but stores no `token_id`
for each allocated particle and creates no automatic same-token sibling group;
its tests stage group edges explicitly. The active-set assembly still needs to
provide that mapping and group for every exposed token identity.
This note does not claim the byte-couplet/table rebuild or assembly path has
been built. The WAL manager's deferred return-path work for these
relationships is described below.

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
mechanisms. **Here `N` is the exposed fields across all particles** in the
loaded base, rather than the fixed particle-pool capacity also called `N` in
some engine notes. We will walk the calculation streams to check what is
already correct and prepare clarified elements for restructuring. The
biological comparison here is an analogy, not a measured complexity claim
about human thought.

**Calculation surface (Patrick, 2026-09-25):** keeping a construct large and
complete in representation does not require calculating every represented
field and particle on every tick. Control of which parts participate in the
current analysis is the scaling lever: the study's active relationships, SNode
granularity and temporary exclusions define the work actually performed, with
reactivation whenever a needed calculation becomes relevant. The exact control
points are being derived in the formula walkthrough. The current harness's
full pass over its loaded surface is a baseline for checking the field math;
it does not yet express the intended calculation-surface control.

### Settling after a new base is loaded

The model does not retain every possible base layout as a fixed state. On
loading a new base, its first ticks establish the current geometry while
elements find their placement. More of the exposed fields participate at this
stage, so those ticks are expected to cost more and to show greater movement
than later, settled ticks. Patrick describes the initial pairwise-calculation
series in `O(log N)` terms, with `N` defined above: its purpose is to control
the otherwise `O(N²)` pairwise work. This is about calculation, not the
number of settling ticks. As centroids stabilize, they can be excluded from
ongoing calculations so work concentrates on the relevant, still active
fields. This is a changing calculation set, not a loss of the archived data.
This is a continuing physics simulation: a single tick contributes a small
step, with its larger effects emerging over many ticks. Patrick's earlier
modeling attempts ran for **hundreds of thousands of ticks** while updating
the monitoring screen only every few hundred ticks. That screen cadence was
specific to the earlier visual monitor; it is not how the future analyst
obtains results. The analyst works from the engine's raw mathematical state
and determines how often to read it, potentially close to **one read per
tick**. Its read cadence, the simulation tick rate and the monitor's refresh
rate are distinct. Roughly **4 ms per lean tick** was an illustration of fast
repeated calculation, not a performance target or measurement for NAPIER.
Physics engines serving games at 120 FPS or more illustrate the speed of this
kind of computation, without imposing a frame schedule on this system.
Loading and highly active periods can cost more, and settling continues for
as many ticks as interactions require.

**Exclusion rule (Patrick, 2026-09-25):** on a tick, if a newly calculated
field centroid is identical to its current value, exclude that field from
subsequent calculations. A later perturbation of **any constituent** makes
the field active again and invokes its centroid calculation, even if its last
result was identical. Motion of an element within it is expected to show as
variance in the **pairwise field calculation**. The exact calculated value being
compared, where the `O(log N)` bound applies within the pairwise stream, and
which passes skip an excluded field can be pinned down as we walk the formulas.

**Compound variance:** two equal masses can move symmetrically while an
isolated mass-weighted centroid remains fixed, but actual interactions are not
monolithic. Multiple field relationships contribute to the tick-to-tick
pairwise calculation, so its variance is compound. The isolated cancellation
is a theoretical edge, not a permanent exclusion: perturbing either object
later reactivates the field. The exact calculated quantity to compare will be
checked during the formula walkthrough.

**Centroid mass (Patrick, 2026-09-25):** within a study, calculate each
centroid's aggregate mass when the base is established and reuse it while its
mass contributors remain the same. Recalculate on a change that affects those
contributors, such as adding a member. Perturbing a member can reactivate the
centroid's position/pairwise calculation without changing its mass; motion
alone is not a reason to sum its mass again. This is an initial and on-change
calculation, distinct from the tick-to-tick geometry update.

**Current formula cross-check (2026-09-25):** the C++ field harness represents
particle `p` participating in group `g` with a membership edge of mass
`m(p) * share(p,g)` at `position(p) + offset(p,g)`. The centroid reduction sums
**every edge of the group, including the interacting particle's edge**, so its
published `kCenM` is the group's full participating mass. The force pass reads
that mass (`m2`) **at the previously published centroid position** for each
member's edge; distance and direction are from the member's participating
position to that centroid. At tick end, after integration, the centroid pass
reduces updated positions and publishes the group's next position and mass;
`seed_centroids()` initializes them before the first tick. Thus subject
inclusion and the one-tick centroid lag agree with the stated sequence. With
unchanged membership, particle masses and participation shares, group mass is
independent of positions and can be reused. **Implementation difference:** the
current pass clears and recomputes mass and position for *every* group on every
tick; first/new-entry-only mass calculation and touched-centroid-only position
calculation are still design intent. The separate origin-pegged universal
field is currently suspended from the tick. The existing centroid test checks
the inclusive seeded mass and position; this review checks the tick ordering
from source, rather than asserting a new runtime timing test.

**Field-to-centroid propagation (Patrick, 2026-09-25):** for every exposed
field of an active particle, its last published centroid position supplies
the target point. The field produces a force toward that point according to
`m1 * m2 / d²`, with `m2` the field centroid's full participating mass; the
current formula gates exact coincidence and sole-participant fields to zero.
The calculation touches that centroid, which must have its position assessed
from its members at the end of the tick, after particle integration. Compare
the newly calculated position with the previously published one. If it is
unchanged, there is no **new outward change through that centroid** to wake
other members; the particle's current-tick force has still been calculated.
If the position changes, all connected particles are due to calculate against
the new centroid position on the next tick. Their field calculations touch
their centroids in turn, allowing the effect to continue across connected
fields over multiple ticks if the system requires them. As each branch's
perturbations settle and its centroids stop moving, its calculations can be
excluded again; there is no fixed number of settling ticks. Previously quiet
centroids can be touched again by later changes. This specifies propagation
of calculation work, not a second force formula. The current harness follows
the force → integration → centroid-publication order, but has no
touched-centroid flags, old/new position comparison or selective wake-up of
connected particles. It still recomputes the full loaded edge list each tick.
How simultaneous field effects compound is the next part of the formula
discussion.

**Particle exclusion (Patrick, 2026-09-25):** apply the same temporary
exclusion principle at particle scale. If a particle needs no movement on a
given tick, leave it out of subsequent particle calculations until something
touches it again. The wake-up chain is: calculate every active field on every
unresolved particle; each field calculation touches its centroids and invokes
their calculation. When a centroid changes, every particle it touches
resolves, potentially bringing its active fields into the calculation and
continuing the process through the network. A particle or centroid without a
new change can fall out of this active work again. This is the intended
propagation rule. The ripple follows the active relationships as far as the
current analysis requires; it does not imply traversing unrelated archived
structures. It is self-limiting: once a perturbation is insufficient to change
a recalculated centroid, no further particles are woken through that
centroid. As the touched particles cease needing movement and their field
centroids hold, stability propagates back through that active portion of the
network and those elements can be excluded again. The exact no-movement test
remains to be specified in the formula walkthrough, and this note does not
establish a complexity bound.

**Exclusion implication:** zero or insufficient whole-construct translation
does not alone prove a particle is resolved. A repeated parent characteristic
may still call for rotary alignment, so the eventual no-movement test must
also account for that expression before excluding the particle.

**Resolved construct as a fixed point (Patrick, 2026-09-25):** the intended
end state is that a fully resolved construct has no motion: its equations
yield no further positional changes because its relationships have found their
places. The self-limiting return to stability should follow from those
calculations, rather than from a separate programmed rule that forces the
construct to stop. Tracking and excluding unchanged particles or centroids is
an efficiency measure; it should preserve the result of calculating them.
The governing rule is to avoid repeating unnecessary work while always being
able to perform necessary work: an exclusion must be reversible as soon as a
new interaction or the current analysis calls for that calculation again.
Exact resolution may be theoretical in practice. Whether the actual field and
constraint equations reach this fixed point remains to be checked in the
formula walkthrough and measured in the running engine.

The current [`field::Harness`](../engine/src/field/field.cpp) runs its force
and centroid passes across all loaded membership edges on each tick; it does
not yet implement this shrinking active set. Its integration and determiner
passes also visit every particle on each tick. It clears and sums group mass
on every tick as part of the centroid pass, so the proposed mass reuse is not
implemented either. The older
[`field-physics-and-tick-notes.md`](field-physics-and-tick-notes.md) already
describes recomputing centroids only for active fields.

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

An instance owner's willingness to share does not set a release rule for
every instance or for other people recorded in its relationships. The
immediate project focus is preserving knowledge; private-data release and
encryption remain design work for a later pass.

**Interpretation to confirm later:** local/global describes visibility and
address scope, while cold/warm describes storage and working state. A private
record could inform an instance's working cache without itself becoming a
swarm record. This interpretation does not decide how derived results cross
the private boundary.

## WAL manager: independent bookkeeping and report ingress

The WAL manager works independently from the analyst's current-work requests.
**RECONCILE is its only analyst-issued command**: it asks the WAL manager to
promote relevant work already tracked for the cache manager into the pinned
priority box. Database WAL reports are its ongoing data input, not commands
from the analyst. The WAL manager owns the cache manager's durable deferred
work list; the cache manager carries out the work, and its resulting database
writes become further WAL reports. Swarm functions are outside this pass.

**Why this deferred work exists (Patrick, 2026-09-25):** each new direct
relationship also needs a stored path for future traversal in the other
direction. An ordered `token_parent` declaration owes a `token_child` return
from each constituent to the composite; a group's `members` declaration owes
a `member_of` return from each member to that group. Repeating these direct
follows through parent structures and group classifications gives the DB its
future n-dimensional traversal paths without searching for reverse edges.
The WAL manager books which returns are owed, delivers outstanding work and
observes the followup writes; the cache manager builds the paths. This is the
connection between its deferred-work list and the cold structure from which
warm aggregates will later be assembled.

**Current integration seam:** the existing database `Controller::mint` and
`Controller::add_membership` already write their reciprocal links immediately
in the record tier. The fixture-fed WAL kernel can book and settle return
obligations, but the live WAL-to-cache-manager route that would establish
those returns as deferred work is not built. The intended future division of
write timing and ownership should be reconciled with the existing eager
controller when this path is implemented.

The next ingress wireframe connects the participating databases' WAL streams
to the WAL manager's existing per-source inboxes:

| Stage | Contract | State |
|---|---|---|
| Database report feed | Observe changes from each relevant database, including instance-local stores, preserving each source's own order. | Live feed deferred. |
| Report-to-inbox adapter | Convert decoded change data to the existing `wal::Report` (`source`, `lsn`, local/global scope, op, decoded address footprint and relevant mass field), then deliver it to that source's inbox. Recognition reads the change data, not the initiating command. No global ordering of unrelated sources is assumed. | Wire form and live adapter still to be defined; decoded-report struct and fixture input exist. |
| WAL manager | From the report, book or settle obligations in its own durable open-obligation relation and History; push owed work to the cache manager's pending-work box. | Bookkeeping and fixture-fed endpoint push built. |
| Analyst `RECONCILE` | Select relevant existing obligations and stage them in the cache manager's normally empty priority box; their durable home stays in the WAL manager. | Routing and staging designed, not built. |
| Content tracker source | Use eligible entries in the WAL manager's PostgreSQL database as source material for the content tracker network. | Lowest priority; network facet deferred and exact source-file/manifest shape not yet settled. |

“Queue” here means the **persistent outstanding-work relation** plus transient
delivery boxes, rather than a consumed FIFO: an obligation closes when its
matching followup write appears in a report. The conversion bridge needs a
focused design for real logical decoding, source identity, and delivery/replay;
this note does not choose its wire format or transaction boundary. See
[`wal_report.h`](../kernels/wal/wal_report.h),
[`WAL-INTEGRATION-PLAN.md`](../kernels/wal/WAL-INTEGRATION-PLAN.md) and
[`ENDPOINT-ACTIVATION-NOTES.md`](../network/ENDPOINT-ACTIVATION-NOTES.md).

Patrick proposes **entry into the WAL manager's own PostgreSQL database** as
the durable handoff for these reports. The persisted entries serve both its
deferred-work tracking and the source files/records from which the content
tracker network is built. The current fixture-fed kernel receives reports
through inboxes and then writes the `history` and `obligation` tables; History
holds a decoded footprint and bookkeeping fields, not the full original WAL
record. How database entry and inbox activation fit together, and what exact
persisted representation the tracker consumes, need the focused data-shape
pass. [`network/SWARM-NOTES.md`](../network/SWARM-NOTES.md) already describes
WAL data as the network's distribution manifest/index, with its piece shape
still deferred. Instance-local contents remain private; any derived result
shared beyond an instance needs the separate guardrails noted above.

### Direction of deferred work at the private boundary

Deferred work concerning the instance-local/personal databases flows **into
those databases only**. A change in global factors can alter an instance's
personal landscape, so global-to-local followup is allowed. A private change
does not generally create deferred work that writes directly to a global
database or to another instance. A report from a local database may still be
observed by **that instance's** WAL manager for local tracking; observation
does not make the report or its resulting work shareable. Any significant
derived result that crosses this boundary needs the separate guarded release
path noted above.

This rule is **design intent, not current enforcement**. `wal::Report` carries
local/global `Scope`, and the WAL History stores it, but `wal::Obligation` has
only its identity and the fixture-fed `WalKernel` pushes every owed item into
one cache-manager out-box without a destination or scope check. The eventual
report-to-work routing must preserve enough provenance and target information
to apply the direction rule. A future tracker must not treat the WAL database's
instance-local History entries as automatically publishable.

**Priority (Patrick, 2026-09-25):** focus on preserving knowledge and on the
local report-ingress, bookkeeping and cache-manager deferred-work loop.
Private-data release rules remain to be addressed. The p2p/swarm and content
tracker path is the lowest priority and can wait until the project's scale
and available help make it relevant; it need not shape the present build.

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
  [`wal_report.h`](../kernels/wal/wal_report.h). The
  [`WAL README`](../kernels/wal/README.md) now records the directional
  instance-local deferred-work rule as a discussion note; the code does not
  enforce it or implement private storage, encryption or guarded release.
- [`AGENTS.md`](../AGENTS.md) keeps analyst functions future work and the
  engine harness separate from database, WAL and network infrastructure.

The exact local database partition, encryption/key lifecycle, other guardrails
and result-release rules await a later design pass. This record can be extended
as the system discussion continues, before assigning work to core development
or expansion.
