# Field physics and tick notes

Working notes, not a specification. Patrick's statements are recorded as given;
lines marked **derived** are mine and need confirming. Gaps are marked, not
filled. Nothing has been built.

2026-09-13 — companion to `parent-structure-notes.md`.

## The governing constraint

This is an extremely complex, multi-level-of-detail analytical tool built out
of **simple math done exceptionally fast, with no fancy tricks. Pure
algorithmic math, applied without exception.**

"Without exception" is the operative half. It rules out the special case, not
just the clever algorithm. A rule that fires for some particles and not others
is a branch, a divergence, and a place for behaviour to differ from the model.
Where a quantity needs adjusting, the adjustment is applied to every
calculation identically, so that the arithmetic every particle receives is the
same arithmetic.

What this forecloses, so it does not get proposed later:

- No spatial acceleration structure. No tree, no opening angle, no accuracy
  tolerance. Interaction with a group centroid is not an approximation of its
  members, it is the model: the group is a real entity and its centroid is
  exact for what it represents. The arithmetic resembles a Barnes-Hut
  approximation and shares none of its assumptions or error terms.
- No adaptive timestep and no substepping. The brake holds the step honest
  through speed instead.
- No conditional guards, clamps or epsilon branches that fire only sometimes.
- No higher-order integrator bought to paper over a sampling problem.

What it depends on, and gets for free: the inverse square law is scale free. It
carries no built-in length, so the same arithmetic is correct at every level of
detail, and cross-level interaction needs no per-level parameter.

## The interaction law

All interactions are simple field physics as expressed by gravity:

    F = m1 * m2 / d^2

where **m2 is the centroid mass of the group, and d is the distance to the
centroid location of the group**.

The consequence is the largest structural fact in the model so far.
Interactions are **particle to group centroid**, never particle to particle. A
group of any size is reduced to one mass at one location before anything reads
it.

- Cost is memberships, not pairs. The primary pass is the number of particles
  multiplied by their memberships, not the square of the particle count.
- The scattered gather flagged earlier as unmeasured is bounded and small.
  Each particle reads one centroid per group it belongs to.
- It fits the sibling rule. A sibling relation applies to the particle's total
  mass, and there m1 is that total mass acting against the group's centroid.

## Partial response, and where the force goes

Not every relation engages the whole particle. The couplet meets a bare hex
code with the mass share of the matching position only. In that case:

**The total force is distributed across the total mass, reducing velocity or
creating spin as appropriate.**

So the responding share sets how much force is raised, and the whole particle
then absorbs it. Two things follow, and they are the standard rigid-body split
of an off-centre force:

1. **Linear motion is damped relative to the responding share.** The force is
   divided by the particle's total mass, not by the share that raised it. A
   half-responding mass-2 particle accelerates as a mass-2 body, not a mass-1
   one. Wide particles are sluggish against relations that touch only part of
   them.
2. **Off-centre force becomes spin.** A force raised at a position offset from
   the particle's centre of mass carries a moment about that centre. Only the
   component through the centre translates; the rest rotates.

"As appropriate" is then geometry doing the deciding, not a policy: a relation
that pulls symmetrically about the centre is pure translation, and the further
off-centre the responding positions sit, the more of the response is rotation.

### What this adds to particle state

Position and velocity are no longer sufficient. A particle also carries
**orientation and angular velocity**, and responds according to a **moment of
inertia**.

**Derived.** The moment of inertia is a static property of the token, not
per-instance state. Internal layout is fixed by the token's own decomposition,
element masses are known, so the second moment about the centre is computable
once per distinct token and shared by every instance of it, exactly as total
mass is.

**Derived, and it reconciles two earlier statements.** Internal geometry is
token-derived and needs no storage; the particle's *placement* is dynamic state.
Element positions in the space are the particle's position plus its orientation
applied to the token-derived internal offsets. The grid is where the internal
arrangement comes from; position and orientation are what move.

