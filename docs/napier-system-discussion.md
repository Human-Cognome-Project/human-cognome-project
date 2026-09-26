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

**Kernel composition (Patrick, 2026-09-25):** discussion of one CPU kernel or
one flow at a time lays out the foundation functions and their dependencies;
it does not fix a serial execution plan or permanent one-function-per-pass
layout. These functions can be optimized, parallelized and combined as needed
for the workload, as with other systems. A later implementation may fuse or
reorder compatible work while preserving the data dependencies and
end-of-tick publication points the model requires.

**Monitor-led kernel activation (Patrick, 2026-09-25):** the runtime monitor
watches multiple mailboxes and activates less frequently needed kernel sets
when their work arrives. Primary kernels can proliferate with demand;
secondary kernel sets are threaded into the active flow as needed. This
ties kernel composability to monitored endpoint activation, rather than a
fixed list of CPU passes. The existing
[`network/ENDPOINT-ACTIVATION-NOTES.md`](../network/ENDPOINT-ACTIVATION-NOTES.md)
describes mailbox occupancy as activation and readiness-driven selection;
dynamic kernel-set scaling and assembly remain a runtime design direction.
An **active runtime balancer** is a fit for this approach: it can adapt the
active primary capacity and bring secondary work into the flow as mailbox
activity changes, instead of treating the foundation functions as a fixed
runtime lineup. Its signals, scaling policy and execution mechanism have
not been specified here.

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
  physics engine's raw numerical output exposed by the harness, without
  relying on rendered views.
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

**Planet-view analogy (Patrick, 2026-09-25):** in a game with multiple
planets, standing on one exposes its local features in detail. Positions of
the other bodies still contribute to the physics construct, appearing from
that viewpoint as stars in the sky and relative placements, although their
local features are not expanded there. The same distinction applies to a
study-rooted SNode view: nearby or topical constructs can be nested in detail
while more distant constructs contribute through compressed representations.
Rollup changes the resolution of an exposed relationship, not whether the
relationship exists. Excluding an unchanged field from redundant calculation
likewise does not erase its current contribution.

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
distributed across its total mass. At the overall-construct scale, the
movement question is whether that parent relationship has enough weight to
shift the whole construct. At the internal scale, the ordered parent
positions give the piece a **polarity-like ordering** for comparing it with
like constructs; this is an analogy for oriented comparison, not a claim
of identical physical polarity. The order can support a **rotary alignment
expression** even when the parent effect cannot translate the whole. For
this effect, the ordered parents act **as if they occupy a straight line
across the particle**, with the construct able to reorient that line about
its centre. Rotary effects can occur in a tick,
but **rotary velocity is not preserved between ticks**. This does not prevent
an orbit produced by the continuing field forces: the particle's position and
translational motion can follow those forces over successive ticks. Parent-line
alignment responds to the current interaction without retaining a separate
spin that would keep the construct rotating on its own. If the same parent
characteristic appears at two positions, both occurrences act **distinctly**:
both contribute to the resultant vector and to alignment of the whole as
units. They must not be collapsed into one operative occurrence simply
because they share a token.

**Chemical-structure analogy (Patrick, 2026-09-25):** constructs may contain
the same parent elements and total mass yet yield very different behaviour
when those elements are arranged differently, just as a chemical formula's
inventory alone does not capture organisation. Parent ordinals and positions
therefore belong to the operative comparison, not only the storage listing.

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

Patrick's scaling goal is that the **active thought area** needs work closer
to `O(log N)` than to the quadratic full-pairwise surface of a much larger
available whole. In NAPIER, the analyst's hot SNode structure and the model's
evaluation of a particular thought occupy that active area; the warm cache
supplies prepared pieces, while the cold swarm continues broader connection
work outside the immediate focus. The comparison with human thought remains
an analogy, not a measured complexity result.

The recovered [`storage-and-working-split.md`](storage-and-working-split.md)
already describes viewer-bounded activation, a compressed working projection,
and a discovery ledger that retires repeated work. Those are relevant
mechanisms. **Here `N` is the exposed fields across all particles** in the
loaded base, rather than the fixed particle-pool capacity also called `N` in
some engine notes. We will walk the calculation streams to check what is
already correct and prepare clarified elements for restructuring.

