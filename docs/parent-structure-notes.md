> **Recovered design record (2026-09-24).** This note was authored during the native C++ engine/harness work before that local workspace was fully pushed. Statements about what was "not built", file locations, or open work reflect the date/context of the note. For current implementation status, read `engine/docs/README.md` and `engine/docs/OPERATIONAL-PLAN.md`; current repository policy is in `AGENTS.md` and `CONTRIBUTING.md`.

# Parent and sibling structure notes

Working notes, not a specification. Everything below is Patrick's, recorded as
stated, except where a line is marked **derived**, which means I drew it out of
what was stated and it needs confirming. Gaps are marked as gaps and are not
filled by inference. Nothing here has been built, and nothing here authorizes
building.

2026-09-12 — from the particle-structure discussion.

## What a particle is

A particle is a pivot table keyed to its identifying token. The affective
field data for that particle is the structure attached to that key. Its shape
is specific but **variable**: the calculations it carries are not the same in
number from one particle to the next.

This is the reconciliation of "absolutely consistent shape" with "variable
shapes". The *schema* is consistent, so every operation is relative and one
compiled kernel can serve the whole set. The *extent* is not consistent, so a
particle is a variable-length run of lines rather than a fixed-width row.

## Where the relationships live

Each data point, that is each `token_id` in the SNode tree, carries its own
parent and sibling relationships. Those relationships define the field effect
the point participates in.

The relationships are held **as data at the token**, not as the topology of the
tree. The tree is storage and level-of-detail rollup over internal component
structure, and any resemblance between the assembled tree's shape and a field
structure is incidental and not to be relied on.

## Total mass

Each particle has a total mass. Its full definition is deferred to the
particle-structure discussion.

What is fixed already: **total mass is cumulative, not multiplicative.** It is
a pure rollup of algorithmic effects into summary expressions, accumulated from
the parent masses.

The rollup is **exact**. A particle carries the exact rolled-up effects of its
parents, so a composite is a complete statement of its contents at its own
level, not a lossy summary of them. Nothing is approximated on the way up, and
so nothing is corrected on the way down; refinement recovers internal
distribution, not accuracy. See `field-physics-and-tick-notes.md`.

This is the same rollup the SNode tree exists to carry. Mass is what the
level-of-detail rollup over internal component structure produces, so the
rollup pass and the mass definition are one concern, not two.

**Reading, from the record.** Effects and expressions were both given in the
plural, and the rollup is of *algorithmic effects into summary expressions*. So
it is per-effect accumulation, each effect rolling into its own summary, with
total mass the cumulative one among them, rather than a single undifferentiated
sum.

## The two kinds of relationship

**Parent and sibling are not two kinds of container. They are two modes of
participation.** The same table can be a parent relationship to one member and
a sibling relationship to another. What decides it is which part of the
particle joins:

- A table that a **component** is part of is a **parent** relationship.
- A table that the **whole** is part of is a **sibling** relationship.

| | Participation | Applies to |
|---|---|---|
| **Parent** | The particle joins through a component | Specific positional masses, selected by linear placement |
| **Sibling** | The particle joins whole | The total mass of the particle |

This is the same rule stated earlier from the other side: sibling relations act
on total mass, parent relations act on positional masses. Asking whether some
container "is" a parent structure is the wrong question, and it was asked twice
here before the answer landed. Nothing in the data model types a group. A group
is a group; the membership carries the mode.

### The consequence for the edge list

A membership edge therefore carries its participation mode: which positions of
the member take part, and at what relative mass.

**Derived.** The two modes need not be two structures. Whole participation is
component participation whose selector covers every position and whose relative
mass is one. On that reading there is a single edge shape, the force kernel
never asks which kind it has, and the partial-response rule handles both: force
divided by total mass either way, moment about the centre either way, which
comes out as pure translation with no spin when participation is whole and
centred. One structure, one code path, no branch — which is what the governing
constraint asks for.

## Sibling groups

**The definition, as given:** a sibling group membership is a gathering within
a group, bounded by any larger groups.

So a sibling group is never free-floating. It is a subset scoped by a
containing group, and that container may itself sit inside a larger one. The
bounding is what keeps a gathering local instead of global.

A particle holds **many** sibling group memberships, not one. Those identified
so far:

1. **Self-identity.** Exact `token_id` matches are always a field, for every
   particle, without exception. The sibling set is never empty.
2. **Table membership.** A particle is a direct member of every table that uses
   it. Each such table is listed as a sibling group.
3. **Representational variation.** Any variation in what the particle
   represents under one of those tables is itself a sibling group.
4. **Sub-groupings inside a parent structure.** Positions within the particle's
   own constituent arrangement that gather together, as in the five-element
   example below.

All four fit the one definition: a gathering, bounded by something larger.

Because a sibling relationship applies to the particle's total mass, each of
these memberships engages the whole particle, not a share of it. That is the
opposite of the parent side, where a relation reaches only the positional
masses it names.

