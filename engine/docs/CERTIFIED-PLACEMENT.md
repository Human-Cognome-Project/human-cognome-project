# B2 certified native placement

Status: implementation and qualification in progress under accepted
[#199](../../docs/native-admission-contact-transaction-plan.md) and the #201
release. This candidate query claims no record and publishes no occurrence.
B3 source/hierarchy/admission publication remains separate. CPU/x64 only.

## Candidate boundary

`PlacementWork` and `PlacementCells` opt a prepared f64 owner into
`query_prepared_placement`. The proposal uses existing `UpdateCommand` structural
references plus intrinsic mass, radius, canonical source rank and an explicit
complete-geometry definition/revision stamp for each new body. There is no
request position, velocity, direction or numerical-start callback. Opaque geometry
stamps belong to the source adapter; the engine performs no database lookup.

The existing source preflight validates pending/current references, exact source
counts, ordered participation/contact ledgers, retirement closure and old-plus-new
claims without staging numerical repair or claiming a native record. Placement
borrows that prospective source view. It is not full B3 hierarchy/numerical
readiness validation, and a result cannot authorize admission on its own.

Private collection reuses the actual geometry collection builder. Current native
presence includes sleeping physical bodies; fields, unexpressed alternatives and
unused addresses are absent. Only explicitly retired bodies from the checked
source view are omitted. New bodies are processed in unique canonical rank order
and each certified stored point joins prospective occupancy for later bodies.
The target is the current combined native focus, frozen for this candidate batch;
an empty source uses local zero. B3 will supply its separate D1 bootstrap convention.

The query preserves numerical values, activity, packets/history, generations,
completed ticks, last Demand, timings and public geometry observations. Attempt
and published candidate outputs have separate reserved buffers. A failed attempt
cannot replace an earlier successful placement observation. Successful candidate
publication changes only the query token and candidate diagnostics. A source
generation/tick change makes the observation stale. Unexpected backend failure
makes the owner unready; no model rollback or host numerical replay is used.

## Exact stored-point validation

The solver applies the existing body significand mask before testing a proposal.
For every current/prospective occupant it proves

`distance_squared_lower >= expanded_radius_squared_upper`.

It additionally checks the existing native contact predicate so a point accepted
by geometric bounds is not reported as free while that predicate finds penetration.
Every axis, radial or neighboring representable proposal is validated against
all occupants. Genuine zero radii remain zero. Missing geometry is rejected.
Exactly representable tangencies are accepted without introducing clearance.

## Outward f64 arithmetic

`placement_math.h` emits strict native f64 operations and integer bit operations.
The fork has no assumed interval/transaction primitive. Knuth TwoSum obtains the
exact residual of addition/subtraction; Dekker splitting with `2^27+1` obtains the
exact residual of multiplication. A nonzero residual selects the adjacent outward
normal endpoint. Exact operations retain an exact singleton interval, including
zero and tangency. Sqrt is bracketed using the exact residual of the rounded
root's square; nonzero division is bracketed by adjacent outward values.

Error-free-transform operands are checked before arithmetic. Add operands are
zero or in absolute `[2^-768,2^768]`; product operands are zero or in
`[2^-384,2^384]`. Split products, low-part products and residual additions remain
normal or zero in these windows: the smallest low-part product is at least
`2^-872`, and the largest split value is at most about `2^411`. Sqrt inputs use
the add window; its roots fit the product window. Unsupported intermediate ranges,
nonfinite results or a nonzero quotient flushed to zero reject the attempt.
The backend FTZ mode is retained. Round-to-nearest is required, and the scalar
solver executes serial native code on the calling CPU thread.

Only the placement solver and its arithmetic probe use Taichi's supported
per-kernel `opt_level=0` / `cfg_optimization=false` configuration. Local IR
simplification and LLVM external optimization remain enabled. This avoids the
optional whole-kernel CSE/CFG cost for the large interval graph. Other kernels
retain their Program configuration. Strict arithmetic is separately checked in
the actual emitted modules and against exact integer results.

The current input profile permits coordinates/focus zero or absolute
`[2^-256,2^128]`, and radii zero or `[2^-128,2^128]`. These are proof windows,
not physical scale assumptions. Generated proposals and all interval operations
are checked against their own supported windows. Difficult geometry or arithmetic
can reject; no arbitrary finite-input convergence claim is made.

For normal exact result `x`, an adjacent outward endpoint has relative error at
most `e = 2u/(1-u)`, `u=2^-53`. The implementation uses a representable upper
bound on e. Three directed coordinate differences, their squares, two positive
sums and sqrt give at most five squared factors and 3.5 distance factors, bounded
by four. Thus a norm enclosure's width is at most

`((1+e)^4-(1-e)^4) D = 8e(1+e^2) D`.

For current feasible upper distance U and outward absolute-coordinate bound
`M = max_k(|F_k|+U)`, the position mask's binade step is
`h = 2^(floor(log2(M))-body_fraction_bits)`, or zero when M is zero.
Truncation changes a point by less than `sqrt(3)*h`. The reported tolerance is the
outward evaluation of

`tau = sqrt(3)*h + 8e(1+e^2)*U`.

It depends on the coordinate range, current feasible bound and actual publication
mask. It is independent of work/cell limits and cannot be enlarged on exhaustion.
Success requires an outward `U-L <= tau`. This tolerance is a certificate
resolution, never added to a physical radius or used to waive overlap.

## Global coverage

A validated finite stored point supplies U. The cube `[-U,U]^3` in coordinates
relative to F contains every point that could improve it. Adaptive binary
subdivision covers that cube without gaps. A box's interval norm supplies a
lower objective bound. A box is excluded only if its maximum possible distance
is strictly below one occupant's minimum expanded radius, or its objective lower
bound cannot improve U. The heap orders valid lower bounds with deterministic
coordinate ties. It is bounded numerical search storage, not a particle work queue.

Single-sphere triangle inequalities supply global lower bounds. Nonnegative
weighted exclusion inequalities supply another bound. With relative centres d_i,
weights w_i, `W=sum(w_i)`, `D=sum(w_i*d_i)` and
`B=sum(w_i*(R_i^2-|d_i|^2))`, every feasible point at distance t satisfies

`W*t^2 + 2*|D|*t >= B`.

For positive B, `B/(sqrt(|D|^2+W*B)+|D|)` is therefore a valid lower distance.
All components use outward intervals. Centre-segment weights improve degenerate
intersection-ring bounds; optional proposals and lower bounds never replace
cube coverage. Heap keys combine the global and box lower bounds. If the heap
is empty, its discarded regions cannot improve the feasible upper bound.

Named cell/work exhaustion returns failure even when a plausible feasible point
has already been found. No unchecked point or record claim is exposed. Final
qualification will record achieved work/cell peaks and supported profiles.

Work units conservatively reserve source validation, collection and fixed
cleanup before dispatch, then charge each obstacle test, proposal, box visit
and heap operation. They are bounded operation units, not a FLOP count. Source
preflight/compaction costs depend on prepared P/G/E/C capacities; numerical
exclusion tests depend on actual present/prospective occupancy. Potential V
does not determine a geometry scan or a numerical payload.

## Qualification in progress

The native interval probe currently passes 632 cases against independent
8,192-bit integer arithmetic for addition, subtraction, product, division,
sqrt and squared/unsquared norm, including exact tangency and wide exponent
ranges. End-to-end placement, independent sphere-boundary/intersection references,
cell/work/domain rejection, complete-state preservation, canonical batches,
resource reuse, compiler evidence and the final CPU/database regressions remain
in progress. This document is not an acceptance claim.
