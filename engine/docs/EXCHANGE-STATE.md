# Private outgoing state for representation exchange

This is the first D7 prerequisite after #213's provisionally accepted exact
rebasing. It derives an outgoing set's mass-weighted centre and linear velocity
from resident physical bodies in the current operative frame. The following
[private replacement staging](EXCHANGE-PLACEMENT.md) consumes it for placement
and velocity transfer; neither prerequisite claims/retires records or publishes
an exchange. The accepted [D7 contract](../../docs/core-operation-readiness-plan.md#decision-status)
still governs that following integration and the enduring occurrence owner.

`detail::Placement::stage_exchange_state` is private to engine/test access.
The caller provides an expected generation and nonempty set of full native body
handles. This seam validates context, address, record and incarnation against the
current source ledger, rejects duplicates and bounds the set by physical particle
capacity. Logical occurrence/recipe membership belongs to the future coordinator;
an arbitrary set cannot be presented as a validated logical exchange through a
public API. There is no public query token or reusable admission authority.

## Native arithmetic and limits

The host uploads only a structural selection mask. A prepared strict-f64 Taichi
kernel visits selected records in ascending physical-record order, including
sleepers. It sums mass and six mass-weighted current position/velocity components,
then divides each weighted sum by total mass. Caller order cannot change the sum
order. Unselected bodies, field centres and study focus do not contribute. The
result retains full f64 precision even when position storage uses fewer bits.
The double frame anchor is unchanged; all selected coordinates already share it.

Positive masses must be in `[2^-128, 2^128]`; nonzero coordinate/velocity
magnitudes must be in `[2^-256, 2^128]`. Zero coordinates/velocities are accepted.
Invalid, nonfinite and out-of-range values reject without live mutation. These
conservative operand limits keep products, sums and division normal or zero in
the fork's FTZ runtime: individual products are at least `2^-384` in magnitude,
their binary lattice is no finer than `2^-488`, and at most signed-i32 many
positive masses produce a total below `2^159`. A nonzero divided sum therefore
remains above `2^-647`; product sums remain below `2^287`. Cancellation is allowed.
The output range guard is `[2^-768, 2^288]` or zero.

This uses ordinary strict-f64 multiply/add/divide, not exact summation or an
interval certificate. With `n` selected bodies and unit roundoff `u=2^-53`, the
usual sequential-sum bound `gamma_n=n*u/(1-n*u)` applies to absolute weighted
sum error against the sum of absolute products, with multiplication and final
division rounding in addition. Cancellation does not promise a small relative
error. Final replacement mass/momentum tolerances remain the coordinator's
responsibility; this prerequisite does not qualify completed momentum transfer.

## Lifetime, work and preservation

The eight-double private output holds XYZ, velocity XYZ, total mass and status.
It is allocated and its kernel compiled/bound/warmed during preparation. The
existing placement selection array and host mask are reused; the manifest adds
the output/control backing and four prepared arguments. Work reservation is
`64*(P+1)` for host validation/mask handling and the bounded native reduction.
It is charged against PlacementWork before scanning or uploading selection.
No recurring allocation, record claims or reference peak is introduced.

Only scratch is written. Source state, current and historical numerical values,
activity, contacts, generation, ticks, frame reference and completed reports are
unchanged. Existing geometry/placement observations remain valid. Ordinary
rejection permits retry; unexpected execution failures mark the owner unready.
Scratch after any attempt has no independent validity and must not be consumed
outside an immediate successful future coordinator call under exclusive ownership.
The host reads diagnostic status but does not replay numerical outputs.

## Qualification

The `exchange_state` case is included in the combined placement suite and can
run separately. It covers an aggregate and an unequal-mass body set, three-axis
position/velocity values, full/reduced position profiles, sleepers and retained
dirty packets, input permutation, unrelated occupants, stale/duplicate/empty
selection, exact work boundaries, invalid masses/velocities, retry, unchanged
observations/state and stable prepared backing with no observed recurring C++
allocation. A nonbinary weighted velocity is compared with an independent
long-double calculation, including under reduced position precision.

The placement compiler audit adds the scratch-only reduction to its frozen
binding/owned-writer/strict-math checks, with omitted-output and contracted-math
negative controls. Results and untested scope are recorded in
[admission evidence](../tests/admission/README.md). CPU qualification does not
establish GPU execution; no backend-specific arithmetic path was introduced.
