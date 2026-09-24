> **Recovered September 2026 record.** This document preserves the native-engine development/vetting context in which it was written. Absolute `/opt/project/...` paths, agent-routing instructions, and statements about branch/commit status are historical. Current paths/status are indexed in [README.md](README.md); current repository policy is in the root `AGENTS.md` and `CONTRIBUTING.md`.

# Control-harness design notes

Working notes for the control harness that sits over the engine in this
folder. These are kept because the design cannot be reduced to a median
summary — the distinctions below are literal and load-bearing — and a context
clear is expected. Read this whole file back after any reload before acting.

Status: **design in progress.** The structure and constraints are settled
(below). The core functions and formulas, and how they reach the GPU, are
still to be given by Patrick — that is the next input. Do not invent them.

## Reload / fresh-context primer (read first on any reset)

If this is a fresh context: everything needed is in the docs — the exploratory
dialogue that built this is fully captured. Read in order:

1. **This file (`HARNESS-NOTES.md`)** — the literal model record + design laws +
   agent policy + scope discipline.
2. **`HARNESS-STRUCTURE.md`** — the open harness skeleton (current phase).
3. **`VETTING-PLAN.md`** — vetted engine facts (seam CONFIRMED, premise
   reconciled, docs audited accurate).
4. The corrected engine docs (`README`, `CONFIGURATION`, `DEVICES`,
   `ENGINE-NOTES`, `api-reference/NOTES`) as reference.

**Operating constraints (do not drop on reload):** examine only
`/opt/project/repo/engine` + subfolders; **documents only** — write no code,
code changes dispatch to agents; **Opus 4.8 for investigation, Sonnet for
coding, Opus 5 / Fable 5 banned**; CPU **follows, never searches**; no
special-cases (gates in the equation).

**Current phase (2026-09-14):** a real field engine already exists in
`src/field/` (`field::Harness`, SoA layout, one-law force + centroid ping-pong +
brake, CPU+CUDA, tests). It was built by the degraded model — "mostly right,
unreliable in the important bits." Drift audit + corrections are in
`docs/DRIFT-AUDIT.md`: **findings 1, 2, and 3 are all DONE and verified**
(Sonnet units A, B, C). Finding 1 (softened law → exact `sgn(d²)` gate); finding
2 (invented rigid-body rotation → stripped, offset kept); finding 3 (the brake:
destination = the sum of the m1·m2/d² vectors = the resultant, reach = its
magnitude, the drifted second accumulator removed). **The physics core is now
complete and correct.** Remaining harness work is the CPU-resident half
(follow-only DB traversal, tree composition, focus) — gated on the DB source and
Patrick's pacing. **Everything is now consolidated into one plan:
`docs/OPERATIONAL-PLAN.md`** — the model (including the contact-and-bonding
material Patrick specified 2026-09-14), the vetted engine reality, the current
build state, and the forward sequence, in one place so it is not re-derived. This
file remains the literal dialogue-level record; the plan is the consolidation.
No agents currently running.

## TRUST LEVEL — read this first

**None of the current workspace can be fully trusted until vetted** (Patrick,
2026-09-13). Everything below sourced from the workspace docs (README,
CONFIGURATION, DEVICES, ENGINE-NOTES, api-reference NOTES) is a **claim, not a
verified fact** — the docs are point-in-time and unvetted. This includes the
four engine primitives, the hard constraints, the 1070 envelope numbers, and
the force/field hypotheses. Do **not** assert any of it as true.

**The workspace was assembled by Opus 5** (Patrick, 2026-09-13) — a model
banned here for its fatal flaws on unique requirements. So distrust extends
past the docs to the **assembly itself**: which build and archives are linked
(`build-review/…`), the config choices, and the layout may **not** reflect the
optimal setup from the updated source, even assuming the source work was done
correctly. Two things follow: (1) the sufficiency `vet-envelope` confirmed is a
property of the *engine source/design*, NOT proof this workspace realizes it
optimally; (2) this elevates **Q5 (build-vs-source staleness)** and adds an
**assembly-correctness** question — is the linked build/config the best
available from the updated source, or should a more correct version be built?
That is exactly the remediation path Patrick flagged (see box below).

**Update (Q5, 2026-09-13):** the *engine binary* side of this is cleared — the
linked build is current and faithfully reflects the modernized source (see
`VETTING-PLAN.md`). The residual Opus-5-assembly concern narrows to the
**workspace scaffolding / config** — CMake, the `src/engine/*` wrapper, and
doc accuracy — which is MVP-scoped, not the engine, and is **not yet vetted**.

Verification is a task for **dispatched agents**, checking each claim against
the live engine source and behaviour. Until an item is vetted that way, mark
it `[unvetted]`; once an agent confirms it against live code, mark it
`[vetted <date>]` with what was checked. **Force set (corrected 2026-09-14):** Patrick **knows many of the forces** — they
are **set up when the database connections are established**, not something for me to
investigate. Distinct from that, **the system's analytical purpose is to discover
MORE than is currently explicable** — the uncalibrated dimensions and emergent
co-structures the running system surfaces and promotes. Known forces = Patrick's
input with the DB; discovery of the inexplicable = the system's job, not mine. No
JSON / config surface; activation is a scalar launch argument.

**Vetted so far (2026-09-13, fork `vet-seam-2`):** the rate-of-change seam
**P1.1–P1.4 CONFIRMED** — capacity freeze at instantiation, values-as-launch-
args vs layout-forces-recompile, name-keyed compilation, and live
`add_snode_tree` with *recycled* tree ids — but per-node SNode ids are
**monotonic across destroyed layouts** (see design implication). **P0.1 64-bit
premise RECONCILED** (label = shorthand for lifting the 1024 ceiling; see box).
**Sufficiency CONFIRMED (fork `vet-envelope`):** the 1024 ceiling is genuinely
removed and replaced by configurable startup capacities honoured at compile +
load and verified end-to-end; the paged ListManager directory is real code;
dense i32/f32 on a ~12 GB card is bounded by card memory, not fixed metadata →
**suffices with configuration** for the MVP intent on the 3060 target class.
Non-blocking caveats: monotonic SNode ids (P2.3, now confirmed), sparse
metadata not reclaimed on tree destroy, 32-bit logical indices, LLVM-CPU + CUDA
only. **Q5 RESOLVED:** the linked build is NOT stale — it post-dates all engine
source and contains the modernized capacity/paging/ABI work; no newer un-built
source exists, so the rebuild-for-more-correct fallback is not supported by
existing files (it would need *new* changes). Hygiene: the engine modernization
is an uncommitted working tree in `/opt/project/taichi` (outside my scope; flag
to commit). **Docs audited & corrected 2026-09-13 (fork `vet-docs`) —
substantively accurate; see "Vetting complete."**