## What a dimension is

Stated, because two different things in these notes were being called by one
word and the collision produced a problem that did not exist.

**A dimension is any agreed plane of comparison between two or more points.**
That is the whole of it. x, y and z are dimensions because humans have agreed
that they are common ways to measure and interpret the space around us. The
agreement is what makes them dimensions; they hold no privilege beyond it, and
nothing about them makes a dimension spatial by nature.

**A dimension of commonality is a dimension in exactly that sense.** An agreed
basis on which two points are compared, and there are n of them. Field,
polarity, position within a parent structure: each is a plane of comparison,
each is as much a dimension as x is, and none of them is a direction anything
travels in.

**This simulation is a three dimensional visualization of field effects across
n dimensions of commonality.** The three are the visualization: the space the
spheres occupy, and where separation, force, rotation and contact are
evaluated. The n are what the field acts across, deciding what compares to
what. Neither is a metaphor for the other and neither reduces to the other.

### What that settles

The number of positions in a parent structure counts dimensions of commonality.
It has never been a count of spatial axes, so it puts no axes into the drawn
space and adds no rotation planes.

An earlier version of this file argued the opposite here, with a table giving a
five-position structure ten rotation planes and a bivector of angular state,
and called it a cost that had to be settled before any layout was fixed. It is
withdrawn. Rotation is three planes because the visualization is three
dimensional, for every particle, whatever it holds and however many positions
its parent structure has. The quadratic-per-particle worry was an artifact of
the word, not a property of the model.

## Calibration of the constant

Two unconstrained particles of mass 1 at distance 1 overlap at the centroid in
one time unit. That fixes the constant rather than leaving it free.

**Corrected, and verified by the harness.** An earlier version of this note
gave the constant as pi^2/16. That is the answer to the *pairwise* two-body
problem, and this model is not pairwise. Each particle falls toward the group
centroid, which carries the group's **total** mass and includes the subject. So
for two mass-1 particles at separation 1 the centroid holds mass 2 and sits at
distance 0.5, not mass 1 at distance 1.

Falling to a fixed centre from rest:

    t = (pi/2) * sqrt(r^3 / (2 * G * M))

With r = 0.5 and M = 2 this is t = 0.2777 / sqrt(G). Setting t = 1 gives

    G = pi^2 / 128  =  0.0771063...

exactly one eighth of the pairwise figure. `engine/tests/field_test.cpp`
integrates the two-body case and checks the meeting time lands on one time
unit, on both architectures.

## The tick

The last cycle of each tick calculates the centroids of all active fields, for
use by the following tick. So **centroids are one tick stale by design**. The
coupling is lagged deliberately, which is the physics basis stated as
mechanism: balancing is always local, always lagged, always partial.

The order that follows:

1. **Force.** Each particle gathers over its memberships, reading each group's
   centroid mass and location as computed at the end of the previous tick.
   Accumulate both force through the centre and moment about it.
2. **Integrate.** Force to linear motion over total mass, moment to angular
   motion over the moment of inertia, then apply the discretization brake to
   the resulting speed.
3. **Determiners.** The secondary set, running on the products of the primary
   calculation.
4. **Centroids.** Recompute every active field's centroid, for the next tick.

Centroids are therefore double buffered: the tick reads the previous set and
writes the next, and the two never alias.

**Why the end and not the start, in flips.** At the start of a tick everything
would be polled and calculated, and much of what was loaded is gone again
before the centroid is applied, so the positions get paid for twice. At the end
the tick already holds positions for every particle, freshly written by the
integrate pass, so the averaging for the next tick reads memory-resident data.
The consolidation is placed where its input is already hot.

That makes three readings of one decision, which is why it should never be
moved or fused for tidiness: the lag is doctrine, the separation is the double
buffer that removes the hazard, and the position at the end of the tick is what
keeps the averaging off the memory bus.

## What each pass costs, in engine terms

