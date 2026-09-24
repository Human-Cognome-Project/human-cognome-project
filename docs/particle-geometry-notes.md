> **Recovered design record (2026-09-24).** This note was authored during the native C++ engine/harness work before that local workspace was fully pushed. Statements about what was "not built", file locations, or open work reflect the date/context of the note. For current implementation status, read `engine/docs/README.md` and `engine/docs/OPERATIONAL-PLAN.md`; current repository policy is in `AGENTS.md` and `CONTRIBUTING.md`.

# Particle geometry: clean context for the next phase

Prepared as a starting point, not as a specification. Everything under
"established" is Patrick's, recorded as stated. Everything marked **derived**
is mine and needs confirming. Nothing under "open" is filled in.

2026-09-13. Companions: `parent-structure-notes.md`, `bonding-notes.md`,
`field-physics-and-tick-notes.md`, `storage-and-working-split.md`.

## Established

**Every particle is a one unit sphere carrying a 0.5 unit corona, and that is
the only relevant size at any level of detail.** The sphere is one unit across;
the corona reaches half a unit beyond its surface, so a particle's outer extent
is two units. The corona is incorporated into the coupling rules. Nothing else
in the model carries a size.

**The simulation is a three dimensional visualization of field effects across n
dimensions of commonality.** Three dimensions are what is drawn and what the
spheres move in. The commonality the field acts across is n dimensional and is
not those three. See `field-physics-and-tick-notes.md`, "What a dimension is".

**At its own level of detail, everything is one particle**, regardless of how
large or complex its contents are. A composite of a billion elements is one
particle with one mass at one location, exactly as a base element is. The force
pass never asks what a particle contains.

**Mass is a count, and it is exact.** Base particles are mass 1 regardless of
what they hold; mass counts structure, not content. Total mass is the
cumulative, *exact* rollup of parent effects. So masses are integers, and the
arithmetic on them does not drift.

**A share of a particle sits somewhere.** Partial response acts at a position
offset from the particle's centre: the share raises the force, the whole mass
absorbs it, and whatever does not act through the centre becomes spin.

**Spin is narrow.** Only predicated, directed spin exists. Contact produces
none, because sliding is free for all particles.

**Contact is frictionless and impacts are cheap.** An impact exchanges
momentum, nothing else, and its only significance is giving a structure a
chance to realign better.

## The separation ladder

The operative geometry so far is what the three bonds hold:

| Bond | Corona | Gap between bodies | Centre separation |
|---|---|---|---|
| Direct | ignored | none, direct contact | 1 |
| Valid | overlapping | half a unit | 1.5 |
| Cross | abutting, never merging | one unit | 2 |

**Established.** The sphere is one unit across and the corona reaches 0.5
beyond its surface, so the centre separations are 1, 1.5 and 2. Those are the
numbers, not a ratio family waiting on a unit. A bond holding "half a particle"
holds half a unit of gap between the two surfaces.

**Established, and load-bearing.** Because bodies have extent, two distinct
particles cannot be closer than one unit between centres. Separation below body
scale does not occur between bodies at all.

**The 0.25 in the harness is not geometry.** `engine/src/field/field.h:65`
carries `float softening{0.25f}`, added to every squared separation in the force
kernel, with a comment calling it body-sized. Nothing in the geometry produces
that number. The corona is 0.5 around the entire orb; 0.25 is 0.5 squared, and
a squared half only appears because the term is added to d^2 rather than to d.
That is Plummer softening out of a standard N-body code, imported whole and
explained backwards afterwards. It is machinery, not a consequence of anything
stated here, and earlier versions of this note — including one written today —
dressed it up as a corona or body radius squared, which is how an imported
constant acquires a false pedigree.

**And it has no case left to cover.** Bodies cannot be closer than one unit
between centres, so between particles the term never bites. The sole-member
centroid, the one place separation is exactly zero, is already handled exactly
by the sign of the absolute mass difference. An unconditional constant added to
every calculation to cover a case already covered is the epsilon fudge the
governing constraint rules out. It should come out of the harness, not be
justified.

**Established.** Rotation is ordinary: the drawn space is three dimensional, so
rotation has three planes and does not gain one per position in a parent
structure. The positions of a parent structure count dimensions of commonality,
which are not axes of the drawn space. An earlier worry in the physics notes
about rotation dimensionality was the two senses of "dimension" run together.

## Answered already, and previously mislisted as open