Scope discipline for this work: examine only documents in
`/opt/project/repo/engine` and its subfolders; the main repo is out of bounds
and is a distraction. Follow a link out only if a document requires it; no
free exploring. Create and change **documents only** — no code written
directly. Any code change is dispatched to an agent, only as Patrick instructs.

---

## What this is for (do not generalize this away)

The updated engine and this harness serve **one specific implementation**.
Any benefit others take from the base code is **incidental**. This is not a
general-purpose TaiChi wrapper and must not be designed as one. The engine is
an extract from an **updated TaiChi** that Patrick framed as having **64-bit
addressing** and other changes enabling **much larger SNode structures** than
stock.

> ⚠ **CONFLICT (vetted 2026-09-13, fork `vet-seam-2`): the 64-bit-addressing
> framing is REFUTED as stated, and this is a STOP-and-confirm item.** Logical
> indices / element counts are still **32-bit** (`runtime.cpp:280`,
> `:419-454`); only byte sizes and host directory pointers are 64-bit.
> "Larger SNode structures" is real but comes from **configurable startup
> capacities + a paged ListManager directory**, not 64-bit indexing — the
> fork's own `modernization/IMPLEMENTATION-STATUS.md` (:37, :112) says so
> explicitly. Evidence + verdicts in `VETTING-PLAN.md`.
>
> **RECONCILED (Patrick, 2026-09-13):** the "64-bit addressing" label was
> shorthand for the real intent — **lift the default 1024 SNode capacity
> ceiling and allow full use of modern cards for computation.** Configurable
> startup capacities meet that, so 64-bit *indexing* is NOT required for the
> MVP/PoC. Sufficient if the current setup covers that intent. **Target run
> hardware: an RTX 3060 on another machine once the engine is stable; the 1070
> here is the local dev / measurement card.** Caveat on record: logical
> element counts are per-container `i32` (~2.1B ceiling), which could bind at
> the very top of scale before card memory does — not an MVP blocker. Improving
> beyond MVP is expected to be easy to resource if the concept proves out;
> do not over-polish the engine's scale ceilings now.
>
> **Remediation path (Patrick, 2026-09-13):** if vetting shows the current
> *built* engine falls short of the intent, there are **theoretically updated
> source files** and a **more correct version may be buildable.** So a
> current-build limitation is NOT a hard limit of the engine — it routes to
> (a) an Opus-4.8 investigation to locate/assess the updated source and whether
> the linked `build-review/` archives are stale vs source, then (b) a
> Sonnet-driven rebuild / source-fix if code changes are needed. Do not report
> a current-build shortfall as an engine ceiling without checking this path.

---

## The three layers and the seam

**Engine** — a fixed, built artifact (`engine_support`, assembled from
`/opt/project/taichi`). Pure mechanism: starts an instance, allocates fields,
JIT-compiles and launches kernels, moves data across the host boundary, on the
LLVM CPU path and the GTX 1070. **The engine binary is not recompiled to change
formulas.** Supplying formulas is a runtime path, not a rebuild.

**Harness** — a separate C++ target that links `engine_support`. It **feeds the
engine the formulas and the shapes.** The few core functions and the single
operative shape are written **directly into the harness in C++** — not exposed
as a configurable surface. It builds kernel IR with `IRBuilder`, hands it over,
and the engine JIT-compiles (30–80 ms first launch, then launch-many) and runs.
The harness is also the layer allowed to **shut down and restart the engine**
if the structure itself must be fundamentally updated.

**User terminal / API** — adjusts **within the designed structure**, never
redefining it. Stays minimal, because there is no broad formula surface to
drive.

Why separate: so the user API can drive the engine's lifecycle (shut
down / restart) through the harness when a fundamental structural update is
needed, without the user API being the thing that owns the frozen structure.

---

## The corrected model — why C++, not Python (literal)

Stock TaiChi's Python wrapper exists to **expose extensive formula access and
user tooling** — an abstraction layer for arbitrary user models. **This use
case does not need that layer, so it is deliberately absent.**

This use case has **very few functions** and **a pure sphere as the only
operative shape**, so they can be written into the harness directly. It is
done in **C++ because no abstraction is required** — just **pure formula
resolution speed across simple kernels.** C++ is not chosen for control; it is
chosen because there is nothing to abstract.

Consequence: harness is thin and concrete. Sphere + the few field formulas
hardcoded, compiled to IR once, launched per tick with live values as
arguments. One shape ⇒ effectively one layout and a small fixed kernel set.

---

## Model foundation (literal — this is the operative model)

Patrick, verbatim: *"The operative model is n-dimensional field analysis
expressed in 3 Newtonian dimensions. For this model, the correct mathematical
definition of dimension applies. A dimension is any plane of commonality
between 2 or more points."*

Load-bearing reading (to be validated as Patrick continues — do not treat as
settled beyond his words above):

- **Dimension is defined relationally, from points — not as a pre-given
  axis.** A dimension *is* a plane of commonality between ≥2 points. No points
  sharing → no dimension. Dimensions are constituted by points having
  something in common, not by a coordinate frame imposed first.
- **"n-dimensional"** therefore = as many dimensions as there are planes of
  commonality among the points. Not fixed at 3.
- **"expressed in 3 Newtonian dimensions"** = the field is realized/rendered
  in ordinary 3-space (where the one operative shape, the sphere, lives). The
  3D is the **expression layer**; the **analysis** is n-dimensional over the
  planes of commonality.
- The usual "space first, points located in it" is **inverted**: points and
  their commonalities come first; dimensions emerge from them.

To pin as this develops: what a "point" is in engine terms (its relation to
the fixed-numeric-slot particle), and whether a plane of commonality is
pairwise or over a set. Not yet stated — do not assume.

### Geometry and level of detail (literal — the operative shape)

Patrick, verbatim: *"the model is in 3 dimensions spacially, with every object
being a perfect sphere of exactly 1 particle at the Level of Detail where it is
the primary. a particle may be composed of any number of inner particles, but
at the bottom active level of detail, each is an orb."*

Reading:

- **Spatially 3D; the sole shape primitive is the perfect sphere (orb).** No
  other geometry exists in the model. This gives meaning to the earlier "pure
  sphere as the only operative shape."
- **Fixed size (Patrick, 2026-09-13): every sphere is diameter 1 — one particle
  across (radius 0.5).** No per-sphere size parameter and no stored constants; a
  particle is a position carrying unit diameter. (Contact therefore occurs at
  centre-distance 1 — sum of radii — feeding the pending contact mechanics.)