This answers the gather-or-scatter question left open earlier. It is both, in
different passes.

| Pass | Shape | Needs |
|---|---|---|
| Force | Gather. Each particle reads centroids it does not write. | No atomics. Read-only on the centroid buffer. |
| Integrate | Elementwise over particles. | Nothing special. |
| Centroids | Reduction over the membership edge list into per-group accumulators. | Atomics, or sort by group and segmented reduce. |

The centroid pass wants mass-weighted position sums plus a mass sum per group,
then one division per group. Two reductions over the same edge list, or one
over a widened accumulator.

## Body, corona, and contact

Active particles are **one unit spheres carrying a 0.5 unit corona, and that is
the only relevant size at any level of detail**. The sphere is one unit across,
the corona reaches half a unit beyond its surface, and the corona is where the
coupling rules take effect.

So contact happens at a finite separation between bodies, not at zero. Two
distinct particles cannot come closer than one unit between centres, the
inverse square is never evaluated at zero by two particles meeting, and the
softening question raised earlier is closed by geometry rather than by a
numerical fudge. Distance reaches zero only theoretically, on a centroid.

**Closed.** The unit spans the sphere, and the corona extends 0.5 beyond the
surface. Coupling therefore begins when coronas touch, at two units between
centres, and the bond ladder reads one, one and a half, two. This was carried
as open; it is stated.

## Self-inclusion is by design

The centroid is composed of the total mass and location **including the
subject, at all times**. A particle is never removed from the group it reads.
The self-exclusion question is answered: there is none.

With floating-point positions, a particle landing exactly on a centroid is a
negligible probability, and a minimal offset in the calculation is available if
it ever bites in practice.

**One case is structural, not probabilistic, and needs a decision.** A group
with exactly one member has its centroid exactly at that member, every tick,
deterministically. Every token that occurs only once has a self-identity group
of that shape, so this is not a rare coincidence but a guaranteed condition for
part of the population.

Separation is then exactly zero, and the resulting division is undefined.

Two rules cover it, either of which prevents the division: **if m1 equals m2
then velocity is zero**, or **if d is zero then velocity is zero**. Both look
like a test, and neither has to be written as one.

The implemented form takes the first and works it into the arithmetic. The sign
of the absolute difference between the centroid mass and the participating mass
is exactly zero when they match and exactly one otherwise, so multiplying the
force by it costs an absolute value, a sign and a multiply, runs identically in
every thread, and never branches. A group whose entire mass is this one
participation has nothing else in it to pull with, and the arithmetic says so.

It is exact rather than approximate in the case that matters: for a sole
member both sides are the same product of the same two numbers, so they agree
bit for bit.

**A softening constant is in the harness, and it should not be.**
`engine/src/field/field.h:65` carries `float softening{0.25f}`, added
unconditionally to every squared separation in the force kernel, commented as
sized to the body rather than to machine epsilon.

That comment is a back-derivation. The geometry gives a one unit sphere and a
0.5 unit corona around the entire orb; it does not give 0.25. The number is
0.5 squared, and a squared half appears only because the term is added to d^2
instead of d — which is Plummer softening from a standard N-body code, imported
whole. Nothing stated about the model asks for it.

Nor is there a case for it to cover. Two bodies are never closer than one unit
between centres, so between particles it never bites. The one place separation
is exactly zero is the sole-member centroid, and that is already exact through
the sign of the absolute mass difference, above. An unconditional constant
added to every calculation to handle a case already handled is exactly the
epsilon that "no conditional guards, clamps or epsilon branches" rules out at
the top of this file.

**So: remove it, do not size it.** The correct value is no term at all.

An earlier note here suggested guarding zero separation with a conditional and
contributing zero force. That was a special case, and it is withdrawn.

## The discretization fix

**Sole purpose:** suppress kinetic shearing caused by discretizing a continuous
effect. Uncontrolled, fine systems acquire jitter and explode. The brake exists
to stop that, and for no other reason.