**Database as pairwise discovery-tax ledger (Patrick, 2026-09-25):** once a
pairwise relationship is discovered, record its common points and the effects
on them so later analyses can read and reuse that result instead of paying
the discovery cost again. The composed working view exposes the relationships
needed for its present calculation; effects outside that explicit exposure
can be excluded from *that calculation* on a defined basis. This is a
scope-relative exclusion, not a claim that other effects do not exist. A
newly discovered relationship, a changed working view or changed relevant
parameters can revise the exposed set and require calculation again. This
links the database ledger to the smaller active physics surface and to the
analyst's provisional conclusions from a partial view.

**Calculation surface (Patrick, 2026-09-25):** keeping a construct large and
complete in representation does not require calculating every represented
field and particle on every tick. Control of which parts participate in the
current analysis is the scaling lever: the study's active relationships, SNode
granularity and temporary exclusions define the work actually performed, with
reactivation whenever a needed calculation becomes relevant. The exact control
points are being derived in the formula walkthrough. The current harness's
full pass over its loaded surface is a baseline for checking the field math;
it does not yet express the intended calculation-surface control.

**Pairwise cost clarification (Patrick, 2026-09-25):** a complete comparison
of all pairs among `n` entities costs `Θ(n²)`. Lawfully excluding settled
particles and fields removes their pair interactions from later ticks, while
the bidirectional would-move gate and wake-up paths preserve interactions
made relevant by new changes. This is how the model aims to reduce the
necessary pairwise work toward the logarithmic active-work scale. Exclusion
changes the *active* number of interactions; its actual asymptotic bound
depends on how that number and the wake-up cost scale with the loaded base.
Removing any one participant saves its incident pairs but does not by itself
turn a dense `Θ(n²)` workload into `O(log N)`.

**Absolute predicate requirement (Patrick, 2026-09-25):** only exclude a
particle, field or predicate from future calculations when the parameters
that justify that exclusion are absolute for the defined calculation and
its exposed relationships. This can be exact within the current composed
view while remaining revisable: track each change in data, exposure or
relevant parameters that can invalidate the condition, and wake the work
when necessary.
A per-tick would-move decision can gate centroid placement and outward work
for that interaction; an approximate small-effect observation alone does not
authorise a continuing predicate exclusion. The calculation savings count
as lawful exclusions only when this stronger condition holds.

**Calculation economy (Patrick, 2026-09-25):** only calculate what changes;
when a calculation is necessary, use its result everywhere it applies. The
bidirectional field calculation can supply both its motion contribution and
the would-move decision, a flagged centroid is placed once for all connected
consumers, and an unchanged group mass can be reused across ticks. This is
the organising rule behind selective activation and the cheap propagation
gate, not a separate force law.

### Settling after a new base is loaded

The model does not retain every possible base layout as a fixed state. On
loading a new base, its first ticks establish the current geometry while
elements find their placement. More of the exposed fields participate at this
stage, so those ticks are expected to cost more and to show greater movement
than later, settled ticks. Patrick describes the initial pairwise-calculation
series as the point where exclusions begin controlling the otherwise
quadratic pairwise work. A newly loaded base can have many active fields;
as centroids stabilize, they can be excluded from ongoing calculations so
work concentrates on the relevant, still active fields. The logarithmic
description is the intended reduction in ongoing *necessary work*, not a
bound already shown for the initial ticks or a claim about the number of
settling ticks. Continuing exclusions require absolute predicates and a
wake path for any later invalidation. This is a changing calculation set,
not a loss of archived data.

**Simulation scale (Patrick, 2026-09-25):** the physics engine is meant to
perform these calculations quickly and repeatedly. A single tick is a small
step; the criterion is the correct overall behaviour that develops across
many ticks, rather than a significant result on any one tick. The local
update rules still matter because their effects accumulate. Patrick's
earlier modeling attempts ran for **hundreds of thousands of ticks** while
updating a browser-rendered monitor every few hundred ticks. That view is a
human-readable interpretation of the math for inspection. The future analyst
instead analyzes the engine's numerical output. The harness controls at
runtime which results leave device memory (VRAM on a GPU) and when; the
selection and readback cadence can change during a run. It might expose
results nearly every tick when needed. The monitor's refresh schedule does
not limit that access. In the game-design analogy, a displayed frame needs
only the physics results relevant to that view, even while the engine
calculates more to support it. Here the harness exposes selected numerical
results for the current analysis while the engine can work on a wider active
field set. Selecting which results to transfer is distinct from excluding
settled fields from calculation.

