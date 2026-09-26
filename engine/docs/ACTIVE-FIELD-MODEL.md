# Active field model and tick dependencies

**Status (2026-09-26):** Field-model explanation for the
[NAPIER system guide](../../docs/napier-system-guide.md). This note distinguishes
[`field.cpp`](../src/field/field.cpp) from the intended selective working
model. The recovered [operational plan](OPERATIONAL-PLAN.md) and
[drift audit](DRIFT-AUDIT.md) preserve earlier decisions; the
[September discussion](../../docs/napier-system-discussion.md) clarifies
the remaining calculations. `field::Harness` is the native field-array/tick
class, not the entire future analyst-facing harness.

## What is computed in the loaded model

At each exposed field edge, a participant of effective mass `m1` at its
participating position reads its group's **last published** centroid of mass
`m2` and position `c`. The group includes the subject itself. For nonzero
separation `d`, the directed attraction has magnitude proportional to
`m1*m2/d²` and points toward `c` (with the active field constant). An exact
coincidence and a group consisting solely of this participation produce zero
in the current code. Several fields can touch the same particle; the code
adds their directed force vectors into `F_net`.

The native edge representation stores `(particle slot, group slot, share,
offset)`: `m1 = particle_mass * share`, and the participating position is
`particle_position + offset`. The **whole** particle mass absorbs the summed
force in integration. Parent occurrences with the same base characteristic
should remain distinct operative contributions. In the intended model,
ordered parent positions behave like a straight line across the particle:
partial forces can reorient that line even when they cannot appreciably
translate the whole. This is rotary *alignment*, without carried angular
velocity or an automatic permanent spin. The current harness only translates
on offset-edge force; no orientation state or reorientation formula is built.

`token_id` identifies a base token; `particle_id` identifies an allocated
instance in the model. Every exposed instance of the same token should share
an automatic whole-mass sibling field using **this same** interaction law.
The current arrays have no per-particle token mapping or generated sibling
field. Study-rooted nested/compressed SNodes and the active fields they
expose are an assembly task for the future cache manager and analyst, not
code in `field::Harness`.

## Tick order and the proposed activity gate

| Phase | Built `field::Harness::tick()` | Intended selective path |
|---|---|---|
| Load | `upload()` stages arrays; `seed_centroids()` publishes inclusive group positions before first force tick. | Assemble a study-specific base; initialize mass/geometry and wake all exposed work needed to establish it. |
| Force | Clear accumulators; force per **every** loaded edge; separate contact pass. | Evaluate every active field of each unresolved particle, sum its forces, and use the bidirectional movement ratio at that interaction to flag any centroid it would effectively move. |
| Motion | Integrate every particle with carried velocity, brake and separate outward radial dampening; run determiner. | Apply combined motive force and brake to active particles; account also for parent-line reorientation before declaring a particle resolved. |
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