**What the motion is.** A particle is finding its place. The target is where
the summed pull would set it down, so the dynamic is settling, not orbiting.

**The target.** All vectors are summed in motion, and the combined target of
the centroid vectors is what the particle moves toward. One target per particle
per tick, resolved from every membership at once, not one target per group.

**Mechanism:** an exponential adjustment of speed, based on the total distance
to that combined target.

- Negligible in any tick where the step falls a real margin short of the
  distance remaining.
- Aggressive exponential braking as the step approaches or crosses the target.

It does double duty. It stops a step from carrying a particle through a region
the continuous effect would never have let it cross, and it stops the particle
from overshooting its place, being pulled back, and overshooting again. That
ping-pong about the resting point is the jitter, and the exponential kills the
limit cycle rather than damping it slowly.

"Contact point" was loose wording, thinking in snooker geometry. The measure is
against the combined target, not against another body. That matters for cost:
the brake stays inside data the force pass already holds, and needs no
neighbour information. The force law was built to avoid neighbours and the
brake does not reintroduce them.

### This settles inertial versus overdamped

The question was a false split. With the brake, motion is inertial at range and
relaxational on approach: speed carries while the target is far, and is
progressively removed as the step closes on it. One scheme, behaving as each
regime requires, which is exactly what "finding its place" describes.

It also means the integration is deliberately non-conservative. Energy is
removed near the target by design. The system converges rather than orbits, and
nothing here should be traded away later for the sake of conserving energy.

**Why this is the right shape for the hardware.** It keeps the timestep fixed.
Overshoot is suppressed by damping speed per particle, not by shortening the
tick or subdividing it for the particles in trouble. Uniform work per tick, no
adaptive substepping, no divergence between particles in the same launch, no
variable-length inner loop. An adaptive-timestep scheme solving the same
problem would give up all four.

**Cost.** One scalar multiply per particle in the integrate pass. No new pass,
no atomics, no memory beyond what computing the step already requires.

**Resolved from the record.** An earlier version of this note claimed the brake
had a units problem: summed vectors giving force where the brake needs a
length. That was invented. What is summed is the *centroid vectors*, and their
combination is a target, which is a place. The primary attractor is where the
particle moves, and the summed vectors define that placement. The distance to
it is available by construction and needs no separate definition.

## Relative level-of-detail scoping, and what "active" means

This is a Taichi feature being used as intended, not something built on top of
it: the hierarchy supports relative level-of-detail scoping directly.

**The viewer.** There is a viewer function, the algorithmic centre of the read,
acting as a telescoping viewfinder into the mathematics. It is not a display
concept. What is in view, and at what granularity, decides what is expressed.

**The math does not change with the view.** It is the same at every level. Only
its expression differs, which is what the scale-free inverse square buys: no
per-level constant, no per-level kernel, one arithmetic for every granularity.

**Out of scope is not absent.** Anything not directly in scope is either rolled
up along the edges, represented by a coarser composite at the periphery, or
sits under the active layer, represented by its parent. So the active set is a
band in the hierarchy: coarser outside it, finer beneath it, and nothing is
discarded in either direction.

That answers what makes a field active. **Active means in scope of the viewer
at the current granularity.** Only active fields get centroids recomputed, and
the tick's work is bounded by the view rather than by the size of the store.

**At its own level of detail, everything is one particle**, regardless of how
large or complex its contents are. A composite of a billion elements is one
particle with one mass at one location, exactly like a leaf. The force pass is
therefore level-blind: it never asks what a particle contains, and there is no
aggregate case and no leaf case.

**A probe is the standard inquiry of a particle.** It is not a separate
operation with its own code path. Querying a distant structure is the ordinary
interaction, run against a thing that is, at its level, just a particle.

**A zero result means no matching field.** The test is structural, not a
magnitude threshold: either the two share a field or they do not. There is no
tolerance to tune and no cutoff to choose.

