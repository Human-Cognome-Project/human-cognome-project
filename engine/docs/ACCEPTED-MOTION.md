# Retained motion from accepted displacement

## Local maintainer decision — 2026-10-29

Patrick authorized truncating motion as well as position and delegated the
choice of rule during GTX 1070 qualification. Physics is an adjustable tool;
this is an intentional change to the wide f64 trial model. It supersedes the
older C1/C2b-1 requirement to retain untrimmed post-brake velocity. Historical
acceptance evidence remains evidence of its own source revision.

For each active body, combine all applicable forces and integrate/brake/damp
once as before. For each coordinate, with old published position `x`, trial
velocity `v_trial` and configured body quantizer `Q_b`:

```
x_next = Q_b(x + v_trial * dt)
displacement = x_next - x
v_retained = displacement / dt       when dt > 0
v_retained = v_old                   when dt == 0
```

Compute cached speed from the retained velocity before consuming activity.
Positive-duration visits with no accepted movement retain zero velocity. There
is no second motion precision, epsilon, positional accumulator or residual
momentum to recover. The existing body precision is adjustable. This discards
momentum below that publication resolution and can change subsequent motion;
it is not merely a storage optimization.

Activity still follows the published positional change. A zero-duration visit
preserves incoming velocity but does not stay active merely because velocity
is nonzero. Existing input/control invalidation and later contact/centroid
notifications continue to select subsequent work. Legacy f32 behavior is unchanged.

A transient post-brake/post-radial velocity may be subnormal before position
publication. NaN and infinity remain errors. Published position, retained
velocity and cached speed still must satisfy the existing normal-or-zero
contract. This does not authorize arbitrary subnormal positions, forces, masses
or mixed-precision conversion. A subnormal multiplied by a sufficiently large
timestep is still evaluated in the trial position; it is not blindly replaced
by zero before projection.

## Verification

`cumulative_publication_test --motion-only --backend cpu|cuda` checks both signs,
4/8/52 body fraction bits, zero/half/unit timesteps, exact retained speed and
activity. Normal starting inputs also produce an independently calculated
subnormal braking result, which must publish unchanged position and zero
retained motion without withdrawing readiness. The fixtures preserve the
ordinary no-allocation and fixed-backing checks.

The hierarchy physical reference bounds the integrated long-double positional
result through Q and separately verifies retained velocity exactly against the
actual accepted displacement. Existing combined-force, cancellation, input wake,
history, held-input convergence and resource checks remain required. Executed
results belong in the source-pinned qualification record; this document alone
does not assert that the GPU or release is qualified.