More active periods can take longer per tick, and settling continues for as
many ticks as the interactions require. No fixed tick-time or frame-rate
target is specified here.

**Readback implementation seam:** `field::Harness::tick()` does not download
results. Its explicit `download()` synchronizes the runtime and reads the
whole particle, group and determiner arrays into host memory. This provides
an on-demand full-array transfer: its caller can choose when to invoke it,
but the harness cannot yet select particular results for readback or control
their cadence through a runtime setting. The intended harness control and
the future analyst's data interface remain to be built.

**Exclusion rule, clarified (Patrick, 2026-09-25):** the bidirectional motive
force expression in each field interaction supplies a *would effectively
move this centroid* decision. If no interaction sets that flag on a tick,
skip placing the centroid and skip new outward work through it. A later
interaction can set the flag again. This is a per-tick placement gate; keeping
that field excluded from future pairwise work additionally requires the
absolute predicate specified above. The earlier description required an
exact post-placement equality comparison to decide activity; that reading
is superseded by the force-ratio trigger below. The ratio of movement
expression across the interacting masses is the test for an effective
centroid effect. Its numerical significance at the active study's resolution,
where the `O(log N)` bound applies within the pairwise stream, and which
passes skip an inactive field can be pinned down during implementation.

**Compound variance:** multiple field relationships can affect the same
centroid on one tick; combine their would-move decisions with an any-active
rule, then place a flagged centroid once from its members. Their actual
motions can compound or cancel in the resulting position. This activation
rule permits some flex instead of requiring an exact post-placement equality
test solely to decide whether propagation is allowed.

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
tick; first/new-entry-only mass calculation and would-move-flagged position
calculation are still design intent. The separate origin-pegged universal
field is currently suspended from the tick. The existing centroid test checks
the inclusive seeded mass and position; this review checks the tick ordering
from source, rather than asserting a new runtime timing test.

**Field-to-centroid propagation (Patrick, 2026-09-25):** for every exposed
field of an active particle, its last published centroid position supplies
the target point. The field produces a force toward that point according to
`m1 * m2 / d²`, with `m2` the field centroid's full participating mass; the
current formula gates exact coincidence and sole-participant fields to zero.
**Correction to the trigger (Patrick, 2026-09-25):** each field effect is
bidirectional. During that interaction, the ratio in which its motive force
is expressed across the participating masses already answers the *boolean*
question: would this interaction effectively move the centroid? If the
smaller mass absorbs the effect, it does not flag the centroid. **Scale
example (Patrick, 2026-09-25):** a person moving against the mass of Earth
has an insignificant share of motion at the whole-Earth centroid; that
interaction should not wake calculations across the entire aggregate. The
movement ratio gives this answer during the field calculation, without
placing the centroid to measure its actual displacement. Accumulate
these answers across the tick: **if any interaction says yes, mark that
centroid for placement**. No interaction calculates or stores how far the
centroid will actually move. At tick end, after particle integration, place
each marked centroid once from its members' updated positions and publish
its new position. Its connected particles can use that position on a later
tick, continuing the calculation through their other fields. With no
would-move flag, keep the published centroid position and do not schedule
further work through it from these interactions. The force calculation's
existing ratio supplies the trigger; a second position comparison solely
for activation is unnecessary. The gate adds a small decision to a field
calculation that is already needed, saving end-of-tick centroid reductions
and subsequent connected work when no interaction flags that centroid.

**Effective stability, not exact immobility:** this gate is intended to stop
propagation once a centroid is effectively stable, while allowing some flex
within the construct. A change to one included member can shift the exact
mass-weighted result even when that interaction does not set a would-move
flag; the gate does not promise exact equality of the two centroid positions.
The previous note confused the early yes/no trigger with end-of-tick
placement and treated effective stability as exact immobility. The governing
test is the movement-expression ratio; the implementation still has to set
what counts as a significant effect at the current model resolution. It
needs no predicted centroid position or arbitrary ripple depth.
The current harness follows force → integration → centroid publication but
has no per-centroid would-move flags, conditional placement or selective
wake-up: it recomputes the full loaded edge list every tick.