**Following is refinement, and this is what the two responses are.** Where a
suppressed zone turns out to be a primary attractor, it gets pursued. As the
centroid of the analyzed item responds to that attractor, the calculations
around it become more granular. So:

- **Incorporate** is the ordinary case. The response joins the force sum.
- **Follow** is refinement. A dominating contribution from a suppressed zone
  causes granularity to increase around the responding centroid.

**The refinement criterion needs nothing at all, not even a comparison.** A
primary attractor reveals itself because it is where the particle moves. It is
all field and force calculation, so the primary attractors are the ones
defining the particle's primary placement. There is no rank to take, no share
to threshold, no criterion to choose: the resultant already points at it.

That is the same quantity the brake measures against. The combined target of
the summed centroid vectors is where the particle moves, which is the primary
attractor, which is the direction refinement follows. One number, three uses,
computed once. Refinement costs nothing beyond deciding to act on it.

**Why a coarse particle can be trusted, and why refinement is still needed.**
Every particle is an *exact* rollup of parent effects, so a composite is not a
lossy summary and operating on it introduces no error. Refinement therefore
does not correct an approximation; nothing was approximated. What refinement
recovers is how the exact total is *distributed inside* the particle, which
starts to matter the moment an attractor engages it differentially rather than
wholly. That is the partial-response rule reaching up into the hierarchy: while
a particle responds as a whole, one mass at one point is complete; once
something pulls on part of it, its internal placement becomes load-bearing and
the granularity has to open.

This is the sharpest line between this design and a multipole or tree
approximation. Those introduce error at the coarse level and spend a tolerance
parameter controlling it. Here the coarse level is exact, and refinement is
driven by what the dynamics reveal rather than by an error budget.

### What this means for the passes

**Dense payload, sparse activation.** Particle rows stay dense and uniform, so
one compiled kernel still serves every one of them. What is sparse is *which*
rows are active. That is the sparse node path over dense leaves.

`DEVICES.md` set the sparse path aside conditionally, saying it matters only if
occupancy itself is sparse. That condition is now met. The note was not wrong;
its precondition has simply arrived, and row shape and activation turn out to
be separate questions with different answers.

**No probe stage. Correction to an earlier note here.** An earlier version of
this section described probing as a hierarchy descent needing its own
irregular, divergent pass that built a work list for the flood to consume. That
was a standard N-body pattern imported where this model does not need one, and
it is withdrawn.

Because everything is one particle at its level and a probe is the standard
inquiry, probes are already the flood. There is no second kind of work and no
second kind of code.

What *is* irregular is refinement, and it is a different thing entirely: not a
per-tick traversal but an occasional set update, expanding a rolled-up node
into its children where an attractor dominated. It touches a small subset,
happens between ticks rather than inside the force pass, and amends the
membership list rather than rebuilding it. Sparse container activation is
exactly the operation the engine provides for it.

**This is what makes depth pay literally.** The earlier caveat was that the
membership list grows with the store, so absolute cost per tick rises even
while cost per unit of analysis falls. Viewer-bounded activation removes it.
The store grows, the rollups at the periphery get better, and the active set
stays bounded by the view. Deeper data buys a better-summarized periphery for
the same active cost.

## Execution model: flood, then consolidate

The intent is to flood the card with simple calculations and wrap them up at
the end for the next tick. That fixes the shape of the launches.

**The flood is launched over memberships, not over particles.** One unit of
work is one particle against one group centroid. Every such unit is the same
handful of instructions: a subtraction per axis, a sum of squares with the
offset folded in, one reciprocal square root, a few multiply-adds. No loop, no
branch, no variable length, nothing to schedule around.

The alternative, one thread per particle looping over its own memberships, is
the wrong shape for exactly the reason the doctrine gives. Membership counts
differ per particle, so the loop length differs per thread, and threads in the
same group wait on the longest one. Launching over the edges makes every thread
identical and lets the scheduler do the balancing.

