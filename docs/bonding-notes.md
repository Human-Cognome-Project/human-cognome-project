> **Recovered design record (2026-09-24).** This note was authored during the native C++ engine/harness work before that local workspace was fully pushed. Statements about what was "not built", file locations, or open work reflect the date/context of the note. For current implementation status, read `engine/docs/README.md` and `engine/docs/OPERATIONAL-PLAN.md`; current repository policy is in `AGENTS.md` and `CONTRIBUTING.md`.

# Bonding notes

Working notes, not a specification. Patrick's statements recorded as given;
lines marked **derived** are mine and need confirming. Nothing is built from
this yet.

2026-09-13 — companion to `parent-structure-notes.md` and
`field-physics-and-tick-notes.md`.

Three kinds of bonding are required. All three are given below. Polarity is
named but not yet explained.

### Local cohesion, dispersion and calibration (Patrick, 2026-10-08)

Distance scaling supplies the intended shear resistance at every expressed
level. Component distances are multiplicatively smaller than macro distances,
so their inverse-square attractive forces are correspondingly stronger.
Ordered composition survives without beads on a string, flexible-joint
machinery, or an additional shear-specific protective gate.

**Limited scope is an effect of distance and dispersion, not a scope mask.**
The fact that the entire model is composed of hex codes has no meaningful
general effect on an individual hex code when those occurrences are dispersed;
nearby configuration dominates at that scale. This repeats at each level up.
Do not implement an artificial construct boundary or disable universal fields
to achieve this. This is the intended model behavior to verify under load,
not a claim that dispersion mathematically guarantees cancellation.

A **binary (power-of-two) factor** is preferred for clean, fast arithmetic.
Test **64** first; **128** is the next candidate if needed. The requirement is
sufficient local force scaling while permitting universal effects. As the
model grows, linear distance grows too; monitor both local cohesion and
aggregate distant effects rather than calibrating on one small assembly.

**Monitoring / discussion, not a schema decision:** degrees of separation
means that different kinds of relationships may need higher or lower distance
multiples to produce the needed effects. One universal factor may not suffice.
Those relationship-dependent separations may need to be part of warm-cache
definitions of SNode objects. This is not graph-hop count, parent/address depth
or raw SNode nesting. Test 64 as the initial binary factor, then assess the
multiples needed by each relationship kind; no per-kind values, field layout
or conversion formula are fixed yet. Track in #115 and the bridge issue #106.

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

## 1. Direct bond

**Defined from the database.** It is stored, not computed and not emergent. It
comes out of the storage construct, which is explicit at every level, and is
carried into whatever the working construct projects.

**What it is for.** It preserves the defined ordered composition in a
compressed representation, so expansion recovers the correct occurrences and
relationships. It does not require a linear spatial string inside the sphere.

**Operative geometry (Patrick, 2026-10-07).** Parent ordering carries composition
and connectivity; molecular-level distances follow expressed LoD. The finest
visible level defines one particle as one distance unit; less granular expressed
levels expand by 64 or 128 (choice open), while compressed equivalences can skip
steps. See the [system guide](napier-system-guide.md#operative-distance-follows-expressed-lod-patrick-2026-10-07).
The contact mechanics below do not impose string geometry on a literal.

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
operative placement of the ordered parent components it acts. Where that structure links two
positions, they align. Where it does not, they still call on similarity, but
not as a direct group.

### How the levels are built

In the Unicode illustration, an expanded byte-code cluster shows its parents as
a new byte id plus the 256-hex block, stepping up in length and referencing the
next smaller construct. Each level is therefore built from one new element and
the level below it, recursively, and expanding a level all the way down yields
the linear byte sequence in order.

That is what the direct bond preserves in a compressed representation:
composition order is recorded in parent numbering at every stored level.
Recovering the ordered byte sequence does not require a spatial chain.

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

## Whole-particle response replaces rotation (2026-10-08)

Parent participation remains proportional and always affects the whole.
No torque, positional response, rotation or rotary alignment is required.
Exposed finer components operate normally; compressed objects are marble-shaped
Markov blankets whose boundary inputs and outputs define the active interaction.
The earlier claim that the harness should accumulate off-centre torque is
superseded. Runtime reconciliation is separate from this documentation change.

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
