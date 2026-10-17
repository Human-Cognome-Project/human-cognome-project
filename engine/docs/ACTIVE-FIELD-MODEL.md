# Active field model and tick dependencies

**Status (2026-10-17):** The native object-support corrections approved in
[#124](https://github.com/Human-Cognome-Project/human-cognome-project/pull/124)
and [#125](https://github.com/Human-Cognome-Project/human-cognome-project/pull/125)
are implemented. See [OBJECT-SUPPORT-RESULTS.md](OBJECT-SUPPORT-RESULTS.md)
for executed checks and settings. The recovered
[operational plan](OPERATIONAL-PLAN.md) and [drift audit](DRIFT-AUDIT.md)
retain historical mechanics. `field::Harness` owns field arrays and ticks;
`assembly::FixedStudy` supplies a bounded native preparation/publication path.
The #91 follow-up implements selective ticks, a supplied focal-field origin,
bidirectional motion gates and demand reporting for review. Its precise f32
predicate is described below; it does not settle a broader semantic resolution
law or the open combined-destination question. [CPU evidence and reproduction](EFFECTIVE-MOTION.md)
are separate from Stage D #149 and from unexecuted local GPU validation.

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
integration is separately under review as Stage D #149.

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

## Selective tick and the effective-motion predicate

The default `TickMode::Selective` path uses the following order. The
`AllLoaded` reference mode evaluates all live particles and recomputes all
positive-mass groups with the same numerical laws and reduction order.

| Phase | Implemented calculation |
|---|---|
| Publication | Validate the supplied state and adjacency; seed inclusive centroids and masses; invalidate previous exclusions. |
| Begin/touch | Form the focal origin. Consider continuing state, changed inputs, changed field-relative vectors and moved contact endpoints. These select candidates, not automatic wakes. |
| Force | Each candidate visits **all** connected fields, including unchanged ones, then its ledgered contacts. Combine contributions before deciding movement. |
| Motion | Apply whole-mass integration, carried velocity, the existing brake and focal radial term. A positional wake requires an actual change in the final stored f32 position. |
| Flag | Use participating-mass movement ratios and retained collective contributions to flag groups before their placement. Multiple triggers combine with any-active. |
| Place | Reduce every member of each flagged group, including sleeping members, once. Publish its centroid and retain rounding residue; unchanged groups keep their published position. |

Field and activity kernels explicitly disable fast-math reassociation, which
could otherwise algebraically erase an intended rounding residue. Here `fl`
denotes the actual backend f32 operations, without an added epsilon.
For a candidate particle the calculation produces `(p_next, v_next)` from the
combined force and previous state. It moves iff any component of
`p_next != p_old`. `particles_woken` counts a positional movement following a
tick without movement, including initial movement. A candidate with no movement
is rejected by this positional gate even if its velocity changes. Such a
velocity change remains stored and keeps the particle eligible on the next
tick. Continuing exclusion requires the **full** position/velocity state to be
fixed under unchanged inputs; the speed determiner is not a sleep rule.

A published centroid change first compares `fl(C_new - p)` with
`fl(C_old - p)`. Identical vectors justify reusing that interaction's input.
Otherwise the particle becomes a candidate and evaluates all its fields before
the final positional decision. Several individually small field contributions
are therefore not independently discarded. A changed focal origin makes all
live particles candidates for the origin-dependent calculation. Contact
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

The radial term uses `r = p - O` in its direction and `|r|²` reach calculation.
This changes integration, not just rendering. Other field/contact forces are
already relative to their endpoints. Empty selection explicitly retains the
legacy fixed origin zero. Stored operative positions set the f32 positional
resolution; no moving-frame velocity transformation, automatic focus selection,
relational dimension/granularity inference or analyst is implemented here.
The f32 predicate is the bounded proposal under review for #91, not an approved
general semantic-resolution formula.

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
work, but current touch and centroid-gate scans still visit every loaded edge
each tick. The
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
Today `tick()` synchronizes and reads 14 i64 counters plus eight f32 focal
control values (144 bytes); `Demand` reports these counts, phase mask/adjacency
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