**The membership edge list is the hot structure.** It is streamed twice per
tick, once to raise forces and once to build the next centroids, with the
integrate between them. Flat, contiguous, coalesced, and the same array both
times.

**The consolidation is the centroid pass.** Everything before it is
independent; it is the only place the tick gathers itself up.

**Where a flood can stall, and the one thing to watch.** Both passes end in
accumulation: forces into per-particle sums, mass-weighted positions into
per-group sums. Done with atomics, a heavily populated group takes an atomic
add per member into one address, and those serialize. Group sizes will be
skewed, because a common token appears nearly everywhere, so the hot groups are
exactly the ones that would serialize worst.

The alternative is to hold the edge list in an order that makes each
accumulation a contiguous segmented reduction, which has no contention and
stays plain arithmetic applied without exception. It costs a second ordering of
the same edges, which is cheap in memory on the measured card. **Open:**
atomics or ordering. It does not change the model, only where the throughput
ceiling sits, and it can be decided by measurement once group sizes are known.

## Sizing the working set

There is no vertex, shader or pixel data on a particle. The payload is numeric
slots and nothing else, so even a particle carrying a few hundred force lines
is a tiny amount of real data. That also settles the line count: **a few
hundred at most**, matching the constraint already recorded in the engine's
`DEVICES.md` for rows.

Costed at the measured four bytes per four-byte slot, against 7.5 GiB usable on
the 1070:

| Lines per particle | Bytes per line | Per particle | Particles in 7.5 GiB |
|---|---|---|---|
| 64 | 16 | 1 KiB | 7.9M |
| 256 | 16 | 4 KiB | 2.0M |
| 256 | 32 | 8 KiB | 1.0M |

**The edge list is the memory, and that is the good case.** Particle state is
small: position, velocity, orientation, angular velocity, total mass, moment of
inertia. A few dozen slots. The lines are a few hundred. So the membership and
force lines outweigh the particle rows by roughly an order of magnitude, which
means the dominant structure in memory is also the one streamed linearly and
coalesced twice per tick. The biggest thing is the thing accessed most
regularly. Nothing awkward sits in the hot path.

**This is a working-set figure, not a store figure.** The device holds the
active band the viewer has in scope, projected under whatever compression suits
the budget. The storage construct on the CPU side holds everything, explicit at
every level. So the table above sizes the projection, and the compression is
what is tuned to fit it, exactly as the split intends.

**It also relaxes the slot-width question.** Eight-byte slots halve the counts
above and still leave a million particles resident at a few hundred lines each.
Precision can be chosen on what the model needs rather than on what the card
will hold.

## This host is the floor, not the target

The FX-6100 and the GTX 1070 are the development machine and the edge
proof-of-concept target. The measured envelope in the engine's `DEVICES.md` is
therefore a floor and an edge budget: what runs here runs at the edge. It is
not the shape the design should be fitted to.

**The doctrine is what makes that safe.** Plain multiply-add arithmetic gets
faster on newer silicon without being touched. Architecture-specific tricks are
precisely the things that stop working across generations: tuned tile sizes,
warp-level intrinsics, occupancy hand-balancing, anything assuming a particular
scheduler. Simple math applied without exception ports for free; a clever
kernel is a rewrite every generation.

**One measurement here must not become a permanent decision.** The f64 penalty
recorded on this card is a consumer-Pascal artifact, where double precision
runs at a small fraction of single. Datacentre parts do not carry anything like
that ratio. So slot width should be decided on what the model needs, and then
verified separately against the edge budget, rather than settled by this card's
weakness.

**Where distributed scaling will be decided.** The tick has exactly one global
synchronization point: the centroid pass at its end. Everything before it is
per-particle and embarrassingly parallel. Across devices that pass becomes an
all-reduce over group accumulators, and its traffic scales with the **number of
groups, not the number of particles**. If groups stay far fewer than particles,
the barrier stays cheap and the rest partitions freely. That ratio is the
number to watch when this leaves one card.