- **At the LOD where an object is *primary*, it IS exactly one particle = one
  perfect sphere.** "Object (at its primary LOD)", "particle", and "orb/sphere"
  are the same thing at that level. One primary object ⇒ one sphere ⇒ one
  particle — never a cluster.
- **Composition is recursive:** a particle may contain **any number of inner
  particles.** The same object resolved at a finer LOD becomes multiple inner
  orbs.
- **The active analysis has a floor:** at the **bottom active LOD, everything
  is an orb.** Analysis bottoms out in spheres, not in any other primitive.

Consequences to hold (validate as it develops):

- LOD is a hierarchy of nested spheres; "primary" is level-relative. The
  resident/active set the engine computes is the set of orbs at the bottom
  active LOD; higher LODs aggregate orbs into a single representative sphere.
- "any number of inner particles" is **variable composition** — the
  structural, changeable part — distinct from the per-particle fixed numeric
  slot payload (which is consistent in shape). This variable composition is
  plausibly where virtual-node promotion / SNode reconstruction and the
  uncalibrated-dimension analysis act. To confirm.
- To pin (not yet stated): whether the "primary" / bottom-active LOD is a
  per-analysis selection or derived; and how composition (inner particles) is
  represented against the dense fixed-slot layout.

### The core interaction and the SNode roll-up (literal — the one formula)

Patrick, verbatim: *"The active SNode hierarchy is not field effects, but
structural groupings used in rolling up Level of Detail items and the
accompanying field values. where ever the current analysis is focused, more
distant objects, as determined by the LoD and neighbourhood of focus, are
rolled to less granular field effects. Because field effects are the only
operative primitive, the core formula for every interaction is m1*m2/d^2 with
m2 representing the centroid mass and position of the entire field including
all members of the dimension, or field being calculated."*

Reading:

- **The SNode hierarchy is STRUCTURE, not force.** It is the set of structural
  groupings that **roll up LoD items and their accompanying field values** — it
  is not itself the field effects. Harness consequence: the SNode tree stores
  the rolled-up centroid values per grouping; the forces are *computed*, not
  encoded as tree topology.
- **Roll-up is focus-relative.** Wherever analysis is focused, objects more
  distant — as determined by **LoD and the neighbourhood of focus** — are
  represented as **less granular field effects** (coarser groupings). Near
  focus = fine (individual orbs); far = coarse (aggregated groupings).
  **(Refined 2026-09-13 — see "Roll-up, corrected": a distant element defaults
  to its primary centroid and expands to granular only *if relevant*; this is
  demand-driven, not automatic by distance, and only SNode-tree chains are LoD
  rollups.)**
- **One operative primitive: field effects. One formula for every interaction:
  `m1 * m2 / d^2`** (inverse-square).
- **m2 is an aggregate, not a peer.** m2 = the **centroid mass and position of
  the ENTIRE field** — all members of the dimension / field being calculated.
  Every interaction is m1 (the focused object) against the **centroid of a
  whole field**, summarised to one mass at one position. The LOD roll-up is
  what produces those centroids at each granularity.

How this ties the pieces together:

- A **dimension** (plane of commonality) selects the members that form a
  **field**; those members' orb masses+positions roll up to a **centroid (mass
  + position)** = the field's rolled-up value stored in the SNode grouping.
- Computing field F for object m1 = `m1 * m2_F / d^2`, where m2_F is F's
  centroid and d is the distance to it. Same formula for every field.
- **"Which forces are active" (the API toggle) reconciles to: which
  dimensions/fields are included** in the calculation for a given aspect of
  analysis. Each included field uses the same core formula; toggling one is a
  scalar launch arg (no recompile), per the vetted seam.

**d RESOLVED (Patrick, 2026-09-13):** `d` is the spatial distance from m1 to the
field's amalgamated **virtual particle** — its **force centroid** (see the
virtual-particle subsection below). m2 sits at that force centroid carrying the
group's cumulative effective mass. (The internal-ordering and numbering senses
of "position" are separate from this `d`.)

