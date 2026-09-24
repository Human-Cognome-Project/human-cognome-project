> **Recovered design record (2026-09-24).** This note was authored during the native C++ engine/harness work before that local workspace was fully pushed. Statements about what was "not built", file locations, or open work reflect the date/context of the note. For current implementation status, read `engine/docs/README.md` and `engine/docs/OPERATIONAL-PLAN.md`; current repository policy is in `AGENTS.md` and `CONTRIBUTING.md`.

# Bonding notes

Working notes, not a specification. Patrick's statements recorded as given;
lines marked **derived** are mine and need confirming. Nothing is built from
this yet.

2026-09-13 — companion to `parent-structure-notes.md` and
`field-physics-and-tick-notes.md`.

Three kinds of bonding are required. All three are given below. Polarity is
named but not yet explained.

## 1. Direct bond

**Defined from the database.** It is stored, not computed and not emergent. It
comes out of the storage construct, which is explicit at every level, and is
carried into whatever the working construct projects.

**What it is for.** It keeps the correct linear structure inside a compressed
large-level-of-detail sphere. When a big structure is standing as one particle,
the direct bonds are what hold its internal order, so that expanding it again
yields the right shape rather than a bag of parts.

**Where the shape comes from.** Parents are listed in order, and that ordered
list *is* the connected shape the level of detail expands to. The ordering that
gives a composite its identity is the same ordering that gives it its
connectivity.

**Mechanically.** Bonded particles **ignore the corona** and **slide freely in
direct contact**. So a direct bond holds separation at contact while leaving
tangential motion unconstrained. It is a contact relation, not a rigid one:
bonded particles can move around each other, but not apart and not through.

## 2. Discovered valid bond

**Discovered, not stored.** Found in operation rather than read from the
database, and it requires **both the field and the polarity to match**.

**Separation.** The coronas overlap, and the pair maintains a half-particle
distance between them.

## 3. Cross-polarity field match

The field is drawing the particles together, but polarity prevents them from
being a full match. So the attraction is real and unchanged; polarity sets a
limit on how close it can bring them.

**Separation.** The coronas abut and never merge.

## Polarity is relative ordering, defined by pairwise connection

Not quite polarity. A matching field position in an opposite relative ordering
position cannot achieve a firm connection; only identical relative ordering
does.

**What defines the alignment is the pairwise connection.** Where two things are
linked by a pairwise connection, that connection is what says they align. It is
a lookup against a relation that exists in the data, not an arithmetic
comparison of position numbers.

So the two tests are:

1. **Field match.** The elements are the same element. This is similarity, and
   it is what produces a response at all.
2. **Alignment.** A defined pairwise relationship exists between those two
   positions, in that configuration.

Similarity with a pairwise relationship gives the firm join. Similarity without
one still calls on the similarity, but not as a direct group, and that is the
weaker, abutting join.

### The worked case, corrected

`FF` drawn toward `0F`. Both hold an `F`, so similarity is present twice. The
positions align where those two couplet positions have a defined pairwise
relationship in that configuration, and not otherwise. The first `F` cannot
reach the level of join the second one does because it does not hold the same
relative ordering position.

**Withdrawn.** An earlier version of this section read the test as comparing
position indices, and then asked how that comparison would work between
structures of different lengths. Both were mine. Nothing compares indices, so
there is nothing to normalize, and the question was a new field invented to
service an example rather than anything the model needs.

### Where the alignment comes from

The pairwise connection is not a new relation. Two couplets **call to each
other as couplets**, whole to whole, through the fields they share. Whether
they also draw a direct connection as related couplet meanings across the two
constructs depends on whether there is **sufficient parent predicate structure
to link those positions**.

So alignment is carried by the parent-predicated force lines already described:
the lines listing which parent masses a force applies to and where along the
linear placement of parent elements it acts. Where that structure links two
positions, they align. Where it does not, they still call on similarity, but
not as a direct group.

### How the levels are built

In the Unicode illustration, an expanded byte-code cluster shows its parents as
a new byte id plus the 256-hex block, stepping up in length and referencing the
next smaller construct. Each level is therefore built from one new element and
the level below it, recursively, and expanding a level all the way down yields
the linear byte sequence in order.

That is what the direct bond holds together inside a compressed sphere: the
chain is already there in the parent numbering, at every level.

### Wrong bonds are displaced, not prevented

An incorrect but possible bond will be displaced by the appropriate bond when
one becomes available, because the appropriate bond carries less constraint on
the distance and can therefore hold closer. At this scale the difference
between the two separations is a large jump in relative hold, so the correct
partner simply out-holds the incorrect one.

That means **no validation logic is needed anywhere**. Nothing has to detect a
wrong bond, score it, or break it. The distance ladder does the selecting: the
closer join binds harder and takes the position. Wrong bonds are transient
states of a settling system rather than errors to be caught.