## Resolved from the record

Carried as open longer than they should have been, and closed by reading back
what had already been said. Recorded as readings, so a wrong one costs one
correction rather than another round of questions.

- **The combined target.** A place, by construction. See directly above.
- **The offset value.** Given as a *minimal* offset, applied if it becomes an
  issue in practice. A contingency, not a constant the specification carries.
  The harness went further and hardcoded an unconditional 0.25, which is
  imported softening with a geometric story attached after the fact. See
  "Self-inclusion is by design" above. The resolution is that there is no term,
  not that the term needs a better size.
- **Rotation dimensionality.** Rotation is three planes because the
  visualization is three dimensional. The positions of a parent structure count
  dimensions of commonality, which are agreed planes of comparison and not axes
  of the drawn space, so they add no rotation planes. The withdrawn
  dimensionality table earlier in this file ran the two senses of the word
  together. See "What a dimension is".
- **Inertial or overdamped.** A false split, already settled: inertial at
  range, relaxational on approach, one scheme.

## What is built

`engine/src/field/` and `engine/tests/field_test.cpp`, as a target linking
`engine_support` rather than living inside the engine.

State is packed structure-of-arrays in ndarrays: particles, groups, and a
membership edge list carrying its participation share and the offset that share
sits at. Parent and sibling participation are one edge shape, as derived
earlier — whole participation is share one at offset zero, which is why it
produces no spin.

**The stored share is wrong and comes out.** A share should not be held at all:
the total mass and the relative masses are already in the data points, so the
direct calculation at the point of use is the fast path and the counts stay
integers the whole way. Carrying a pre-divided fraction adds a float, adds a
rounding error and buys nothing. See `particle-geometry-notes.md`.

Six kernels, each with its own name, each with its outermost loop carrying the
runtime thread count:

| Kernel | Launched over | Shape |
|---|---|---|
| `field_clear` | particles, then groups | elementwise |
| `field_force` | **edges** | gather, atomic accumulate |
| `field_integrate` | particles | elementwise, includes the brake |
| `field_determine` | particles | elementwise, on the products |
| `field_centroid` | **edges** | reduction into groups |
| `field_publish_centroids` | groups | elementwise |

The flood is over edges, not particles, so every thread runs the same
instructions with no loop and no branch. Centroids are double buffered: the
force pass reads what the previous tick published, the centroid pass builds
into separate accumulators, and publishing divides them through.

What the tests check, on CPU and on the 1070, with the two agreeing after forty
ticks: centroid mass and location including the subject; the two-body
calibration landing on one time unit at the midpoint; a sole-member group
staying finite and not drifting; an offset share producing spin where a whole
response produces none; four hundred ticks staying finite with speeds bounded;
and the determiner stage reading the products of the primary pass.

Instances are constructed at the working base, a little over a million SNode
identifiers and sixty-five thousand trees, so the raised capacity is exercised
rather than assumed.

**Not built, because not specified.** Orientation is not integrated and spin
does not yet feed back into which positions face which neighbours. Refinement,
the viewer and the storage projection are absent; this is one active set at one
granularity. The determiner is a placeholder computing speed, present to hold
the stage rather than to mean anything.

## Open

- The step per tick. **Reading:** the tick is the time unit the calibration is
  stated in, and the brake is what makes a step that coarse safe. This is the
  one worth confirming, because if a tick is instead a subdivision of the time
  unit, the brake engages far less often than assumed.
- Whether any relation carries the opposite sign, or all are attractive.
- Whether any separation in commonality enters the force law, or whether
  commonality only selects which groups a particle reads and every distance in
  `F = m1 * m2 / d^2` is a distance in the drawn three dimensions. The grid in
  `parent-structure-notes.md` supplies a separation between pieces, and which
  of the two senses that separation belongs to has not been said.
- Whether the determiners can move particles, or only read.