(NOT importing any named tree/aggregation technique here — this records
Patrick's literal description only. See `off-median-no-imported-machinery`.)

### Participation: Parent and Sibling — the two payload data types (literal)

Patrick, verbatim: *"no special rules. every group pulls to itself. The 2 kinds
of data in the particle payload define which fields and to what degree a
particle participates in. The distinct data types are Parent and Sibling
relationships. Parent relationships are tied to the internal predicate
components of a given particle and act only on the ratio of mass expressed in
the parent listing. the operative example is a magnet embedded in a non
magnetic material, the magnetic pull only affects the magnet and affects the
whole proportionally. A sibling relationship is a grouping that the entire body
of the particle participates in equally and is listed by group name only. in
the storage db everything uses token_ids and arrayed addressing storage but the
composed tree can be any required shape."*

Reading:

- **No special rules; every group pulls to itself.** All field effects are the
  same inverse-square pull toward a group's own centroid. No exclusion/repulsion
  special case at this level — the earlier commit-message hypothesis (identity
  pull / exclusion / condensation) is NOT the operative structure; this is. Any
  apparent repulsion would be emergent, not a rule (confirm).
- **The payload has exactly two data kinds; together they set which fields a
  particle joins and to what degree:**
  - **Parent relationship — partial, mass-ratio participation.** Tied to the
    particle's **internal predicate components** (its inner particles). The
    field acts **only on the ratio of mass** that component holds in the parent
    listing, and **affects the whole particle proportionally.** Operative
    example: a magnet embedded in non-magnetic material — the magnetic pull acts
    only on the magnet's mass fraction, and the whole body responds in
    proportion. Degree = the internal component's **mass ratio**.
  - **Sibling relationship — full, equal participation.** A grouping the
    **entire body** participates in **equally**, **listed by group name only**
    (no ratio). Degree = full mass.
- **A "field" (dimension) is therefore either** a **sibling group** (members at
  full mass, keyed by group name) or a **parent predicate component** (members
  at their internal-component mass ratio). Its centroid m2 (mass + position)
  rolls up the participating masses accordingly; interaction is
  `m1 * m2 / d^2` with the participation weighting.

Storage vs composition:

- **Storage DB: flat, `token_id`-keyed, arrayed addressing.** The ledger is all
  token_ids + arrayed storage — canonical, dense, array-addressed (fits the
  engine's dense fields + arrayed indices).
- **The composed tree can be any required shape.** The active SNode grouping is
  built from the flat store into whatever LOD/roll-up shape the analysis needs —
  reconcilable with the vetted `add_snode_tree` (compose any shape on a live
  runtime; restart only for a capacity change).
- So: the **DB is the flat arrayed truth**; the **composed tree is a derived,
  any-shape grouping** over it. This is the concrete form of "ledger of known
  relationships" (flat) vs "active SNode hierarchy" (composed structure).

To pin / hold (confirm, do not assume):

- Parent lists (component → mass ratio) and Sibling lists (group names) are
  **variable in count per particle**, which tensions with the earlier "fixed
  numeric slots, consistent shape." Likely reconciled by storing them as arrayed
  **`token_id` references** into shared parent/group listings rather than inline
  variable payloads, with the composed tree carrying the variability — but that
  is my inference; confirm the payload layout against the dense fixed-slot field.
- Whether "internal predicate components" are literally the inner particles
  (recursive orbs) or a separate predicate decomposition.

(Literal record of Patrick's definition; no imported machinery. See
`off-median-no-imported-machinery`.)

### DB structure and traversal (literal)

Patrick, verbatim: *"the db structure is very lean and has almost identical
data to the particle, which is just predicates, fields and forces. the only
data structure addition to the db is that the entries also contain child
listings so the db is n-dimensionally traversable by CPU threads."*

Reading:

- **The DB is very lean and nearly identical to the particle.** A particle (and
  a DB entry) **is just three things: predicates, fields, and forces.**
- **The one DB-only addition: child listings.** DB entries carry **child
  listings** on top of the particle data, whose purpose is to make the DB
  **n-dimensionally traversable by CPU threads** — the downward links a CPU walk
  follows across dimensions.
- So the flat `token_id` arrayed DB = particle data {predicates, fields, forces}
  + child listings; **CPU threads traverse it n-dimensionally** (following child
  listings). That traversal is plausibly where the LoD roll-up / tree
  composition is driven, on the CPU side, before the GPU does the inverse-square
  field math. (CPU = engine `cpu_threads` / C++ marshalling — Python stays
  front-end-feed only.)
- **Save state = positional data (Patrick, 2026-09-13).** If a checkpoint is
  needed it is saved as the model's **positional data.** So **positions are the
  dynamic state**; predicates / fields / forces / child-listings are standing
  structure that recomposes. Persistence and readback across the host boundary
  are therefore essentially the positions.

**Mapping RESOLVED (Patrick, 2026-09-13):**

- **Parents are predicates.** The Parent relationships ARE the "predicates" —
  internal predicate components, partial by mass ratio.
- **Every listing is a field.** Each listing a particle is in — Parent OR
  Sibling — is a field. "fields" = the set of listings it participates in.
  Parent-listings are predicate fields (partial, mass-ratio); Sibling-listings
  are full-mass group fields (by name).
- **The mass involved is the force.** "forces" is NOT a separate stored datum —
  the force of an interaction = the participating mass (full for sibling, ratio
  for parent). Consistent with `m1*m2/d^2`: the masses are the forces.

So a particle collapses to: a **position** + a set of **field listings**, each
carrying a **participating mass** (= its force in that field), where
Parent-listings are predicates (ratio mass) and Sibling-listings are full-mass
memberships. DB entries add **child listings** for CPU traversal.

Translation implication (mine, hold for design): the payload is naturally a
**list of (field `token_id`, participating-mass) entries + position**, not a
rigid fixed-slot row — exactly where "fixed slots may not be appropriate."

**Child listings RESOLVED (Patrick, 2026-09-13):** a child is simply *"this
appears in these constructs as a unit"* — a lean membership reference recording
which **constructs** the entry appears in, **as a whole unit** (not decomposed).
This is the DB-only traversal addition, and it is **structural**, distinct from
field/force participation: predicates (parents) + sibling fields drive the
`m1*m2/d^2` physics; **child listings drive the n-dimensional traversal / LoD
composition** the CPU threads walk. (My read to confirm: child listings
navigate the composed constructs — they are not themselves a force term.)

### Position, disambiguated — and the "CPU never searches, only follows" law (literal)

Patrick, verbatim: *"the intent is for the CPU to never have to search, only
follow in the db. you are getting muddy with position because it is carrying a
few terms at once. within a construct, the internal elements have a fixed
ordering. they are what they are because of that precise configuration. the
positional order of those in the chain that makes the element at the higher LoD
is expressed in the parent table and acts on the whole based on its position vs
the center. the numbering also provides some additional benefits in analytical
coupling, not direct bonding. That will be clarified in a moment in distance and
contact mechanics."*

**Design law (hard): the CPU never searches — it only follows in the DB.** Every
traversal is a direct follow (child listings + the fixed orderings below); no
scan, no lookup. This constrains the layout: linkage and ordering must be laid
so any needed walk is pointer/index following, never a search.

"Position" carries several distinct terms — do NOT collapse them again:

1. **Internal ordering = configuration = identity.** Within a construct the
   internal elements have a **fixed ordering**; they "are what they are because
   of that precise configuration" — the arrangement of the internal chain IS the
   construct's identity.
2. **The parent table expresses that positional order,** and each internal
   element **acts on the whole based on its position vs the center.** This
   extends Parent (predicate, partial by mass ratio): the contribution depends
   not only on the mass ratio but on the ordered position relative to the
   construct's centre (e.g. an off-centre component).
3. **Numbering → analytical coupling, NOT direct bonding.** The ordinal
   numbering additionally provides benefits in **analytical coupling** — a
   coupling used in analysis, explicitly *not* direct/physical bonding. TBD.
4. **(Separate) the 3D spatial location** — the Newtonian expression I fixated
   on. Its exact role in `d` awaits the next item.

**The ball-vs-line image and the rotation ruling (Patrick, 2026-09-13).** From a
contact / location perspective the parent predicates are "snarled up in a ball"
in the sphere that represents the whole; but **operationally they are treated as
a line across the middle of the sphere — the components in order, rotating on
the middle.** The "rotation" is the orientation of that ordered line about the
centre: an **ordering-and-alignment trick, NOT running rotational dynamics.** It
sits in the ordering-and-connection family (with internal-ordering = identity
and numbering → analytical coupling), and Patrick will discuss it **"as it is
closer" — do not build it now.**

- **Rotation ruling:** when Patrick earlier called torque "unnecessary" he meant
  **as a running tick-to-tick concern** — nothing physically spins with conserved
  angular momentum carried between ticks. So the degraded model's rigid-body
  integration (inertia, torque → angular velocity, carried spin) is **drift** —
  imported machinery filling this gap. "Position vs centre" is a component's
  **ordered place along that diameter** (for alignment / coupling), not a lever
  arm generating spin.

**Coming next (Patrick): distance and contact mechanics** — will clarify how
these senses of position/numbering resolve into `d` and into coupling vs bonding.

### m2, the force centroid, and the tick's virtual particles (literal)

Patrick, verbatim: *"as noted m2 is read as the cumulative mass at the force
centroid of the group at question. at end of every tick, the force centroid and
mass of every field at play is calculated and assigned to a virtual particle of
no real mass or presence but with the effective mass of the group as a whole."*

Reading:

- **m2 = cumulative (total) mass at the force centroid of the group in
  question.** Its position is the group's **force centroid**; its mass is the
  group's cumulative/effective mass.
- **End of every tick, per field at play:** compute the **force centroid**
  (position) and the **mass** of the field, and assign them to a **virtual
  particle** — **no real mass or presence** (not a physical member; not pulled,
  does not occupy) **but carrying the effective mass of the whole group** at the
  force centroid.
- **That virtual particle IS the m2** used in the next tick: interaction is
  `m1 * m2 / d^2` with m2 = the field's virtual particle, and **`d` = distance
  from m1 to that virtual particle (the force centroid).**
- **These virtual particles are the "virtual nodes"** referenced earlier (used
  in analysis; promoted into the SNode model via reconstruction when relevant).
  Per-tick amalgamation creates/refreshes them.
- **This is the tick cleanup / amalgamation, and it IS the next-tick setup
  (Patrick, 2026-09-13):** at tick end each field's members are amalgamated into
  one virtual particle (force centroid + effective mass). There is **no separate
  "tick setup for the following tick"** — the amalgamation is it. **The centroids
  calculated and assigned are exactly what feeds m2 in the next tick:** each
  field's virtual particle is the m2 that the next tick's `m1 * m2 / d²` reads.
  So the loop **closes on its own tail** — tick N's amalgamation mints tick
  N+1's m2 sources, which is why it runs at tick end on the hot resident data
  and does not gate the next launch. (Residual: whether amalgamation is one of
  "the 2 things that need to be added" per tick, and the identity of the other,
  is still open — see Open threads.)
- **Why it runs at END of tick (Patrick, 2026-09-13):** all required factors are
  **still resident in memory from the tick's calculation**, so amalgamating then
  **reuses the hot data** and **avoids the bus traffic** of re-gathering it — and
  it **does not gate the next tick's start** (the m2 sources are already
  prepared). Doing it at the next tick's beginning would force a reload and block
  the tick on it. **Translation implication:** fold the amalgamation into the
  **tail of the tick's own launch**, reading the just-computed state **in place /
  device-resident** and writing the virtual particles — no host round-trip, no
  re-gather, so the next tick launches clean.

**Roll-up, corrected (Patrick, 2026-09-13)** — I had this messy (not wrong, but
loose):

- A more distant element **focuses on its primary centroid**, and **if relevant
  is expanded into granular form.** This is **demand-driven expansion**, not
  automatic coarsening by distance.
- **Not all centroids are LoD rollups.** Every field's per-tick centroid virtual
  particle exists as a prepaid interaction target; the **LoD-rollup hierarchy is
  only the chains specified in the SNode tree for that analysis.** So: centroids
  everywhere (amortization), but rollup chains only where the SNode tree says.
- **The centroid calculation prepays the pairwise tax** on a group in live
  computation — one interaction with the centroid instead of pairwise with every
  member — and it is **faithful, not a mere approximation, because the centroid
  is the end structure the group develops into.**
- **Why it is exact (Patrick, 2026-09-13):** *"if you have a full pairwise
  comparison of any group on field interactions all end up in relationships to
  the force and mass centroid by nature of the math."* The full pairwise sum of
  a group's field interactions **resolves, by the math (inverse-square
  superposition), to relationships with the force-and-mass centroid.** So the
  centroid relationship *is* the pairwise result — prepaying it loses nothing.
  **Invariant CONFIRMED (Patrick, 2026-09-13): the force centroid and the mass
  centroid are ALWAYS the same** — *"the nature would be warped to vary."* They
  coincide by construction (mass = force); any divergence is a warping of the
  nature, i.e. a defect. Treat as a **correctness invariant / natural test
  point** for the translation: assert force-centroid == mass-centroid.

