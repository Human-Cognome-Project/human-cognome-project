# Active field model and tick dependencies

**Status (2026-10-10):** The native object-support corrections approved in
[#124](https://github.com/Human-Cognome-Project/human-cognome-project/pull/124)
and [#125](https://github.com/Human-Cognome-Project/human-cognome-project/pull/125)
are implemented. See [OBJECT-SUPPORT-RESULTS.md](OBJECT-SUPPORT-RESULTS.md)
for executed checks and settings. The recovered
[operational plan](OPERATIONAL-PLAN.md) and [drift audit](DRIFT-AUDIT.md)
retain historical mechanics. `field::Harness` owns field arrays and ticks;
`assembly::FixedStudy` supplies a bounded native preparation/publication path.

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
memberships. Native CPU seeded-response checks pass; full warm-to-hot integration
remains queued as D.

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
mass is still counted once per particle each tick for radial damping.
Integration, its brake and radial damping otherwise retain their prior
behavior; the focused checks did not require retuning.

## Tick order and the proposed activity gate

| Phase | Built `field::Harness::tick()` | Intended selective path |
|---|---|---|
| Load | Fixed-study publication prepares records, uploads arrays, seeds centroids and enables ticking. | Assemble a study-specific base; initialize mass/geometry and wake all exposed work needed to establish it. |
| Force | Clear accumulators; force per **every** loaded edge; separate contact pass. | Evaluate every active field of each unresolved particle, sum its forces, and use the bidirectional movement ratio at that interaction to flag any centroid it would effectively move. |
| Motion | Integrate every particle with carried velocity, brake and separate outward radial dampening; free slots are inert; run determiner. | Apply combined motive force and brake to active particles before declaring a particle resolved. |
| Publish | Reduce **every** loaded edge to group mass/weighted position; publish all centroids. | After motion, reduce positions **once** for each centroid flagged by any interaction; reuse unchanged group mass; publish new positions for the next tick and wake connected work as needed. |

The interaction only needs to decide **whether** a centroid is effectively
perturbed. It does not predict how far that centroid moves; end-of-tick
placement uses the updated positions of all its participating members. The
movement-expression ratio across masses can avoid a whole-aggregate wake for
an interaction absorbed by its much smaller participant (person/Earth is a
scale analogy). Multiple active interactions combine their flags with an
**any-active** rule; their actual positional effects can still compound or
cancel. If none flags the group, the published position is retained for now.
This effective stability may allow tiny flex and is not a proof of identical
pre/post positions.

Once a centroid is republished, its linked participants can be woken on a
following tick. Their other fields may in turn flag other centroids; the
result is a self-limiting ripple as expressions cease to be significant for
the active resolution. A particle that requires no motion can sleep until
something relevant touches it. A few ticks of wake latency are intended to
be acceptable in a long simulation, but the behaviour has not been bounded
or measured. A model fully resolved by its equations is intended to stop
moving without a separate forced-stop rule.

**Two distinct decisions:** the per-tick ratio gates whether to recompute and
propagate that centroid now; *continuing* to omit a particle or field needs
an exclusion predicate absolute for the current defined calculation, plus
tracked changes that invalidate it. Changed edges, masses, study exposure,
centroid dependencies, and parameters can wake work. A small measured effect
alone is not such a predicate. The resolution/significance rule, exact
particle-sleep predicate, and invalidation graph still need specification.
Group **mass** is stable while member identities, shares, and masses are
unchanged, even when positions change; the current clear/reduction pass
recomputes it on every tick.

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
runtime's loaded centroid-edge pass instead costs at least the number of
edges it actually visits; it currently visits all of them every tick. The
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
Today `tick()` does no download and `download()` synchronizes and transfers
the whole arrays on demand. The future analyst might sample nearly every
tick; a human browser display might refresh only every few hundred ticks.
Those cadences are independent. CPU functions/kernels can be parallelized or
combined where dependencies allow; reading last tick's centroids and
publishing updated ones at the tick boundary remain part of the model.