## Forces predicated on parents

Forces whose effect is predicated on parent structures are listed in the
particle's attached data. For each such force there is one line carrying:

1. **Which force** it is.
2. **Which parent mass or masses** it applies to. One line may reference more
   than one parent mass.
3. **Where along the linear placement of parent elements** it takes effect.
4. **The relative masses** at those placements.

## The Unicode hex example

Used as an illustration of the intended data structure only. It is not the
data, and nothing about Unicode carries over except the shape.

**Level 0.** The 16 hex codes are the most basic particle, each at **mass 1,
regardless of hex value**. Mass counts structure, not content. Computationally
a single digit is just a single digit, and two of them, unordered, are just
two: magnitude with no identity.

**Level 1.** Ordering is what creates identity. The 16 combine into 256
possible couplets because the combination is ordered. A couplet built from two
different hex codes is:

- total mass **2**, cumulative from its parents
- two mass-1 elements at **positions 1 and 2**
- identity carried by which code sits at which position, not by the mass

Mass alone is degenerate across all 256 couplets. They all weigh 2. Ordering is
the whole of the distinction.

**Interaction across levels.** Half of the couplet's mass, based on position,
responds to the individual hex code elements of other particles. A mass-2
couplet meeting a standalone mass-1 hex code engages with the mass share of the
position that matches, not with its whole mass. The field is therefore not
level-flat: particles at different levels of detail interact, and the
interacting fraction is set by positional match.

**Membership.** Each of the 256 couplets is a direct member of every table that
uses only those couplets, and each of those tables is one of its sibling
groups. Each distinct thing it represents under one of those tables is another.
So a single couplet carries a membership list whose length depends entirely on
how widely it is used, and no two couplets need carry the same number.

## One couplet's memberships, worked

The first complete instance, for a couplet of two different hex codes, say
`AB`. Mass 2, two positions.

| Group | Mode | Joins as | Mass engaged |
|---|---|---|---|
| The table of 16 hex codes | Parent | its component at position 1 | half |
| The table of 16 hex codes | Parent | its component at position 2 | half |
| The specific Unicode table it belongs to | Sibling | the whole construct | total |
| `Unicode_tables`, the category | Sibling | the whole construct | total |
| Exact `token_id` matches | Sibling | the whole construct | total |

Three things this settles.

**The same table serves both modes.** The hex code table is a *parent*
relationship to a couplet, which joins it through components, and a *sibling*
relationship to a bare hex code, which joins it whole. One group, two modes,
decided entirely by the member. Nothing types the group, which is the rule from
the previous section demonstrated rather than stated.

**Memberships exist at more than the immediate level.** The construct
participates in the specific Unicode table *and* in the category above it. So a
particle holds edges up the containment chain, not just to its nearest
bounding group. *Residual:* whether that means every level to the root, or a
declared subset.

**It matches the cross-level statement made earlier.** Half the couplet's mass,
by position, responding to individual hex codes of other particles is exactly
its parent-mode participation in the hex table, entered once per position at
half mass each.

**Derived, now better supported.** "Distinct participation" reads as the two
positions participating separately because they hold different codes. A doubled
couplet `AA` would then gather both positions into one participation at full
mass rather than two at half, which is what the sibling sub-grouping rule
predicted earlier.

## Geometry and the grid

**The containment hierarchy has a root.** In the example, every specific table
gathers under one Unicode Table group, and each specific table sub-organizes
beneath it. So bounding chains terminate at a top group rather than running
open-ended, and a membership sits somewhere on a path from the root down.

**Arrangement is not assigned, it lines up.** The tables whose endpoints are
just the 256 couplets line up according to internal definition and the geometry
among the pieces. Nothing places them from outside. What results is a grid in
free space: free meaning there is no pre-existing coordinate frame the grid is
fitted into, the pieces themselves are what constitute it.

**First, which sense of "dimension" this is.** A dimension is any agreed plane
of comparison between two or more points; x, y and z are dimensions because
humans have agreed they are common ways to measure and interpret the space
around us, and they hold no privilege beyond that agreement. The simulation is
a three dimensional visualization of field effects across **n dimensions of
commonality**. So there are two senses in play and they are not the same n: the
three the spheres are drawn and moved in, and the n the field acts across. The
axes below are the second sort. Full statement in
`field-physics-and-tick-notes.md`, "What a dimension is".

**Derived, needs confirming, and load-bearing.** The natural reading of the
example is that the ordered positions of a parent structure are axes of
commonality, and the element sitting at each position is the coordinate along
that axis. A couplet has two positions each drawn from sixteen codes, which is
a sixteen by sixteen grid holding exactly the 256 combinations. A triplet would
be three such axes and sixteen cubed. On that reading:

- the number of positions in the parent structure is a count of commonality
  dimensions, not of spatial axes, and puts no axes into the drawn space