### Units and the per-tick move (literal)

Patrick, verbatim: *"the primary force ratio is 1:1, so 1 force moves a 1 mass
particle 1 space in 1 tick."*

Reading:

- **Primary force ratio = 1:1.** Unit calibration: **1 force moves a 1-mass
  particle 1 space in 1 tick.**
- ⇒ **Per-tick displacement = net force / mass** in these units, for the primary
  force: `Δposition = F_net / m` per tick. As stated ("moves 1 space in 1
  tick") this is a **direct first-order move** — force produces displacement
  within the tick — not an acceleration term.
- Force = the field interactions `Σ (m · m2 / d²)` over the active fields' virtual
  particles; net force is the **vector** sum, and the move is along it.
- **Superposition CONFIRMED (Patrick, 2026-09-13): the expressed vector of
  motion of a particle is the vector SUM of all vectors acting on it** (plus
  carried momentum). This is the same linearity that collapses a group's
  pairwise sum to its centroid.
- **On "primary" ratios (Patrick, 2026-09-13):** none known on primary; but
  **in combination, forces can present differently** — emergent variance from
  combination, not special per-force ratios (consistent with "no special
  rules").

Translation implication: the move step is cheap calculation — `x += F_net / m`
per particle (1:1), reusing resident force + mass + position, no allocation.

**Momentum RESOLVED (Patrick, 2026-09-13):** velocity / momentum **do carry
between ticks** — the move is inertial / second-order, not overdamped.

**The further element (Patrick, 2026-09-13): a discretization correction.** The
**goal is an optimally settled state** — the dynamics should converge to
settlement (field balance). Discrete ticks can inject **kinetic-energy
imbalances** that **impede settling and cause shear / instability**, so this
element corrects the tick's KE imbalance to approximate the **continuous**
effect and **ease settling**. It is a *fidelity-to-continuous* correction —
**NOT arbitrary damping and NOT an imported integrator scheme.**

**Mechanism (Patrick, 2026-09-13):** the expressed vector is **reduced
exponentially as the particle nears the "destination point" of the current
vector.** Because it is exponential, the attenuation is **unity (≈1) at the
beginning of any tick** and only applies **aggressively if the particle would
overshoot the goal within that tick.** Effect: the tick's move **approaches the
destination without overshooting** — a continuous-like approach that eases
settling and removes the overshoot-induced KE imbalance / shear.

- Elegantly this is **branchless by nature**: one exponential expression is ≈1
  for normal steps and self-limits a would-be overshoot — no `if overshoot then
  clamp`. **The function *is* the gate.** Honours no-special-cases /
  bounded-in-representation, keeps the primary kernel pure replication, and is
  consistent with the d=0 gate (motion → 0 at the destination).
- Exact algebraic form is my translation job, to firm up with Patrick — keep it
  exponential, unity at tick start, overshoot-limiting, branchless
  (`off-median-no-imported-machinery`).
- **Cost / placement (Patrick, 2026-09-13):** the exponential runs **once per
  particle, on the resultant (net) velocity** — not per interaction — so it is
  one transcendental op per particle per tick (cheap; the GPU has fast
  transcendental units). It **may fold into the original equation / the one
  primary kernel**: sum field vectors → resultant velocity → exponential
  overshoot-attenuation (+ the d=0 gate) → move, all in one replicated kernel.
  Confirms the kernel-shape target holds with the exp included.
- **Per-vector option (Patrick, 2026-09-13):** applying the attenuation to
  **every vector** (each interaction, pre-sum) **"wouldn't hurt"** —
  correctness-safe — **but only if it fits in** (the one kernel / compute
  budget). Per-vector = one exp per interaction (many); per-resultant = one exp
  per particle (cheap). Translation latitude: **default to the resultant** (the
  cost model favours it); per-vector is an acceptable finer application **only if
  the budget/kernel allows.** (exp is nonlinear, so per-vector ≠ per-resultant
  numerically — Patrick has blessed both.)

**d=0 gate — part of the equation, not a code special-case (Patrick,
2026-09-13):** the primary calculation needs a gate: **if d=0, v=0.** It covers
a **sole-item centroid** (a group of one → its centroid is itself → d=0) and a
particle **sitting exactly on the centroid.** It avoids the `1/d²` singularity
by yielding **zero**, not infinity.

- **Must be expressed within the equation** (a gate that is part of the maths),
  **not an `if` branch / special case** — honours no-tricks / no-special-cases /
  bounded-in-representation (`field-engine-no-exceptions`,
  `works-in-shape-of-math`).
- Translation approach (mine, to firm up): an **exact branchless zeroing** — the
  interaction term multiplied by `[d ≠ 0]` (1 when d≠0, 0 when d=0) — **exact
  zero, NOT epsilon-softening.** Confirm the algebraic form Patrick wants; do not
  import a softening trick.

### Known vs uncalibrated dimensions, the ledger, and reconstruction (literal)

Patrick, verbatim: *"dimensions are declared in the database if known but
analysis of uncalibrated ones is the analytical goal. the db structure is a
ledger of known relationships. SNode reconstruction would be required when the
virtual nodes used in analysis have enough relevance to need direct
incorporation into the overall model. that is why the forces themselves must
remain manipulable via the API. not the formula used for them, but whether
they are active for a particular aspect of analysis."*

Reading:

- **Known dimensions are declared in the database.** The DB structure is a
  **ledger of known relationships** — the calibrated planes of commonality
  already established.
- **The analytical goal is the *uncalibrated* dimensions** — the planes of
  commonality not yet known/calibrated. The work is to analyze those.
- **Virtual nodes** carry the uncalibrated dimensions *during* analysis. They
  are used in analysis but are **not yet part of the SNode structure**.
- **Promotion → SNode reconstruction.** When a virtual node accumulates enough
  relevance to need **direct incorporation into the overall model**, the SNode
  structure is **reconstructed** to include it. This is a layout change, which
  on this engine means a rebuilt structure — the fundamental-structural-update
  seam. **This is the reason the engine and harness are separate and the
  harness owns shut-down/restart.** (Reinstates the seam I had wrongly dropped.)

So there are two distinct rates of change:
1. **Fast, live, no restart:** which **forces are active** for a given aspect
   of analysis (see manipulable surface below).
2. **Slow, structural, rebuild:** a relevant virtual node graduating from the
   ledger into the resident SNode model → SNode reconstruction via the harness.

**Design implication (vetted 2026-09-13):** live `add_snode_tree` recycles
*tree* slots, but per-node **SNode ids are monotonic across destroyed layouts**
within a Program lifetime. So repeatedly promoting virtual nodes consumes the
configured `llvm_snode_capacity` monotonically. The harness therefore either
budgets a generous capacity up front, or uses its **restart authority to
reclaim the exhausted id space** — a real second reason the harness owns
shut-down/restart, beyond changing capacities.

### Manipulable surface — forces, not formulas (literal, load-bearing)

- **The forces must remain manipulable via the API** — but **not the formula
  used for them.** What is manipulable is **whether a force is active for a
  particular aspect of analysis.** Formula = fixed (hardcoded C++ in the
  harness). Force activation = live, API-driven.
- Mechanically this fits the engine cleanly: an activation toggle is a
  **launch argument** (a scalar/flag), so switching a force on/off for an
  aspect of analysis needs **no recompile and no restart** — only a layout
  change (i.e. SNode reconstruction) forces the rebuild.
- Observed context (from the opening git status / recent commits, NOT yet
  confirmed by Patrick in this thread, and the `field/*.json` live outside my
  scope so I have not opened them): control configs named `ctl-off-excl`,
  `ctl-off-pull`, `open-off-excl`, `open-off-pull`, and commit text "identity
  pull, exclusion, condensation" suggest the forces include **identity pull,
  exclusion, condensation**, with the `off-*` variants being exactly these
  activation toggles. Treat as hypothesis to confirm, not fact.

## Division of labour (Patrick ↔ me) — 2026-09-13

**Patrick owns the model:** how the **DB** and the **functions** need to work —
the data (Parent/Sibling participation, `token_id` arrayed storage), the
semantics, and the formulas (all inverse-square `m1*m2/d^2`, participation-
weighted, no special rules). He knows this.

**I own the translation:** expressing that model **through C++ into Taichi** —
the SNode tree shape, the field/payload layout, the kernel IR, argument
passing, the LoD roll-up composition, and the host boundary. I bring the
engine's realities (dense arrayed fields, i32 indices, the vetted
capacity/seam constraints) to bear on the shaping.

**The payload shape is OPEN.** "Fixed numeric slots per particle" (from
`DEVICES.md`, an unvetted sizing sketch) is **NOT a given** — whether it is the
right shape is a translation-design question Patrick has explicitly asked me to
help determine. Shape it to the model, and keep the representation
combinatorially bounded — no special-casing in code (see
`works-in-shape-of-math`, `field-engine-no-exceptions`).

## Cost model — Patrick's three rules of computing (literal, load-bearing)

Patrick, verbatim: *"I follow three rules of computing. Declaration is near
free, calculation has a cost, allocation is expensive. Declaration touches 1
data point, calculation at least 3 plus the computation resource, allocation
costs it plus the work that required it."*

Cheap → expensive:

1. **Declaration — near free.** Touches **1** data point (name/reference a
   value).
2. **Calculation — has a cost.** Touches **≥3** data points **plus the
   computation resource** (e.g. `a = b op c`: three operands + the compute
   unit).
3. **Allocation — expensive.** Costs **the allocation itself plus the work that
   required it** — it cascades, dragging in the reason it was needed.

**Design law for the translation: prefer declaration, ration calculation, avoid
allocation.** Concretely against this engine:

- **Allocate once, up front, reuse.** Fix the pool at instantiation (32 MiB
  granularity; the vetted "pool fixed once" pattern). No per-tick allocation.
- **Keep data resident; don't re-gather.** Re-gathering is extra
  calculation/allocation; the end-of-tick amalgamation reuses hot data exactly
  to honour this.
- **Lean on declaration / args.** Launch arguments (loop bounds, scalars,
  arrays) are declarations, not recompiles or allocations — the vetted seam.
  Toggle forces, sizes, focus via args.
- **Minimise touches in kernels.** Calculation is ≥3 touches + resource;
  prepaid centroids reduce a group's pairwise touches to one per group.

Apply every translation decision against this.

## Physics as an analytical tool (Patrick, 2026-09-13)

The model **uses physics as an analytical tool**, not as an end in itself
(n-dimensional field *analysis* expressed in 3 Newtonian dimensions). So **minor
adaptations** — the exponential discretization fix, per-vector vs resultant,
etc. — **feed the analytical purpose and are legitimate so long as they do not
distort what is being displayed.**

**The test for any translation adaptation:** does it serve the analytical
purpose *without distorting the displayed result*? This is not a licence for
tricks — it is why a purpose-serving, uniform, display-preserving adaptation is
fine while a distorting hack is not. (Reconciles with
`field-engine-no-exceptions` / `off-median`: no special-casing in code, but
purpose-serving adaptations that preserve the display are the intended mode.)

The exponential was chosen because it was **the easiest way to scale a
discretization fix** — being exponential it self-scales across vector magnitudes
(unity at tick start, aggressive only on overshoot) with no per-magnitude tuning.

## Kernel shape target (Patrick, 2026-09-13)

Get the **primary calculations into ONE kernel** → then it is **all
replication**: the same kernel applied identically across every particle in one
data-parallel launch (the vetted one-compiled-kernel / name-keyed path). The
primary per-particle work — the field interaction `Σ (m · m2 / d²)`, the move
`x += F_net / m` (1:1), the momentum carry, and the **branchless d=0 gate** —
must be **uniform: no branches, no special cases**, so it replicates without
divergence (also the GPU-fast path). This is *why* the gate lives in the
equation, not in an `if`.

Patrick notes **more than one kernel is involved overall** (e.g. end-of-tick
amalgamation, CPU traversal/composition, setup) — the "one kernel" target is
specifically the **primary calculations**, not the whole system.

## What the engine offers (four primitives the harness must own)

1. **Instance.** One `engine::Runtime` = one `Program` + every allocation made
   against it. `InstanceSettings`: arch, two startup capacities, device-memory
   budget, launch thread count — each optional; unset = engine default.
   Capacities are **frozen once the instance exists**, chosen before
   construction; they bound SNode/tree **identifiers issued over the whole
   process life** (monotonic counter; destroyed layouts still count), not live
   objects. Destroying the runtime releases allocations and invalidates
   everything built on it.
2. **State / layout.** `SNode` tree: `root->dense(Axis(0), n)` or
   `root->pointer(...)`; leaf gets a `dt`; hand root to `add_snode_tree`. Trees
   on a live runtime coexist.
3. **Kernels.** Build IR with `IRBuilder`, wrap in `engine::CompiledKernel`
   with a name + parameter list. Compile once, launch many; arg ids follow
   declaration order.
4. **Boundary.** `engine::upload` / `engine::readback` move a whole `Ndarray`
   per call. Kernels reach it via `create_ndarray_arg_load` +
   `create_external_ptr`; a field via `create_global_ptr`.
   `runtime.synchronize()` before any readback.

Shortest complete example of all four: `tests/engine_smoke_test.cpp`.
Authoritative calling convention: `docs/api-reference/ir_builder_test.cpp`.

---

## Hard engine constraints (break these and the run aborts or corrupts)

- **Top-level loop must carry a thread count.** The outermost loop becomes the
  offloaded task; launching with zero threads aborts in `threading.cpp`. Pass
  `runtime.cpu_threads()` (= `cpu_max_num_threads`). Nested loops run serially,
  no count needed.
- **Kernel name is the compilation key.** Two kernels with the same name share
  one compilation; the second silently runs the first's code. **Every compiled
  kernel needs a distinct name.**
- **Only a layout change forces recompilation.** A loop bound, a scalar, or an
  array can be a launch argument (`create_arg_load`,
  `create_ndarray_arg_load`), so one compiled kernel serves many launches.
- **Capacities read before the runtime exists** and snapshotted at startup
  (`CompileConfig` copied into `Program` at construction). Env vars:
  `TI_LLVM_SNODE_CAPACITY`, `TI_LLVM_SNODE_TREE_CAPACITY` (strict positive
  int). `TI_LIB_DIR` is mandatory or the engine aborts.
- **Runtime ABI version = 1.** Offline cache off by default (kernels
  recompiled in-process each run). An AOT/offline dir must carry a matching
  `runtime_abi` file.
- **CUDA device is visible-ordinal-zero**, no index setting; choose with
  `CUDA_VISIBLE_DEVICES` before CUDA starts, or `set_visible_cuda_devices` in
  process before any CUDA call.

---

## Device envelope (GTX 1070, measured 2026-09-12 — the target)

- Target the **1070**: 7.84 GiB free of 7.92. 750 Ti is **deferred** (first
  field allocation refused by the driver). CPU path is the reference, not the
  throughput path.
- **32 MiB reservation granularity** — a small field still takes 32 MiB; a few
  wide fields cost less overhead than many narrow ones.
- Field storage: **exactly 4 bytes per i32 cell.**
- **Precision cost:** f64 ≈ 4× f32 when bandwidth-bound, ≈ 27× when
  arithmetic-bound. A row-shaped, low-arithmetic working set sits near the 4×.
- **Pool fixed once at instantiation** is the intended pattern (pays startup
  cost once; per-tick cost thereafter). Destroying an instance does not return
  all memory to the driver — a long-lived, fix-once process never meets this.
- **Data shape (Patrick, 2026-09-12):** a particle's data is database rows, a
  few hundred at most, absolutely consistent in shape, no vertex/mesh payload,
  used only as variables in calculations ⇒ fixed numeric slots per particle,
  **dense containers suffice** (sparse only if occupancy is sparse), and **one
  compiled kernel serves the whole set** because nothing about layout varies
  per particle. **(Update 2026-09-13: the fixed-slot shape is now OPEN — may or
  may not be appropriate; it is a translation-design question — see Division of
  labour. The dense-containers and one-kernel points still hold.)**
- **Capacity is division:** `pool bytes / (slots per particle × 4)` particles.
  E.g. 7.5 GiB holds 31.4M @ 64 slots, 7.9M @ 256, 2.0M @ 1024.

---

## Operating discipline for this thread

- Patrick's structural claims are **literal**, not analogy. Do not hedge with
  "like"/"as if"/"metaphor."
- Do **not** collapse these distinctions into a median/generic model. Preserve
  the exact wording of the distinctions above.
- Engine binary fixed ↔ formulas fed at runtime is a distinction that keeps
  getting re-flattened; hold it.
- Canadian English in any prose. Every artifact ships with tests.
- **Agent model policy (Patrick, 2026-09-13):** investigation agents run on
  **Opus 4.8** — specify **opus-4-8** directly (Patrick confirms it selects
  cleanly, no issues, same as this terminal). **Coding is Sonnet only** (less
  creative in application → sticks to instructions; pass `model: "sonnet"`).
  **Opus 5 and Fable 5 are banned** — if any agent is running on Opus 5 or
  Fable 5, **stop it and discard its results.** A `fork` also guarantees
  Opus 4.8 (inherits this terminal's model) and is the choice when the agent
  should carry this conversation's context; otherwise pin opus-4-8 on a fresh
  agent.
- **Sonnet capability profile (Patrick, 2026-09-13):** Sonnet is **good at
  linear coding** but **requires validation** and **does not handle large
  structure well.** ⇒ Opus 4.8 (me) owns the structure and decomposition; Sonnet
  gets **small, linear, fully-specified units**, one at a time, no
  large-structure judgment; **every unit is validated (tests + review) before
  it is trusted.** Decomposition detail in `HARNESS-STRUCTURE.md`.
- Verify any file:line / behaviour claim against live code before asserting —
  the doc claims are point-in-time.
- **Do not enshrine counts / recorded numbers (Patrick, 2026-09-13).** A number
  written once and never revised goes stale as the design grows ("2 things to
  add" is the example). Record structure and update figures when they move;
  treat any bare count as descriptive-at-the-time, not a fixed target.
- **Patrick's explanation governs — when in doubt, go with it (Patrick,
  2026-09-13).** Where the code, my inference, or a doc conflicts with what
  Patrick has stated, his explanation wins; do NOT re-open a thing he has
  already defined as an "open question," and do NOT float the drifted code's
  version as a live alternative to his definition. Drift appears fast in this
  context — apply his stated model, don't re-litigate it.
- **Do NOT pinball on corrections (Patrick, 2026-09-14).** When Patrick corrects
  one thing, absorb it **narrowly** and hold the rest of the model steady. Do NOT
  take a single correction and (a) generalize it into a sweeping restructuring,
  (b) stamp the result "locked"/"absolute," or (c) declare other pieces
  "redundant." Patrick's words: "I try and correct and you CAREEN and declare it
  absolute and other things redundant." This is the same fatal failure as the
  degraded model (right in general, careens on specifics) and it forces Patrick to
  spend every message walking back an over-extension. Take each correction as a
  local refinement; let Patrick pace the tightening; don't pronounce parts
  final/redundant unless he did. (See memory `feedback-dont-pinball-on-corrections`.)

---

## Vetting complete

- **`vet-docs`** (Opus 4.8 fork, read-only) — DONE 2026-09-13. All five docs
  audited against live source: **substantively accurate, no misleading
  claims.** Parent fixed four line-number drifts (`lang_util.cpp` :30→:35;
  `llvm_runtime_executor.cpp` :41→:43 and :712→:713 in CONFIGURATION.md;
  `cuda_context.cpp` :50→:39 in DEVICES.md). The three api-reference `.cpp`
  copies are byte-identical to their origins. `DEVICES.md` numeric
  measurements remain UNVERIFIABLE-NEEDS-HOST-RUN (host readings; the target is
  a 3060 elsewhere, so the 1070 figures are dev reference — not chasing, per
  Patrick's no-over-polish). **The docs are now trustworthy to build against.**

## Open threads (pending Patrick's input)

- **Tick spec (2026-09-13, updated):** the **next-tick setup IS the amalgamation
  itself** (Patrick, 2026-09-13) — not a separate step. End-of-tick amalgamation
  mints each field's virtual particle (force/mass centroid + effective mass),
  and those centroids **feed m2 in the next tick**; the loop closes on its own
  tail. The tick's operation set is still being added to (primary kernel and
  amalgamation are settled; more CPU-resident functions are coming) — the
  earlier literal "2 things to add" is descriptive, NOT a fixed count to
  preserve. Capture new operations as given; do not enshrine a number.
- ~~Confirm the mapping: predicates/fields/forces ↔ Parent/Sibling~~
  **RESOLVED 2026-09-13** (parents=predicates; every listing=field; mass=force
  — see DB structure subsection).

- **The remaining formula pieces for the primary kernel.** The physics is
  specified (interaction `m1·m2/d²`, participation weighting, superposition,
  move, momentum carry). What is NOT pinned is the **exact algebraic form** of
  (a) the exponential overshoot-attenuation and (b) the d=0 gate, plus
  confirmation of the momentum-carry law. I can draft these for Patrick to
  correct rather than block on them; do not invent silently.
- **How the math reaches the GPU is NOT a pending input — CORRECTED 2026-09-13.**
  The TaiChi engine DOES the GPU dispatch; this harness is built *for* it. The
  arg/launch mechanics are documented (`api-reference/ir_builder_test.cpp`, the
  vetted seam) and building/naming/compiling/launching the kernel IR is harness
  translation work, not a Patrick input. Removed from the pending list.
- **The sphere is RESOLVED (Patrick, 2026-09-13): diameter 1, one particle
  across.** No parameterization and no per-sphere constants — a sphere is a
  position with a fixed unit diameter (radius 0.5). Coheres with the 1:1 units
  (1 force moves a 1-mass particle 1 space in 1 tick).
- REINSTATED (I had wrongly dropped this): the fast-vs-structural change
  contract IS central. Answered by Patrick: force *activation* is the live
  launch-argument surface (no restart); virtual-node promotion into the model
  is the SNode-reconstruction/restart surface. Confirm the exact set of forces
  and how activation is expressed as an argument.