An earlier version of this file raised six questions here, all six already
settled in what had been said, and it then answered one of them by declaring it
malformed, which loses the predicate just as thoroughly. Recorded properly,
because losing an established predicate — by re-asking it, or by ruling it out
of order — is worse than not writing it down at all.

**The size is stated, and it is one unit across.** A particle is a one unit
sphere with a 0.5 unit corona. An earlier version of this file argued the
question away — that asking whether the unit was a diameter or a radius
imported an outside metre that did not exist — and that was wrong twice over.
It was a real question, it has a plain answer inside particle units, and
declaring it malformed is the same way of losing a predicate the section
heading above complains about. Everything is still expressed in particle units;
the unit simply spans the sphere rather than half of it.

**Size does not vary.** At its relative level of detail, everything is one
particle regardless of how large or complex its contents are. One particle
means one unit across. A composite is not a bigger sphere; it is a sphere at
its own level.

**So the unit renormalizes with the level, and so does the metric.** One unit
is one unit at whichever level is being looked at. A particle's contents are
measured in the units of the level below, where each of them is itself one unit
across. There is no single ruler spanning the levels, which is what lets a
chain of contacting elements sit inside a sphere that is itself one unit: the
chain's separations are not in the parent's units.

**The corona is relative in the same way.** Half a unit of whatever the
particle is at that level. It follows from the same statement, not from a
separate rule.

**The internal arrangement is the ordered parent list.** Parents are listed in
order, and that ordered list is the connected shape the level of detail expands
to. The direct bond holds that chain at contact separation, corona ignored, and
it is read from parent numbering rather than stored. A share's offset therefore
comes from where its position sits along that chain.

**Moment of inertia follows.** The arrangement is known, the element masses are
known and exact, so the second moment is computable once per token and shared
by every instance, exactly as total mass is. It was never waiting on anything
except the arrangement above.

**Geometry does not vary with contents either.** One unit sphere, 0.5 unit
corona, the only relevant size at any level of detail. Since that holds at
every level regardless of what is held, a particle's own geometry cannot vary
with what it holds. An earlier version listed this as open; it was answered by
the same statement that fixed the size.

**Spin does not feed back.** Spin serves no purpose beyond predicated, directed
spin. A spin that changed which positions face which neighbours would be a
purpose beyond the predicated one, so there is none. Orientation is carried
only as far as directed spin requires.

## Genuinely not yet stated

Short, and about the next phase rather than about the last one.

- How the ordered chain occupies the sphere: whether the linear structure lies
  along one axis, folds, or takes its shape from the grid the pieces line up
  into. Well posed only because the metric renormalizes per level; without
  that, a chain of unit elements inside a unit sphere has no answer at all.
- Nothing about the softening constant. It is not an open question; it is an
  import to delete from `engine/src/field/field.h`.
- Whether any separation in commonality enters the force law, or whether the
  only distances in it are the three dimensional ones the spheres sit in. Held
  in `parent-structure-notes.md` and `field-physics-and-tick-notes.md`; listed
  here because the answer decides what the geometry above is a geometry of.

## Constraints any answer has to satisfy

Carried forward so the next phase is not re-derived:

- **No exceptions.** Whatever the geometry is, it applies to every particle
  identically. A rule that fires for some and not others is a branch and is out
  of bounds; express it as arithmetic instead, the way the sole-member rule
  became a sign of an absolute difference.
- **Combinatoric control.** The geometry should remove touches, not add them.
  If an answer requires comparing a particle against its neighbours to decide
  anything, that is a cost class the force law has so far avoided entirely.
- **Level-blindness.** The force pass must be able to stay ignorant of what a
  particle contains. Any geometry that makes a composite behave differently
  from a base element breaks the one-kernel-every-level property.
- **Exactness where the quantity is a count.** Masses are integers and must
  stay integers. See the defect below.

## The stored share is the defect, not its precision

The harness stores a participation share as a float. Carrying a fraction at all
is the mistake, and rounding is only the symptom: three fifths is not exact in
binary, so a stored share is wrong before anything is computed with it.

**The share does not need storing or fixing up, it needs not existing.** The
total mass and the relative masses are already present in the data points, so
the direct calculation is available at the point of use and is the fast path:
take the participating mass and the total mass as the counts they are, and
divide there. Nothing is carried between the two, nothing is pre-divided, and
no invariant about shares summing to one is needed because no share is held for
one to be stated about.

That also keeps the counts integers the whole way, which is what the exactness
constraint above asks for. It is small, it is in `engine/src/field/`, and it has
not been done.
