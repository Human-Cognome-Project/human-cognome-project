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
bidirectional motion gates and demand reporting, accepted in #150. The revision after
the origin-dependence finding subtracts a nearby supplied reference before
f32 conversion and integrates in local coordinates. Its precise local-f32
predicate is described below; it does not settle a broader semantic resolution
law or the open combined-destination question. [CPU evidence and reproduction](EFFECTIVE-MOTION.md)
include integration with accepted Stage D #149 (`867c310`) on both host paths.
#150 is accepted/merged at `755bcef`. The focused [#152](https://github.com/Human-Cognome-Project/human-cognome-project/issues/152)
revision removes duplicate particle sets: type defines calculation phase and
one native binary activity set belongs to each type. The revised CPU evidence
is separate from the accepted #150 baseline. GPU execution remains untested here.

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
Activity is one binary flag per calculable element, stored in native
pointer/bitmasked SNode sets: one particle set and one centroid set. Type defines
phase order: particle calculation, centroid calculation, then particle calculation
again on the following tick. There is no per-element current/next scheduling
state or promotion pass. Native phase traversal selects active elements before
numerical work reads their stored motion state. Numerical payloads and
memberships remain separately resident.

| Phase | Implemented calculation |
|---|---|
| Publication | Validate the supplied state and adjacency; seed inclusive centroids and masses; invalidate previous exclusions. |
| Begin | Form the focal origin from the compact supplied focus list. Whole-study invalidation activates all live particles; ordinary ticks do not scan loaded particles. Update motion history only for native active particles. |
| Force | Each candidate visits **all** connected fields, including unchanged ones, then its ledgered contacts. Combine contributions before deciding movement. |
| Motion | Traverse active particles; apply the existing integration law. Retain the activity bit while position/velocity changes, or consume it when the full state is fixed. Actual displacement activates centroid bits. |
| Contact propagation | After all particle calculations/consumption, moved particles activate supplied contact peers using native operations. These peers calculate at the following particle phase. |
| Flag | Use participating-mass movement ratios and retained collective contributions to flag groups before their placement. Multiple triggers combine with any-active. |
| Place | Reduce every member of each flagged group, including sleeping members, once. Publish its centroid and retain rounding residue; unchanged groups keep their published position. |
| Centroid propagation | Consume each selected centroid bit, then qualify particle activation from changed-centroid member inputs. The particle phase has already finished, so those bits survive to its following phase. |

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

[Patrick's type-phase clarification (#152)](https://github.com/Human-Cognome-Project/human-cognome-project/issues/152)
governs the simplification of accepted #150. Each calculable element has one
activity bit; type determines when native traversal polls it. Particle effects
activate centroids; centroid effects activate particles. The next particle phase
occurs on the following tick by this order, without an additional scheduling
policy. Motion/velocity history belongs to the selected particle's numerical
calculation, not a second activity mechanism.

The thin [NativeSet](../src/field/native_set.h) wrapper uses existing fork
operations, rather than implementing a host bitmask/list/queue:

- `IRBuilder::create_struct_for` supplies sparse traversal. The compiler emits
  its own list clear/generation tasks (`taichi/transforms/offload.cpp`).
- `SNodeOpType::activate` / `deactivate` emit native cell operations. Native
  pointer activation allocates an ancestor block before native bitmasked
  activation; explicit cell activation in this fork does not allocate its
  ancestors automatically. The runtime owns the pointer allocation and atomic
  OR/AND bit operations (`node_pointer.h`, `node_bitmasked.h`). The former
  dummy atomic marker write is removed; the i8 layout leaf carries no activity
  payload and is never written by this wrapper.
- Pointer capacity uses the fork's `bit::least_pot_bound` storage helper. A
  bounded unpadded 3-by-32 probe reported all 70 requested bits active but entered
  only 50 traversal bodies on this CPU fork. Padding the pointer level to four
  blocks makes all 70 entries traversable. Logical counts remain unchanged and
  padding stays inactive. This is a verified layout constraint/workaround, not
  a new model scale or a claim that arbitrary non-power-of-two layouts work.
- LLVM checks the live leaf bit before each struct-for body
  (`taichi/codegen/llvm/codegen_llvm.cpp`). Generated container lists are not
  an immutable snapshot of individual leaf bits. The CPU probe activates a
  peer inside a single-thread traversal and observes two bodies from one
  initial bit. HCP must order effects accordingly; extra particle banks are
  unnecessary for this model.

The particle set remains stable through clear, force and contact calculations.
Integration consumes a particle's bit only when its full position/velocity state
is fixed. A changing state retains the same bit for continued calculation.
No peer particle is activated while this calculation pass consumes bits.
The speed determiner updates continuing particles; a consumed fixed particle's
cached speed is still valid because its velocity is unchanged.

Supplied contacts require an explicit ordering detail. Their force calculation
still uses the pre-integration positions and existing source order. Only contact
**activity writes** move from individual integration bodies to a native
propagation pass after all particle calculations and consumption. All actually
moved particles already retain their own activity bit. The pass emits peer
touches only from their stored Moving state. A newly touched sleeping peer has
Moving=0, so even if its live bit is encountered in this propagation traversal,
it performs no force/integration calculation and emits no further touches.
The pass is omitted when there are no supplied contacts or stiffness is zero.
No queue, epoch, timestamp, bank or full-particle scan is added.

Centroid gating traverses the touched centroid set and consumes unqualified bits.
Qualified centroids place once, including sleeping numerical members. Propagation
consumes each remaining centroid bit before activating particles. There are no
centroid activations during this centroid phase and no further particle-bit
consumption until the next particle phase. Thus repeated touches collapse and
later-phase activation survives, without promotion or end-of-tick clearing.
Publication alone clears both type sets after seeding; ordinary ticks have no
blanket finish/consume pass.

Moving/PriorMoving and displacement serve centroid arithmetic and wake reporting
only within active work. A mover retains its bit until a later calculation fixes
its full state and clears Moving; a sleeping particle therefore has Moving=0.
Sleeping positions/masses remain available independently of control bits.
Whole-study publication/parameter/mode invalidation and radial-origin changes
still activate all necessary live particles. Stable ticks use a zero-length
invalidation range; focus visits only selected groups and exposed counts are
cached at publication.

The fork's CHI IR compiler memory-cache key is the kernel name, so tree-bound
kernel names still include the execution-layout SNode id. Control trees and
pointer high-water storage live until Program teardown. Native list generation,
mask checks, the contact propagation traversal and launches all cost work; named
demand counters do not measure every instruction. No hardware speed or logarithmic
bound is claimed. Dynamic LoD slopes and #151's optional viewport stay separate.

[Type-phase CPU evidence](../tests/type_phase/acceptance.json) covers native
phase entry, duplicate touches, live-bit behavior, consume/reactivate order,
carried activity and retained member values. Contact fixtures put the source
both below and above sleeping target indices, within/across pointer blocks,
and check delayed response without propagation through an unmoved peer.
Existing full-field/ripple/collective/local-frame tests and both workbench paths
remain required. Actual GPU behavior/performance is unexecuted here.

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