### What follows

**Nothing new is stored for this.** No polarity field, no charge, no sign. The
tests read what is already there: the element at a position, and whether
sufficient parent predicate structure links two positions.

**Discovery is paid once.** The pairwise tax of discovery is real, it is paid
deliberately, and it is **ledgered permanently**. A relationship found once
becomes explicit in the database and is a lookup ever after. The database is
explicit in every relationship precisely because discovery keeps writing into
it.

**Bonds are per position pair, not per particle pair.** One particle pair can
hold a firm join through one position and a weak one through another, which is
the same shape the direct bond has, since that is read from parent numbering.

**It engages the mass at that position**, which is the partial-response case,
using the relative masses the force lines already carry.

## The framework

Two tests, and they classify everything:

| Field | Polarity | Result |
|---|---|---|
| no match | — | nothing. A zero response, as already stated |
| match | match | valid bond, coronas overlapping |
| match | mismatch | cross bond, coronas abutting |

The direct bond sits outside that table because it is not discovered at all. It
is read from parent numbering, which is structure rather than a finding.

**The three bonds are one thing with three values.** Each names a separation the
pair settles to and holds:

| Bond | Corona | Gap between bodies | Centre separation |
|---|---|---|---|
| Direct | ignored | none, direct contact | 1 |
| Valid | overlapping | half a unit | 1.5 |
| Cross | abutting, never merging | one unit | 2 |

So bonding is a held separation, and what differs is only the distance held and
what establishes it. That is expressible as one mechanism carrying one number
per bond, rather than three mechanisms. Worth holding onto when the enforcement
is specified.

**Cross polarity is a standoff, not a reversal.** The field still pulls the pair
together; polarity stops the approach at a distance. The pair stays bound, held
between an attraction inward and whatever polarity does outward. That is how
structure gets spacing rather than collapse.

**Established.** A particle is a one unit sphere carrying a 0.5 unit corona, so
the three centre separations are one, one and a half, and two. A clean ladder,
and a fixed one: these are the numbers rather than a ratio family waiting on a
unit. Contact is at one because the bodies are a unit across; the valid bond
adds half a unit of gap, with the coronas still overlapping since each reaches
half a unit past its surface; the cross bond adds a full unit, which is exactly
where the two coronas meet without merging. The ladder is the corona geometry
read off directly. See `particle-geometry-notes.md`.

**Likely the sign rule.** `physics-basis.md` derives inverse-square attraction
from the field and notes it gives "Newton's law, and Coulomb's with a sign
rule". Polarity looks like that sign rule arriving. Holding that as a
connection to check rather than an assumption, until polarity is explained.

## What this closes

**"Unconstrained" now has a meaning.** The calibration case is two
*unconstrained* particles of mass 1 at separation 1. Unconstrained means
unbonded. Constraint is bonding, so a bonded pair does not fall together the
way the calibration pair does, and the constant was fixed on the free case.

That was an open item in `field-physics-and-tick-notes.md` and is now answered.

## Readings, to confirm

**Confirmed.** The direct bond needs no separate storage. It is read from
parent numbering, so the connectivity is already present wherever the ordering
is.

**Derived.** "Ignore the corona" reads as a per-pair property rather than a
global one: the corona still governs every unbonded interaction, and a bond
changes it only between the two particles it joins.

**Closed.** Sliding applies to all particles, not only to the direct bond.
Contact is frictionless everywhere.

## Spin is scoped, and narrowly

Particles can impact kinetically, but **spin serves no purpose beyond
predicated, directed spin**. Rotation exists only where a force predicated on
parent structure acts at a listed position and directs it. It does not arise
from contact.

Because sliding is free, an impact exchanges momentum without imparting any
turn. That rules out a whole family of things by name, and they should not be
reached for later: no friction coefficient, no restitution, no tangential
contact force, no angular momentum transfer on collision, no contact solver.

This is what the harness already does and should keep doing. Torque is
accumulated only from a share sitting off the particle's centre, which is the
predicated case exactly. Nothing else writes to it.

**Closed, and it is small.** Momentum exchange is all an impact does. Because
the field gathering is what places things, an impact is at most a chance for a
structure to realign better. It is a perturbation on a settling process, not a
force law of its own and not a second mechanism competing with the field.

So there is nothing here to build a collision system around. The field
gathering does the placing and the bond separations do the holding. An impact
only lets a structure that has settled poorly get another go.

## Not designed here, deliberately

How a direct bond is *enforced* during a tick has not been given, and nothing
is invented for it. Holding separation at contact while leaving tangential
motion free is a description of behaviour, not a mechanism, and the obvious
mechanisms are all imported machinery that would quietly make this something
other than what it is.

All three are now on the table, and they unify to a held separation, so one
enforcement should serve all of them. What that enforcement is has still not
been given, and polarity is still to be explained. Waiting for both rather than
inventing either.
