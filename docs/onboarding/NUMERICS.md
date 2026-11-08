# numerics

Read before changing field/tick calculations, activity, focus or their tests. This is the accepted direction summary; follow its owning numerical contracts for exact predicates.

[Choose another path](README.md) · [Bounded work](AGENT-START.md)

## Latest model direction — 2026-10-23

The directions below have bounded CPU implementations through #201. Read
profile-specific statements with the [accepted motion rule](../../engine/docs/ACCEPTED-MOTION.md)
and current state above; they do not reopen completed prerequisites.

See [precision, exposure and concept catalogue](../../docs/precision-exposure-and-concept-catalogue.md).
#172 owns the requested math-core work: higher-precision combined calculation,
configurable significand trimming of stored positions, and discard of subthreshold
positional rounding residuals. Retain as much precision as viable; f64 calculation
may retain an f64 storage container with trimmed terminal bits. Do not default to
f64→f32 or any fixed per-LoD reduction ladder. This supersedes indefinite residual carry as intended policy for that
new path; the opt-in trial implementation is accepted through #187/#192/#195/#196.
Legacy arithmetic remains separately supported.
Do not confuse rounding residue with physical velocity or centroid mass accounting.
#115 records global/per-object exposure controls, separate from awake/sleep and
browser visibility; visual parity with analytical needs is unproven and these
controls do not add a GPU gate. #173 owns concept.db as the cross-media centroid
catalogue and recipe/download root, with schema/addresses still undecided.

#177 owns the new LoD-local centroid cascade: resolve finest-exposed local fields,
aggregate upward until the exposed study resolves to one centroid at its resulting
scale. Internal aggregation adds no distance scaling; scale factors belong only
to actual conceptual LoD transitions, with secondary compression/naming at 1:1.
The bounded production response and publication protocol are accepted in
#194/#195, with coherent refresh in #198. Broader #177/#172 scope remains open.

Resolution-cycle clarification (2026-10-24): trial native propagation across ticks.
Initialize all exposed centroids, then aggregate current published inputs in the
normal type phases; nested up/down ripples may take several ticks to settle.
Coherent settling and perturbation recovery are the gate, not a complete cascade
within every tick. Measure time/work to settle. #180's full upward candidate query
is not a required per-tick scheduler; one-tick lag remains a trial assumption.


## Latest cumulative-publication direction — 2026-10-26

The [cumulative significand clarification](../../docs/precision-exposure-and-concept-catalogue.md#cumulative-significand-clarification--2026-10-26)
governs the accepted numerical path under #172/#177. Accumulate the tick's
applicable contributions at calculation precision before the destination's
significand filter, compare the net with its stored position, and base
movement-triggered activity on that cumulative published change. Preserve
pending-input invalidation, physical velocity, mass and required history.
No discarded positional tail is carried between ticks or recovered from members.
Physical velocity follows the later accepted #196 displacement/dt rule; the
older general instruction to preserve untrimmed velocity does not override it.
Overshoot braking and radial damping are unchanged.

This supersedes the provisional any-untrimmed-attempt activity policy. The
[C2b-1 correction](../../engine/docs/CUMULATIVE-PUBLICATION.md) is accepted in #192;
C1 descriptions and archived evidence remain records of their pinned behavior.
C1 already combines centroid
inputs before publication; audit the actual gating seam rather than assuming
each member is independently trimmed by that reduction. Require cumulative
threshold-crossing, cancellation and multilevel no-lost-input tests. Integrate
with C2a's publication/consume contract without a same-tick global solve.


## Activity, focus and privacy

Use fork-native activation/deactivation and traversal before custom machinery.
One binary activity set per particle type selects work; type determines phase.
Particle calculation/consumption precedes contact activity propagation, then
centroid propagation activates particles for the following particle phase.
No particle banks, promotion, scheduling epochs/timestamps, custom queues or
loaded-array scheduler scans. Motion states belong inside active numerical work.

A changed centroid alone does not wake every dependent: test the combined
positional effect; a woken particle evaluates every connected field. Combine
centroid would-move flags with any-active and place each flagged centroid once,
including sleeping members. Preserve carried motion and changed-input invalidation.
For the accepted wide path, discard unpublished positional tails and retain
velocity according to the accepted #196 displacement/dt rule; do not restore
historical rounding-residue carry. Keep pre-integration contact forces and sleeping-target checks on both index sides
and across pointer blocks. Separate numerical force work from gate/adjacency and
native metadata costs. No arbitrary epsilon or zero-speed sleep rule is implied.
The [active field model](../../engine/docs/ACTIVE-FIELD-MODEL.md) owns exact predicates.

Selected focal fields define the origin by ordinary combined centroid calculation,
including the origin-dependent radial dampener. The finest required conceptual
LoD sets the floor; compatible category scope, capacity and data completeness
govern intended surrounding rollup. This is not a universal absolute 3D map or
hard Euclidean ring policy. Dynamic selection, slopes and transitions remain
unbuilt; [#91 notes](https://github.com/Human-Cognome-Project/human-cognome-project/issues/91#issuecomment-6251014344)
are not authority to invent analyst relevance rules.

Private/local information enters only at the analyst-owned active hot boundary.
The cache manager must reject private namespaces/handles **before source lookup**;
it must not read, filter, redact, publish, acquire or refresh private DB data.
Joint reports return to the active owner, never through manager reporting.
Public entity knowledge may remain shared. Use supplied private fixtures; actual
private DB access and analyst reasoning are not implemented. Earlier isolated
Stage A scoped-storage tests do not authorize a private manager path.
Warm publication, assembly completeness and execution readiness are separate.
No blanket change to temporary ownership or retained fixtures is authorized.

