# Active field model and tick dependencies

**Status (2026-10-19):** The native object-support corrections approved in
[#124](https://github.com/Human-Cognome-Project/human-cognome-project/pull/124)
and [#125](https://github.com/Human-Cognome-Project/human-cognome-project/pull/125)
are implemented. See [OBJECT-SUPPORT-RESULTS.md](OBJECT-SUPPORT-RESULTS.md)
for executed checks and settings. The recovered
[operational plan](OPERATIONAL-PLAN.md) and [drift audit](DRIFT-AUDIT.md)
retain historical mechanics. `field::Harness` owns field arrays and ticks;
`assembly::FixedStudy` supplies a bounded native preparation/publication path.
The #91 follow-up implements selective ticks, a supplied focal-field origin,
bidirectional motion gates and demand reporting, accepted/merged in #150
at `755bcefb`. The revision after
the origin-dependence finding subtracts a nearby supplied reference before
f32 conversion and integrates in local coordinates. Its precise local-f32
predicate is described below; it does not settle a broader semantic resolution
law or the open combined-destination question. [CPU evidence and reproduction](EFFECTIVE-MOTION.md)
include integration with accepted Stage D #149 (`867c310`) on both host paths.
Native binary scheduling now uses Taichi SNode activation and active traversal.
Local GPU validation remains unexecuted; combined CPU/PostgreSQL evidence was
accepted on 2026-10-19, clearing that review prerequisite for bounded local tests.

**Type-defined activity phases (Patrick, 2026-10-19):** element type determines
when it calculates; one binary activity flag selects work at that phase's entry.
Particles activate centroids for the centroid phase; centroids activate particles
for the following particle phase. “This/next tick” describes that order, not an
independent scheduling policy. #150's duplicate current/next particle sets are
built implementation details, not a contract. [#152](https://github.com/Human-Cognome-Project/human-cognome-project/issues/152)
requests their simplification using native traversal/consumption, with concrete
verification of carried-motion, contact and parallel activation dependencies.

The phase table and separate sets described below document the current #150
implementation. They do not supersede the type-defined contract or assert that
#152's simplification is already built.

## Fixed-study SNode scale mapping

Preparation binds a designated study root and its native SNode children to
explicit roles. The root uses unit scale; an expressed primary step multiplies
spatial units by 64. Secondary compression and naming equivalence inherit 1:1,
including nested compression. Raw depth, axis size and schema id have no
scale policy. Bindings follow actual SNode child relationships.

The caller owns the layout and keeps it alive for preparation and supported
refreshes. Host records retain these bindings and occurrence-to-slot mappings;
hot kernels use the existing ndarrays and cached operative values. This is a
fixed-study bridge, not general shared-subtree storage, zoom/recomposition,
or the complete cache manager/analyst. Positions, velocities and radii convert
together; inclusive centroids are seeded from those converted positions.
Time units, mass and identity are unchanged. Equivalent trajectories across
focus changes are not claimed.

## Prepared extent and ordinary body contact

The definition supplies an object centroid and component spheres. Preparation
computes `max(distance(component centre, centroid) + component radius)`,
then converts the radius into the operative frame, rounding outward to f32.
Finite coordinates and non-negative radii are required. Mass is independent
of radius. Preparation repeats on publication; ticks reuse prepared geometry.

This helper encloses a supplied layout. Packing, a movable minimum-sphere
centre and exact single/double/triple/quadruple sizes remain open. Fixture
coordinates in the results are explicit experimental inputs.

The contact payload contains only two endpoint slots for each explicit pair.
At nonzero centre separation `d`, penetration resistance has magnitude
`contact_stiffness * max(r_a + r_b - d, 0)` with equal and opposite forces.
There is no corona spacing, alignment regime, defined-bond hold or attraction
above contact. Field participation supplies attraction. Contact is finite
resistance, not a hard non-overlap guarantee. At exact coincidence the
direction gate produces finite zero force and does not separate the centres.
Contact discovery remains caller-supplied; there is no all-pairs pass.

## Whole-particle response and object records

Every edge carries `(particle slot, group slot, share)`. Force and centroid
accumulation both use the particle centre. The participating mass is total
mass times share; total particle mass absorbs the sum of forces. The inclusive
centroid, inverse-square law and exact coincidence/single-member gates remain.
No internal application-offset or rotational payload remains.

Preparation keeps reusable definition identity, distinct occurrence identity,
slot, parent value, ordinal and exact structural counts on the host. A
parent-value field spans ordinal positions, with a separate edge for every
repeated parent occurrence. Exact counts produce the hot float shares; those
shares are not archival count authority. Same-definition instances also share
an automatically prepared whole-mass sibling field using the same law.

**Stage C accepted in #146 (2026-10-17, `43f418a`):** Explicit membership
requests add a distinct membership field kind through that same edge shape and
kernel. Each active occurrence contributes its whole mass once per target.
[Contextual preparation](MEMBERSHIP-FIELDS.md) follows pinned upward memberships
of the owning letter, deduplicating paths and inheriting those groups on exposed
byte occurrences. Parent ordinals/counts remain separate. NULL group naming
masses remain metadata, without creating group particles or changing source
memberships. Native CPU seeded-response checks pass; the bounded warm-to-hot
integration is accepted/merged as Stage D #149 and included in this revision.

Exposure selections choose an aggregate or its declared parts, preserving
exact total mass and excluding the other representation. This bounded path
uses disjoint selections and preassigned distinct slots. It does not establish
#123's unresolved general recomposition contract.

## Publication and inert slots

`FixedStudy` synchronizes prior work, prepares and validates a complete
target, stages/uploads it, seeds inclusive centroids and initial determiner
state, and then marks the field ready. Supported refreshes preserve array
counts and restage the supplied target state; they do not automatically carry
forward old moving state. Failed publication records its exception and leaves
ticks disabled until explicit successful republication. Multi-array transfer
is not atomic and there is no rollback protocol.

Direct field callers can use `publish(parameters)` or `upload()` followed by
`seed_centroids(parameters)`; upload alone cannot tick. Numerical staging and
interaction indices are checked before transfer. Unused slots have mass zero,
zero state and no field/contact reference. Particle-mass and all-free
total-mass divisions are guarded. Empty arrays have size-safe storage while
logical loop counts remain zero.

The dormant universal-attraction kernel and launcher were removed. Total
mass is counted once per particle at publication and reused for radial damping.
Group mass is also cached until republication. Integration, its brake and
ordinary contact retain their prior laws; the radial term now uses the supplied
focal origin rather than always measuring from coordinate zero.

## Current selective tick and the effective-motion predicate

The default `TickMode::Selective` path uses the following order. The
`AllLoaded` reference mode evaluates all live particles and recomputes all
positive-mass groups with the same numerical laws and reduction order.
Scheduling uses native pointer/bitmasked SNode sets for current particles,
next-tick particles and current centroids. Numerical payloads and memberships
remain separately resident. Movement history is updated only inside native
active traversal; it supplies centroid displacement and wake diagnostics.

| Phase | Implemented calculation |
|---|---|
| Publication | Validate the supplied state and adjacency; seed inclusive centroids and masses; invalidate previous exclusions. |
| Begin | Promote the previous next-particle set and consume its bits. Form the focal origin from the compact supplied focus list. Whole-study invalidation activates all live particles; ordinary ticks do not scan loaded particles. |
| Force | Each candidate visits **all** connected fields, including unchanged ones, then its ledgered contacts. Combine contributions before deciding movement. |
| Motion | Traverse active particles; apply whole-mass integration, carried velocity, the existing brake and focal radial term. Changed position/velocity activates continuation for next tick. Actual displacement touches native centroid bits and contact neighbours. |
| Flag | Use participating-mass movement ratios and retained collective contributions to flag groups before their placement. Multiple triggers combine with any-active. |
| Place | Reduce every member of each flagged group, including sleeping members, once. Publish its centroid and retain rounding residue; unchanged groups keep their published position. |
| Propagate/finish | Changed native centroids visit their members and qualify next-tick particle activation. Clear current particle/centroid sets only; preserve the next set. |

In the formulas below, particle and centroid positions are **local operative
coordinates**, not supplied absolute placements. Field and activity kernels
explicitly disable fast-math reassociation, which
could otherwise algebraically erase an intended rounding residue. Here `fl`
denotes the actual backend f32 operations, without an added epsilon.
For a candidate particle the calculation produces `(p_next, v_next)` from the
combined force and previous state. It moves iff any component of
`p_next != p_old` in that local frame. Arbitrary translation of the supplied
study must not erase this displacement or stop continued evaluation.
`particles_woken` counts a positional movement following a
tick without movement, including initial movement. A candidate with no movement
is rejected by this positional gate even if its velocity changes. Such a
velocity change remains stored and keeps the particle eligible on the next
tick. Continuing exclusion requires the **full** position/velocity state to be
fixed under unchanged inputs; the speed determiner is not a sleep rule.

A published centroid change first rejects exact zero-response cases (zero gravity
or group mass equal to this participation), then compares `fl(C_new - p)` with
`fl(C_old - p)`. Identical vectors justify reusing that interaction's input.
Otherwise the particle becomes a candidate and evaluates all its fields before
the final positional decision. Several individually small field contributions
are therefore not independently discarded. A changed focal origin makes all
live particles candidates when the radial term is enabled. Contact
dependencies are reconsidered after either endpoint moves.

For edge `e` in group `g`, let `h_e = fl(m_particle * share_e)`, cached group
mass `M_g`, actual integrated displacement `delta_p = fl(p_next - p_old)`, and
published centroid `C_g`. A moved member flags the group if any component of

```text
fl(C_g + fl(h_e * delta_p / M_g)) != C_g
```

It also adds its weighted displacement to retained `R_g`, in source-edge order.
The collective test `fl(C_g + fl(R_g / M_g)) != C_g` can flag the group even
when none of its individual contributions does. `R_g` survives unflagged ticks.
Any individual qualifying interaction keeps the flag set even if contributions
cancel. This is a would-move Boolean, not a preliminary full centroid placement.

Each flagged group then reduces weighted member offsets around its previous
centroid: `S_g = sum fl(h_e * fl(p_member - C_g))`. Publication is
`C_next = f32(f64(C_g) + f64(S_g) / f64(M_g))` for positive mass; exact zero
mass publishes zero. The old additive `1e-20` denominator floor is replaced by
an exact empty-mass guard so finite small masses retain an ordinary centroid.
The remaining weighted residue is
`R_g = f32(f64(S_g) - (f64(C_next) - f64(C_g)) * f64(M_g))`.
Only final placement/residue arithmetic is widened; stored positions and the
would-move predicates remain f32. Seeding reduces around a member
position and retains its rounding residue as well. This preserves collective
small changes that a sum of large absolute coordinates can erase. Group mass
is recomputed only at publication. Stable source ordering and one writer per
particle/group remove float-atomic scheduling order from these reductions.

Group position publication only schedules a candidate check on the next tick;
it never declares every participant awake. The bounded CPU ripple fixture
reaches successive linked particles over four ticks and stops at a massive
participant with no positional response. This is fixture evidence, not a
general bound on settling or wake latency. Structural/mass/share/exposure,
radius/contact and supplied state changes require republication. Parameter,
mode and focal-selection changes invalidate exclusions directly.

## Supplied focal fields and mathematical origin

`StudyInput::focal_fields` explicitly names prepared `(GroupKind, value)` keys;
raw harness callers provide group slots. Missing, duplicate and mass-zero
fields are refused. Selected fields form the ordinary mass-weighted centroid
`O = sum(M_g * C_g) / sum(M_g)`, computed as offsets around the preceding origin
in stable group order. Field masses are their existing inclusive participating
masses; overlapping selected fields retain those supplied weights.

Preparation chooses a fixed numerical reference `A`: the supplied operative
position of the first participating member of the first selected field. This
choice is storage arithmetic, not the mathematical focal centroid. It subtracts
`A` in double **before** narrowing active positions to f32:

```text
q        = f32(supplied_position * operative_units - A)
O_local  = ordinary_centroid(selected fields' local centroids)
r        = q - O_local
q_next   = f32(q + integrated_velocity * dt)
```

Integration, member displacement, centroid publication and geometry checks all
use these local coordinates. The radial direction/reach uses `r`. The fixed
translation leaves operative velocities unchanged. Raw harness callers that
select focus after seeding are synchronized, translated around a participating
member, and reseeded with their carried state intact. Focus selection is a
configuration operation, not an automatic per-tick policy.

`PreparedStudy`, `Harness` and the owner-scoped `ActiveReport` retain `A` as
double `frame_anchor` metadata alongside local numerical positions. Supplied
coordinates can be interpreted as `A + double(q)`; no conversion back to a
large absolute f32 value enters motion or wake decisions. `Demand::focal_origin`
is local and accompanies its `frame_anchor`. Empty focus retains supplied
origin zero, represented locally as `-A`, without discarding local state.
BulkCopy and CpuDirect use the same checked payload bindings for activity and
physics; control buffers remain resident arrays on the chosen backend.

Independent CPU expectations exercise offsets zero and `2^27`, where adding a
unit to an absolute f32 position would fail. Unit velocity/dt produces local
positions 1, 2, 3, 4, continued evaluation and centroid propagation on both
paths. Native letter preparation also preserves a supplied initial unit
separation at that offset. These supplement the shared-law AllLoaded oracle.
They do not establish an unbounded local numeric range or a global trajectory
error bound.

## Focus-scoped LoD: intended versus built

Patrick's [2026-10-18 notes](https://github.com/Human-Cognome-Project/human-cognome-project/issues/91#issuecomment-6251014344)
give focus two roles: selecting the expressed three-dimensional neighbourhood
of the underlying n-dimensional relationships, and providing its ordinary
centroid origin. The finest LoD required by a study sets its floor; surrounding
structure is intended to roll up through compatible dimensions/category scope,
available capacity and source-data completeness. These are relational scopes,
not Euclidean rings in a universal absolute 3D map. Particle, field and edge
demand remain separate, including when whole constructs replace dense internal
constituents. This is a representation rationale, not a measured hardware limit.

Built here are supplied fixed-study scale roles, explicit exposure/focal keys,
local coordinate preparation, native activity and bounded workbench execution.
Dynamic view/LoD selection, step sloping/transitions, initial test granularities
and analyst policy remain pending discussion. No fixed ring boundary, relevance
heuristic, universal transition formula or storage-depth scale rule is inferred
from the notes. Existing primary-step and 1:1 compression distinctions remain.

## Native binary activity

The [2026-10-18 clarification](https://github.com/Human-Cognome-Project/human-cognome-project/issues/91#issuecomment-6251177820)
uses one binary scheduling decision per particle/centroid. The implementation
in [native_set.h](../src/field/native_set.h) attaches pointer blocks with native
bitmasked cells. Repeated atomic touches collapse to one active entry.
Particle force/clear/contact/integration/determiner loops use native struct-for;
centroid gate/reduction/publication loops likewise traverse native active groups.
Numerical eligibility checks occur inside that traversal; the group gate
consumes unqualified bits before placement/publication. No Candidate or
Continued array is used for scheduling.

Source inspection of the pinned `d191c5ce` tree establishes the mechanism:

- `IRBuilder::create_struct_for` iterates SNode containers. LLVM code generation
  guards bitmasked loop bodies with the native `is_active` result before
  entering the body (`taichi/codegen/llvm/codegen_llvm.cpp`).
- `Bitmasked_activate` uses atomic OR; `Bitmasked_deactivate` uses atomic AND
  (`taichi/runtime/llvm/runtime_module/node_bitmasked.h`). Clearing a bit does
  not clear that cell's payload. This fact does not generalize to pointer or
  dynamic deactivation. The scheduler leaves pointer parents allocated and
  stores persistent numerical/membership values separately in any case.
- Native list generation, pointer high-water allocation and mask checks still
  cost work. Zero inactive calculation-body entries do not prove zero metadata
  traversal, a logarithmic bound or universal hardware speed superiority.

The fork's CHI IR compiler cache uses the kernel name as its memory-cache key
(`taichi/compilation_manager/kernel_compilation_manager.cpp`); these kernels
embed a particular control tree. Their names therefore include the execution
layout's SNode id. Distinct studies compile independently rather than silently
reusing code bound to the first tree. CHI IR is not restored from disk cache.
The Program owns these control trees until runtime teardown; this bounded
implementation does not claim per-study tree reclamation or stable compilation
cost across new layouts. Runtime warmup is separate from steady-tick work.

Current particles, next particles and current centroids use separate masks.
At begin, native next entries activate current and are consumed; no numerical
phase produces new next entries until that pass completes. Integration touches
current centroids and next-particle continuation/contact bits. Placement touches
next particles only. Finish consumes current particle/centroid bits and never
next bits. Ordered kernel phases and tick completion synchronization protect
this handoff. Multiple touches cannot cause repeated integration or placement
within a phase.

Publication reseeds all logical slots once. First tick, parameter/mode changes
and republished inputs activate every live particle. A changed focal origin also
invalidates all live particles when radial integration uses it. Stable ticks
use a zero-length invalidation range. Focus calculation visits only explicitly
selected focal groups. Exposed-field counts are cached during host publication.

Moving/PriorMoving and displacement values serve the numerical centroid gate
and wake reporting. Begin resets them only for active particles. A mover always
schedules its own next calculation until its full state is fixed, so a sleeping
particle necessarily retains Moving=0. No inactive pass is needed to maintain
that invariant. Sleeping member positions/masses remain available to full-member
centroid reduction, independently of scheduling-bit consumption.

[Native acceptance](../tests/native_activity_test.cpp) executes inactive
traversal, 4,096 parallel duplicate touches collapsing to two entries, separate
current/next consume/reactivate behavior and retained numerical values.
Combined activity/workbench cases check ripple latency, one placement, carried
motion, invalidation, sleeping-member contributions and both host paths.
Dynamic LoD transitions remain a separate discussion.

## Combined destination and the discretization brake

Each field contributes a target relative to its centroid and a directed
inverse-square pull. The directed forces sum to `F_net`; the particle also
needs an **effective destination and remaining distance** to compare with
proposed travel. The September 14 implementation treated the resultant as
the destination displacement, so the current brake uses `reach = |F_net|`:

```text
proposed_velocity = carried_velocity + dt * F_net / whole_particle_mass
proposed_travel   = |proposed_velocity| * dt
multiplier        = exp(-(proposed_travel / reach)^4)  if reach > 0
multiplier        = 1                                   if reach = 0
```

The brake reduces proposed velocity near/over the assumed target distance
to counter discrete-tick overshoot and kinetic shearing; successive ticks
can continue settling. It is a numerical correction, not another field.
The separate radial dampener and contact pass have their own roles. At a
travel/reach ratio of `0.8`, the present multiplier is about `0.66`: it
slows approach before overshoot. Increasing the exponent sharpens the
transition but leaves the factor at ratio `1` equal to `1/e`.

**Open again after the September 25 walkthrough:** it has not been shown
that `|F_net|` equals distance to the intended *combined* endpoint when
several radial targets contribute. Summing the directed vectors is the
expected pull, but whether the destination requires a sum, a mean, or
another normalization is unsettled. The weighted-target algebra in the
[working record](../../docs/napier-system-discussion.md) is an assistant
candidate **for comparison, not an approved rule**. Verify the target
construction and trajectories before retuning the exponential slope. Existing
tests check mechanical boundedness/near meeting; they do not prove exact
multi-field convergence.

## Scaling claim and measurement boundary

A fully explicit pair comparison among `n` entities costs `Θ(n²)`. The
runtime's centroid-edge machinery instead costs at least the number of
edges it actually visits. Selective force and placement avoid some numerical
work. Native activation traverses active particle/group bodies, while touched
groups still inspect their own membership adjacency and native metadata has
its own traversal cost. Publication and whole-study invalidation remain dense.
The
intended ledger, chosen LoD, and reversible active-set exclusions reduce
the surface of necessary interactions. `O(log N)` is the project's goal for
active thought, where **`N` means exposed fields across particles** here;
older engine notes also use `N` for the fixed device particle pool, a
different quantity. Visiting all `N` fields already costs `Ω(N)`.
Specify the bounded operation and the growth of active interactions and wake
traffic before claiming a logarithmic bound. Measure first-base settling,
warm/hot LoD depth, active-edge count, wake depth, numeric trajectories,
and data transfers across representative devices. A single tick is a small
part of a possibly long run; there is no fixed 4 ms target.

The intended harness controls which numerical results leave a GPU and when.
Today `tick()` synchronizes and reads 18 i64 counters plus eight f32 focal
control values (176 bytes); `Demand` reports these counts, phase mask/adjacency
visits and elapsed tick time including synchronization/reporting. Force
eligibility checks, actual numerical field evaluations, rejected positional
candidates and unique flagged/recomputed groups are distinct. Contacts count
endpoint calculations, potentially two per pair. See the
[counter definitions and evidence](EFFECTIVE-MOTION.md).
`download()` transfers the whole numerical arrays on demand. Selective payload
readback remains planned. The future analyst might sample nearly every
tick; a human browser display might refresh only every few hundred ticks.
Those cadences are independent. CPU functions/kernels can be parallelized or
combined where dependencies allow; reading last tick's centroids and
publishing updated ones at the tick boundary remain part of the model.