- the coordinate on each axis is the identity of the element at that position
- the grid is complete precisely because the combinations are exhaustive
- no coordinate needs storing, because the token already carries it

If that reading is right it collapses a large amount of the data model, since
geometry becomes a decomposition of the token rather than a stored field. If it
is wrong, positions have to be held per member and the harness carries them.

**Distance becomes available, in commonality.** A grid supplies separation
between pieces, which is what the field basis needs: attraction falling off
with distance between imbalances. Until this point nothing in the particle
structure supplied a distance at all.

**Open, and it was hidden by the word.** Whether that separation is the `d` in
`F = m1 * m2 / d^2`, or whether commonality only selects which groups a
particle reads and every distance in the force law is a distance in the drawn
three dimensions. The earlier phrasing, "the positions are the axes", read as
though the commonality grid were the space the particles move in. It is not,
and the two cannot be assumed to share a metric.

## What this resolves

**"Relative to what" is answered.** A relative mass is the share of the
particle's total mass attributable to the positions taking part in that
relation. In the couplet, one of two positions is half. Since mass is
cumulative, the general form is the summed mass of the participating positions
over the particle's total mass.

**The earlier 1-3-5 example is explained.** A force affecting elements 1, 3 and
5 of a five-element parent "because they repeat" is the sibling rule operating
inside a parent structure: those positions hold the same `token_id`, so they
gather, and that gathering is bounded by the parent structure. Its relative
mass is three parts of five. **Derived:** repetition is the mechanism that
creates the gathering, and the parent structure is its bound.

**Derived, needs confirming.** A couplet of the same code twice, `AA`, would
put both positions in one gathering, giving it a relative mass of one whole
against that code rather than a half.

**Derived, needs confirming.** Placement selectors must support arbitrary index
sets, not only periodic ones. The periodicity of 1, 3, 5 is incidental to that
repeat pattern.

**A testable invariant.** The relative masses over the gatherings within one
parent structure partition it, so they sum to one. A cheap correctness check
for the harness to assert on every particle it loads. Note this holds for the
intra-parent case only; it does not hold across table and variation
memberships, which each take the whole mass.

## Resolved from the record

Closed by reading back what was already said, rather than asked again. Recorded
as readings where I inferred rather than quoted.

- **Mass across memberships.** Stated flatly: a sibling relationship always
  applies to the total mass of the particle. Every membership engages the whole
  mass. Nothing divides it among them.
- **Given or emergent geometry.** Emergent. Particles move under force and find
  their place, and the primary attractors are what define a particle's primary
  placement. Position is produced, not read in.
- **One space or many.** One. A couplet exerts and receives force against the
  hex table, its Unicode tables and the category above them, all at once and
  all by distance to a centroid. Separate spaces would leave no distance
  between them for the law to use.
- **Bounding chain depth.** Every level, in the storage construct, which is
  explicit at every level and carries its own parent, child and sibling
  relationships. The working tree keeps whatever the chosen compression keeps.
- **Rollup depth.** The whole chain, by construction rather than by rule. Each
  level's total is the exact rollup of the level below it, so an ancestor's
  contribution is already inside its child's total before the next rollup
  reads it.
- **Self-identity scope. Reading:** global. Exact `token_id` matches are always
  a field for every particle, stated without a bound, where every other
  gathering was given one.

## Open

- **Level span.** How many levels of detail the structure carries, which sets
  how far cross-level interaction has to reach. A property of the data rather
  than of the model.

## Consequences worth carrying forward

**Sibling groups are first-class objects, not particle-local data.** A group is
a gathering of many particles, so holding it inside each member would duplicate
it once per member and let the copies drift. The harness wants a group registry
plus a membership edge list, with the bounding relation as a group-to-group
edge. That makes three structures, not one: particles, groups, and memberships.
A data-harness consequence, not a decision taken here.

**Variable extent is now doubly confirmed.** Force lines vary per particle and
membership counts vary per particle, and they vary independently. Fixed-width
rows are out. The shape is a flat array per kind, with per-particle start and
count into each.

**Mass rollup is a tree traversal, field effect is not.** The rollup runs up
the level-of-detail hierarchy and is what the SNode tree is shaped for. The
force lines and sibling memberships act on data held at the token. They read
related records but they are different traversals, and they are different
kernels.

**There are two hierarchies, and they are not the same.** The level-of-detail
hierarchy carries mass upward from constituents. The group containment
hierarchy bounds gatherings inside larger gatherings. Conflating them would be
the same error as reading field structure off the SNode tree.

**Cross-level interaction is a harness requirement, not an optimization.**
Because a composite engages other particles through the mass share of matching
positions, no pass can be assumed to operate within one level.

**Slot width is still open.** A cumulative rollup stays within range far better
than a repeated product would, so the four-versus-eight byte decision rests on
the force calculations rather than on mass accumulation. Relative masses being
rational fractions of small integers is a point in favour of four.
