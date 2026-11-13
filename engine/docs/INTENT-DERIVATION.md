# Spatial intent: comparison proposal for #324

Status: proposed, not an accepted numerical law or kernel correction. Authorized
by [#324 comment 6859051253](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324#issuecomment-6859051253).
Source baseline: main `647c64845f1314cea1cdc6142fd82a108d09bf56`; numerical code
unchanged from the inspected `4f5d273c`. This work is separate from #325's storage
inspection and the later Taichi-reuse audit. No production numerical code changes.

## Existing calculation and the seam to change

[Ordinary contributions](../src/field/numerical_kernels.cpp) use participating
mass `w = m_body * share`, inclusive field mass M, and centroid displacement d:
`F_j = G*w*M*d/|d|^3`. Singleton/no-complement, zero gravity and exact coincidence
retain their existing zero-response gates. Whole-body mass divides the combined
force once during integration. This is not evidence of a double-mass defect.

[Hierarchy contributions](../src/field/live_hierarchy.cpp) use local
`d = c_leaf - x_body`, or inherited `d = c_parent - c_child`, and
`w = m_body * exact_coverage_ratio`. Child mass cancels from the distributed
child-to-parent force; it must not divide the result a second time. Covering
ancestor edges, local rows and canonical contacts accumulate before integration.
All these coordinates already use the common operative frame; storage depth adds
no scale multiplier. See [the phase contract](LIVE-HIERARCHY.md).

Current integration forms `v_trial = v_old + dt*F_total/m_body`, multiplies all
components by `exp(-(|v_trial|*dt/|F_total|)^4)` when force is nonzero, then applies
the separate outward-only dampener relative to the focal origin. It publishes
position once. The wide profile retains accepted displacement/dt for dt>0 and
incoming velocity at dt=0, under [accepted motion](ACCEPTED-MOTION.md).
The force-magnitude reach is the historical implementation, **not a justified
spatial distance**. This proposal supersedes its interpretation as a destination,
but does not yet replace its executable behavior.

## Candidate normalization, units and limitations

For each eligible attractive, noncoincident contribution let
`k_j = G*w_j*M_j > 0`, `r_j = |d_j|`, `alpha_j = k_j/r_j^3`, and
`t_j = x + d_j`. Use offsets around x rather than summing large absolute targets.
Under conventional force dimensions, k has units force*length^2, alpha has units
force/length, and F has units force. Coordinates/time may be model units, but
force and displacement must still have distinct roles.

| Candidate | Intent displacement I-x | Consequence |
|---|---|---|
| Coefficient weighting | sum(alpha*d)/sum(alpha) | F_attraction = sum(alpha)*(I-x); aligned with force |
| Force-magnitude weighting | sum((k/r^2)*d)/sum(k/r^2) | Spatial centroid, generally not aligned with net force |
| Coupling weighting | sum(k*d)/sum(k) | Spatial centroid, ignores distance in target weights |

All three return the single target, have length units and translate with the
frame. Units alone cannot choose among them. **Recommend coefficient weighting
as the leading candidate for further testing**, conditional on wanting a target
whose displacement explains the current force direction. The force factorization
supplies its denominator; `x+F` has none. This is an additional model criterion,
not a theorem that all notions of intent must satisfy it. Positive coefficients
give a convex target, with no dependence on unrelated body mass when w is held
fixed. Scaling every coupling equally leaves intent unchanged and scales force.

The target is instantaneous, not a solved equilibrium. Even fixed source centres
can yield a moving combined intent as the body's distances change. Persistent
intent is therefore an observation to measure, not something this rule guarantees.
Signed/repulsive field couplings are outside this candidate's positive domain;
do not silently substitute signed weights or classify them as absent.

### Independent worked cases

Positions below are offsets from x=0; k already includes participating mass.
These are algebraic contribution packets, not a validated native membership study.

| Inputs (d,k) | F | Coefficient intent | Magnitude intent | Coupling intent |
|---|---|---|---|---|
| (2,8) | 2 | 2 | 2 | 2 |
| (2,8), (-1,1) | 1 | 1/2 | 1 | 5/3 |
| ((2,0),8), ((0,1),1) | (2,1) | (1,1/2) | (4/3,1/3) | (16/9,1/9) |
| (1,1), (-1,1) | 0 | 0, valid | 0, valid | 0, valid |

In the angled case the 2D cross product of F and coefficient displacement is
zero; for magnitude weighting it is -2/3. Equal-force/different-intent is now
constructed for the proposed rule: the single packet (1,1) and single packet
(2,4) both yield force 1, but intent distances 1 and 2. This proves the algebraic
possibility, **not** an executed sleeping-recipient fixture.

For (2,8), holding participation/geometry fixed and changing whole-body mass
from 2 to 8 leaves F=2 and intent=2 unchanged while acceleration changes from
1 to 1/4. A native fixture must hold participating mass fixed by updating the
participation ratio; keeping share fixed would change participation too.

## Inherited targets, contacts and identity

For an inherited contribution use virtual recipient target
`t = x_body + (c_parent-c_child)`. Using c_parent directly would change the
existing displacement and destroy the factorization. Example: body=10,
child=3, parent=5 gives target=12, not 5. Translating all three by A translates
target by A; changing the local frame anchor subtracts A from all positions
without changing displacement. Operative-scale conversion precedes this algebra.
This is a proposed transplantation of the existing force geometry, not proof
that the resulting target is a persistent material point.

Recommend excluding contact resistance from *field intent*: contact remains an
equal/opposite force in F_total and changes proposed motion, but has no supplied
attractive destination. Including it would need a separate contact-target law;
negating a centroid weight is not such a law. For the unequal case above, contact
force -3 changes total force from +1 to -2 while field intent remains +1/2.
Consequently field intent need not align with total force when contacts act.
Contact-only motion has no field intent, not a fabricated zero-distance target.

Use the existing canonical influence/contact enumeration. Duplicate source paths
must not add a second weight; distinct occurrences/ordinals remain distinct even
when vectors match. The independent packet fixture does not validate native
alias/dedup rules. Repeated-path and rebasing native fixtures remain required.

## Exact cases and the unresolved coincidence decision

* No eligible positive contribution: intent absent; do not infer a centre from
  contact or carried velocity. A destination brake has no target in this case.
* Noncoincident opposing contributions can cancel: coefficient sum positive,
  F=0, I=x is **valid coincident intent**, distinct from absence.
* An eligible source at exactly d=0: the native force is gated to zero, but
  alpha=k/r^3 is undefined. The reference explicitly reports unresolved
  coincidence. It neither divides by zero nor silently drops this source.
* Candidate choices needing review: define a limiting coincident target at x
  (which can conflict with remaining nonzero force), omit zero-response terms
  (which creates a discontinuity from the near-coincidence limit), or use a
  different target construction. None is selected here; no epsilon is proposed.
* Carried motion remains independent of intent availability. Cancellation is
  not permission to zero carried velocity or suppress a required calculation.
* dt=0 must preserve accepted position and incoming wide-profile velocity.
  An intent observation can remain meaningful at zero duration; it must not
  create movement or bypass changed-input invalidation.

Do not describe this candidate as fully specified or ready for kernel use until
coincidence and signed-domain policy are resolved.

## Brake comparisons, not a selected replacement

After intent construction, let R=|I-x|, e=(I-x)/R when R>0, and v be the
post-force proposal. Compare full travel `|v|*dt` with approach travel
`max(v dot e,0)*dt`, and distinguish crossing the target plane from passing
through a target neighbourhood. Plane crossing alone does not characterize
oblique overshoot. A component-only replacement would also change energy and
must earn its own evidence; it is not preselected by this proposal.

With x=(0,0), I=(1,0), v=(0,1), dt=1, full-speed exponential attenuation is
exp(-1), while the approach-travel comparison has ratio zero. The unbraked
endpoint is (0,1), distance sqrt(2) from intent: continued motion is not a
crossing of the intent plane. With v=(2,0), travel/reach=2 and the same exponent
gives exp(-16); even spatial reach can nearly stop a large step rather than
land at the target. Neither result establishes a useful settling law.

At valid R=0, radial direction and travel/R are undefined. Blindly applying
zero attenuation would erase tangential/carried motion; blindly bypassing
would introduce a separate policy. Compare those limits explicitly before
choosing a brake. Keep the focal-origin radial dampener separate and initially
disable it only in controlled reference comparisons to isolate brake effects.
Do not change its production law as an accidental part of this correction.

## Publication, activity and storage plan

Proposed intent belongs to the body's pre-integration position and the complete
source packets acquired for that particle phase, expressed in its operative
frame. Proposed endpoint follows force/brake/radial processing; accepted endpoint
follows Q. Intent must not be relabelled as the post-motion equilibrium. Later
centroid publication changes inputs for a subsequent particle phase, as today.
Inspection must distinguish evaluated, absent, unresolved and stale intent; old
coordinates alone cannot certify current validity for a sleeping body.

The hierarchy notification path currently compares before/after contributions
and a fresh complete force with the body's kForce baseline. Under the candidate,
same-force/different-intent can require a wake. Compare the actual complete new
input signature (including intent validity/normalizer or target as appropriate),
not only the changed row. Preserve native binary activity and pending-input
notifications; no scheduler, second activity set or automatic stability gate.
Mass, focus, structural refresh, local-frame changes and contact invalidation
need explicit coverage. Publication cannot conceal stale intent.

Before implementation, enumerate required per-body fields, scratch, baseline,
readback validity and candidate/retained copies in existing owner preparation and
capacity accounting. Reuse native arrays/reductions and traversal, with budgeted
inspection. Do not add an unaccounted side registry. Preserve cumulative Q,
accepted displacement-derived velocity and discarded tails; no residue carry.

## Evidence and next review boundary

[intent_reference.cpp](../tests/intent_reference.cpp) is a standalone C++17
long-double algebra comparison, independent of production helpers. It checks the
worked cases, force/target nonalignment, equal force with distinct intent, inertia,
inherited translation, proposed contact separation and simple brake ratios.
Its tolerance is only for reference arithmetic, not a model epsilon. dt=0 and
contact checks are algebraic checks, not native state-transition tests.

This slice deliberately stops at the authorized comparison proposal. It has no
native Taichi execution, many-tick trajectory, quantization, sleeping-recipient,
alias, allocation-bound or GPU evidence. No tests for unrelated storage PRs are
rerun or counted as numerical validation.

Next: review the normalization criterion, exact coincidence and contact policy;
then implement candidate diagnostics through native owners and run independent
CPU single/unequal/cancelling, inertia, inherited/repeated-path/rebase and sleeping
invalidation fixtures. Compare approach/crossing/tangential many-tick trajectories
at several dt and accepted Q settings before selecting the production brake.
Report intent drift, accepted position/displacement/velocity and intent separation
in one frame, alongside focal damping and quantization effects. Distinguish
bounded motion, spirals and quantization stops; no universal orbital-stability
claim or classifier follows from persistent intent alone. #324 remains open.