**Per-particle superposition (Patrick, 2026-09-25):** each exposed field gives
the particle a destination at its relevant centroid and a directed pull
toward it, scaled by `m1 * m2 / d²`. Across the particle's fields, the
destinations and pulls must combine into **both an effective destination and
a motive force for that tick**. Each interaction has an intended endpoint
relative to its `m2` and a vector toward it whose magnitude follows the
inverse-square expression. **Sum the directed radial force vectors** to
produce the resultant pull. Patrick expects the same contributions to yield
the combined intended destination and its distance, with a balanced particle
at the positions appropriate to all its interacting masses and their
characteristics. Whether the endpoint operation is a sum, mean or other
normalisation is **not yet settled**; Patrick presently expects summation
but wants the radial-vector expression checked. The earlier
`engine/docs/DRIFT-AUDIT.md` §3 and `docs/field-physics-and-tick-notes.md`
define the destination as position plus the summed force vector. Keep that
as a candidate while checking that its distance is the intended brake reach.

**Optional algebraic consistency check (assistant derivation, not approved):**
let `x` be the particle position and `q_i` the field's centroid point
expressed as a target for that particle position (accounting for an edge's
participation offset). For positive nonzero distances, write the current
inverse-square vector as
`F_i = a_i * (q_i - x)` with `a_i = G * m1_i * m2_i / |q_i - x|³` (including
the field's engagement/share terms). Then `F = Σ F_i`, `A = Σ a_i`, and,
when `A > 0`, one possible averaged destination
`T = (Σ a_i * q_i) / A = x + F/A` satisfies `F = A * (T - x)`. This would
place the destination along the net pull and give a remaining distance
`|F|/A`. It is only an alternative to test against the simpler `x + F`
reading, **not a settled rule or a second target to implement now**. Compare
their behaviour for balanced, unequal and repeated fields before deciding.
This algebra covers the attractive field pass; other contributions such as
the contact pass need separate treatment when evaluating the whole motion.
The current `field_force` accumulates only `kForceX/Y/Z`, while
`field_integrate` uses `|F|` for its brake reach. Parent-line rotary
alignment remains distinct work to implement.

**Discretization brake (Patrick, 2026-09-25):** its purpose is to prevent
kinetic shearing caused by finite simulation ticks. The fields are intended
to balance continuously, with their kinetic expression settling a particle
into position. A discrete tick can carry it past that balancing target;
small oversteps followed by reversals can compound across ticks into motion
that the continuous field interaction would have damped. The brake addresses
this numerical instability by dampening the overstep's kinetics directly,
without adaptive time steps, substeps or a more complex temporal correction.
After a particle's field contributions have produced its combined destination
and motive force, the brake compares proposed travel for that tick with the
remaining distance to that destination. It should have an effect near unity
while the step stays comfortably short of the destination. It can slow an
approach tick and should brake aggressively if the unbraked step would
overshoot.
Later ticks can continue settling toward the exact effective centroid; the
brake need not put the particle at the target in one step. This is a
correction to the discrete step, applied to the particle's combined motion;
it is not another attracting field. The exact transition close to the target
and the construction of the combined destination are still being clarified.

**Current brake formula:** `field_integrate` first calculates a proposed
velocity from carried velocity plus `dt * F_net / mass`. It calls the
proposed travel `travel = |velocity| * dt`, uses `reach = |F_net|`, and
multiplies the proposed velocity by `exp(-(travel / reach)^4)` when reach is
nonzero. At travel/reach ratios of 0.5, 0.8 and 1, that multiplier is about
0.94, 0.66 and 0.37, respectively. It therefore reduces velocity before an
overshoot and does not land exactly at the destination on that tick. That
early attenuation can be consistent with the intended multi-tick settling,
provided subsequent ticks converge. A zero reach leaves this brake at unity.
A separate outward radial dampener runs after the brake; it is not the
discretization correction. The code implements a smooth exponential brake,
but whether its reach is the **actual distance to the combined effective
destination** and whether the resulting trajectory converges as intended
need rechecking once the destination composition is settled. Current tests
check near-meeting within a tolerance and boundedness over 400 chaotic ring
ticks; neither establishes exact convergence to the effective centroid.

**Brake-slope tuning candidate (discussion, 2026-09-25):** Patrick suggests
steepening the exponential if it suppresses legitimate approach motion too
early, while keeping a strong slow wall at an impending overstep. With
`r = proposed_travel / reach`, the present multiplier `exp(-r^4)` is about
0.66 at `r = 0.8`; `exp(-r^16)` would be about 0.97 there. Every form
`exp(-r^p)` still equals `1/e` at `r = 1`, regardless of `p`; increasing
the exponent sharpens the transition but does not eliminate attenuation
at that boundary. For `p >= 1` and an aligned displacement toward a
correctly identified destination, the corrected distance `r * exp(-r^p)`
is below one reach, even if the proposed step exceeds it. This makes a
larger exponent a plausible tuning change without a new temporal scheme.
That bound applies to the brake's chosen reach; it cannot establish
overshoot protection relative to the *actual* combined destination until
the relationship between `|F_net|` and that destination is resolved.
Treat the exponent as a candidate to measure against settling trajectories,
not as a settled change to the shipped formula.

**Model-design criterion (Patrick, 2026-09-25):** NAPIER derives its field
model from common physics principles to serve the behavior this system needs;
it does not aim to reproduce one particular existing physics model. Choose
the brake's curve, and other numerical details, by how well they preserve
the intended field relationships, settling and tractable computation across
many ticks. The exponential slope is a tuning choice within that derived
model, not a physical constant to inherit unchanged from elsewhere.

**Wake propagation latency (Patrick, 2026-09-25):** activation can take
several ticks to ripple through connected particles and centroids. A
previously resolved particle may have its next movement deferred a few ticks
until the change reaches it. Against a fast simulation running for very many
ticks, that short delay is intended to be a nominal disturbance in the
overall physics behaviour; the calculation does not require every connected
particle to move on the tick when the initial perturbation occurs. When the
active-set path is built, check both eventual wake-up and the resulting
trajectories. This is a design expectation, not a measured error bound.

**Particle exclusion (Patrick, 2026-09-25):** apply the same temporary
exclusion principle at particle scale. If a particle needs no movement on a
given tick, leave it out of subsequent particle calculations until something
touches it again. The wake-up chain is: calculate every active field on every
unresolved particle; each bidirectional field calculation supplies its
centroid's would-move decision. Mark a centroid for end-of-tick placement if
any interaction flags it; its connected particles can then calculate against
the published position on a following tick, continuing through their active
fields. If no interaction flags it, do not schedule that placement or outward
work. The ripple follows active relationships as far as the analysis
requires, without traversing unrelated archived structures. It limits itself
when the force-expression ratio no longer flags further centroid effects;
small unpropagated flex is allowed. As particles cease needing movement, they
can also be excluded until a later interaction wakes them. The exact
particle-level no-movement test remains to be specified, and this note does
not establish a complexity bound.

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

**Formula points still to derive or check (2026-09-25):**

- The directed radial pulls sum into a resultant. Check how their intended
  endpoints yield the combined destination (sum, mean or normalised sum), and
  therefore the brake's remaining-distance reference. The current `|F_net|`
  reach has not been shown to equal distance to that destination.
- The centroid's would-move gate follows the interaction's ratio of movement
  expression across masses; set its effective significance for the current
  model resolution. The particle-level no-movement rule must also cover
  parent-line reorientation. The any-active accumulation and end-of-tick
  centroid placement are already clear.
- Specify how ordered parent occurrences reorient their line in response to
  distinct partial forces, including repeated characteristics, and how that
  polarity-like ordering participates in comparison of like constructs.
  Reorientation does not carry angular velocity between ticks.
- State what operation the `O(log N)` claim bounds when `N` is all exposed
  fields. A pass that reads each of those `N` fields individually has at least
  linear total work; logarithmic depth, lookup or active work may be a
  different measure. Show that continuing exclusions are exact for the
  calculation's exposed relationships, reactivate when that exposure
  changes, and give the claimed active-interaction and wake-up cost for the
  workloads of interest.

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
