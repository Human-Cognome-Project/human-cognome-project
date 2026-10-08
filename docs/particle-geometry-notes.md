> **Recovered design record (2026-09-24).** This note was authored during the native C++ engine/harness work before that local workspace was fully pushed. Statements about what was "not built", file locations, or open work reflect the date/context of the note. For current implementation status, read `engine/docs/README.md` and `engine/docs/OPERATIONAL-PLAN.md`; current repository policy is in `AGENTS.md` and `CONTRIBUTING.md`.

# Particle geometry: clean context for the next phase

Prepared as a starting point, not as a specification. Everything under
"established" is Patrick's, recorded as stated. Everything marked **derived**
is mine and needs confirming. Nothing under "open" is filled in.

2026-09-13. Companions: `parent-structure-notes.md`, `bonding-notes.md`,
`field-physics-and-tick-notes.md`, `storage-and-working-split.md`.

## Corona-disabled test direction (Patrick, 2026-10-08)

Test with the **corona effect disabled**. Particles may come to rest in ordinary
body contact; competing field forces should separate them where appropriate.
Remove imposed corona spacing, overlap/abut regimes and corona-derived gaps
from the experimental model. Retain ordinary contact.

The proposed explanation for earlier difficulties is the limited size of the
early modeled structures. Test that hypothesis in larger, more connected
environments; it is not an established result. Assess settled contact,
field-driven separation, cohesion, numerical stability and settling with the
LoD-scaled, whole-particle response model.

This is a test direction, not a claim that the native implementation has already
changed or that permanent removal is validated. Historical corona implementation
and results below describe the baseline only. The old corona separation ladder
is not an acceptance requirement for the disabled experiment. Runtime work and
comparative evidence are tracked in #119.

## Whole-particle response and focus-relative distance (Patrick, 2026-10-08)

**Settled model direction; implementation reconciliation remains pending.**
Parents retain proportional participation: the participating parent mass/share
sets its contribution, and that contribution always acts on the whole particle.
Preserve ordered composition and repeated parent contributions. Remove positional
response, internal application offsets, torque, rotation and rotary alignment
from the requirements; there is no rotational reference to calculate against.

When finer LoD is exposed, its components operate normally as particles.
When it is compressed, the object is a **marble-shaped Markov blanket**:
the boundary inputs and outputs are the relevant points of interaction.
Do not calculate hidden internal geometry or orientation to apply a parent
response. This does not remove proportional parent participation.

**Distance is relative to the current focus of study.** An object distant in
LoD from one perspective can be directly adjacent from another. Warm-cache
SNode definitions and their composition must support this study-relative
expression; a cold object identity does not have one universal fixed distance
or permanently assigned operative LoD. Preserve the finest-visible unit baseline,
equivalence compression and consistent scale conversion within each view.

The possible relationship-dependent distance multiples/degrees of separation
must be interpreted within that focus. Test 64 initially, consider higher/lower
multiples where needed, and monitor whether these belong in warm SNode
definitions. Their exact representation remains open (#106, #115).
Changing study focus must retain identity and proportional participation while
recomposing the exposed relationships and their operative distances.

## Current distance clarification (2026-10-07)

The finest visible LoD is the one-particle/one-distance-unit baseline. Each
less granular expressed LoD expands by the proposed factor 64 or 128; the
factor choice is open. Compressed equivalences need no full distance step.
Zooming into a finer operative level rebases the measurement frame. See the
[system guide](napier-system-guide.md#operative-distance-follows-expressed-lod-patrick-2026-10-07).
This is model intent, not a verified feature of the native engine.

## Established

**Historical corona baseline (disabled in the new test): every particle is a one unit sphere carrying a 0.5 unit corona, and that was
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

**Parent response is proportional and acts on the whole (2026-10-08).**
The participating share raises its contribution; the whole particle absorbs it.
There is no internal application offset, torque or spin.

**Contact is frictionless and impacts are cheap.** An impact exchanges
momentum, nothing else, and its only significance is giving a structure a
chance to realign better.

## Historical corona separation ladder — not a requirement for the new test

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

**Rotation removed (2026-10-08).** Neither angular dynamics nor directed rotary
alignment is required. Exposed components operate independently; compressed
objects expose only their boundary inputs/outputs.

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

**The metric is anchored to the finest visible LoD.** At its own primary LoD
an object is one particle; in the current finest-visible measurement frame,
each less granular expressed level is a factored expansion (64 or 128 per
operative step). A common measurement frame therefore requires explicit scale
conversion. Stored nesting and equivalence links do not necessarily add a step.
There is no requirement to fit a contacting literal chain inside a unit sphere.

**The corona is relative in the same way.** Half a unit of whatever the
particle is at that level. It follows from the same statement, not from a
separate rule.

**Ordered composition is preserved; spatial string geometry is withdrawn.**
The parent list retains identity, order and repeated occurrences. It does not
by itself prescribe a chain at contact separation or an offset along a line.
Use molecular-level operative distances at the expressed LoD. Do not derive
component offsets or a moment of inertia from the withdrawn string arrangement.

**Geometry does not vary with contents either.** One unit sphere, 0.5 unit
corona, the only relevant size at any level of detail. Since that holds at
every level regardless of what is held, a particle's own geometry cannot vary
with what it holds. An earlier version listed this as open; it was answered by
the same statement that fixed the size.

**Orientation is not operative state.** The earlier directed-spin requirement
is superseded by proportional response on the whole particle.

## Genuinely not yet stated

Short, and about the next phase rather than about the last one.

- Implement the expressed-LoD distance conversion and molecular-level placement
  without deriving a spatial string from parent ordinals. Select 64 or 128;
  verify rebasing and equivalence compression without changing composition.
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
