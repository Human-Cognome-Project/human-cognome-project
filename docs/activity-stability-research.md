# Structural stability and awake/asleep selection

Status: **exploratory; mathematical predicate unresolved**, recorded 2026-11-10.
Owner: [#248](https://github.com/Human-Cognome-Project/human-cognome-project/issues/248).
Patrick requested a separate note for specialist input or development as engine
data becomes available. This is not a new PoC release gate or an implementation
instruction. Existing numerical and activity contracts remain in effect.

## Intended question

Patrick clarified the practical motivation: dense knowledge contains uncertain
identities and competing or unresolved relationships. Continued motion across
fields is therefore likely, even when the arrangement is useful. Human knowledge
is messy; exact stillness need not be the endpoint. The research question is how
to avoid treating all persistent motion as equally consequential unfinished work,
while preserving meaningful uncertainty and sensitivity to new evidence. This is
not a request to force agreement, erase ambiguity or assume literal stable orbits.


Can the existing hierarchical particle/centroid calculation distinguish continued
motion from meaningful reorganization, so stable relationships need less repeated
calculation even while their participants move?

Patrick described each centroid as participating as a particle at the next higher
expressed LoD. Particle-to-field calculations already use the field centroid,
mass and relative geometry; cumulative responses determine subsequent activity.
The intended improvement is a better local stability predicate using that existing
path, rather than a separate clustering search, second solver or new scheduler.

Activity selects the calculable surface; sleeping numerical state remains
resident and relevant to participating mass and centroid calculations. Decisions
made from tick N affect following native phases/ticks. No same-tick retry,
stacked solve or permanent duplicate simulation is proposed.

## Clarification: evaluate known motion when needed — 2026-11-10

Patrick clarified the intended meaning of passivation: if a particle's position
at the required time is fully determinable from its motion description, there is
no need to integrate every intervening tick merely to keep that position current.
A periodic description can be evaluated at the required times, retaining phase,
period and a time reference. The same principle includes nested motion: evaluate
the necessary chain of relative descriptions to obtain the requested state.

This settles the intended semantics, not the mathematics for recognizing suitable
trajectories. Sleeping here means omitting unnecessary repeated numerical solving
while retaining an evaluable temporal description. It does not mean freezing
position, dropping orbital phase, replacing motion by an average, or ceasing to
represent consequential effects. Continued motion and uncertain knowledge can
persist without demanding repeated reconstruction of a known trajectory.

When an interaction needs position or other required state, it obtains that state
at the relevant time. Changes that invalidate the description return affected
elements to active solving. How to establish validity, compose nested descriptions,
detect invalidation and account for interactions between observations remains
research. No periodic model, fit, tolerance, event-detection method or evaluator
has been chosen. Do not infer that deterministic dynamics alone supplies a cheap
closed-form trajectory.

Patrick requested that wake/sleep work be left for later. Preserve next-tick
decision semantics and the existing execution authority; this note changes no
runtime behavior and adds no work to the current allocation PRs.

## Origin and candidate ideas

A discussion with Google began with minimum observables for orbital stability,
then considered cluster virial ratios, phase-space covariance and angular-momentum
consistency. Patrick clarified the existing hierarchical centroid machinery and
cumulative activity propagation. The final suggested shortcut was:

- circular-speed scale: `v_c² = G M / r`;
- proposed local ratio: `R = |v_relative| / v_c`;
- proposed passivation near `R = 1`, or below `sqrt(2)`;
- squared comparison `|v_relative|² r ≈ G M` to avoid a square root.

These are recorded candidates, **not accepted sleep conditions**. Google's
additional claims about automatic rigid collapse, subtree pruning, exclusively
top-down perturbations and infinite scaling were not established project decisions.
Do not import them as requirements.

## Why the mathematics remains open

The circular-speed magnitude does not establish orbital geometry: a velocity
pointing directly toward the centroid can satisfy the same scalar equality as a
tangential circular velocity. Below escape speed establishes binding only under
the relevant isolated conservative two-body assumptions; it does not establish
fixed geometry, insignificant internal change or permission to stop evolution.

The virial theorem concerns an averaged force–position/energy relationship.
Instantaneous equality does not certify stability. HCP combines multiple fields,
contacts, braking, damping and configured publication precision; a single
particle–centroid Keplerian formula is not automatically its governing invariant.
A centroid's zero net force also does not exclude internal deformation from
differential forces.

The broader covariance and angular-momentum suggestions likewise need derivation
for this model. A plateau of one summary statistic alone has not been shown to
preserve the relationships relevant to HCP. No covariance determinant, energy
ratio or angular threshold has been selected.

References for specialist review:
[John Baez, virial theorem](https://math.ucr.edu/home/baez/virial.html) and
[University of Maryland, central forces](https://physics.umd.edu/hep/drew/orbits.html).
These explain ideal mechanics; they do not validate an HCP activity predicate.

## Questions to resolve

1. What observable represents relevant structural stability at each expressed
   level: relative geometry, change in combined response, bounded recurring
   dynamics, or another quantity? Define the local frame and treatment of bulk
   motion, damping, multiple fields and overlapping participation.
2. How can an evaluable motion description replace intervening integration?
   The intended semantics are clarified above. Specify phase/time references,
   requested state, nested composition and how field/contact effects remain
   represented. If an approximation is proposed, state its error contract.
3. What changes invalidate the stability decision? Preserve structural/mass/input
   changes, contacts, focus/rebase and consequential pending inputs. Do not assume
   every influence passes through a parent or that the membership graph is an
   exclusive tree.
4. Can the useful observables be collected during existing evaluations and
   reductions? Specify any retained history, persistence/hysteresis and precision
   dependence. A constant-size comparison does not make the entire update O(1);
   account for obtaining inputs and waking affected elements.
5. How does this compose with existing cumulative publication and activity
   consumption? Individual field stability must not discard collectively
   consequential contributions or suppress reevaluation after changed inputs.

## Evidence to develop

Use growing engine data and bounded fixtures to compare candidates with the
current activity policy and full-evaluation reference, with matching numerical
laws. Separate numerical defects from intentionally different coarse behavior.

Useful discriminating cases include translating structures with unchanged relative
geometry; radial approach versus tangential motion at the same speed; eccentric or
recurring motion; opposing forces that deform a group without moving its centroid;
multi-field cancellation; damping; insertion/contact perturbations; source/mass
changes; overlapping fields; and multilevel wake propagation. Include phase-sensitive queries at arbitrary
requested times, nested periodic descriptions, and perturbations or contacts
between queries; compare evaluated state and event recovery with the reference.

Record missed consequential changes, false wakes, retained observable relationships,
recovery after perturbation, active work and incremental cost. Derive thresholds
from declared resolution and measured behavior rather than choosing them here.
Any additional reference calculation is a separate validation experiment, not
another operational solve within the tick.

The desired deliverable is a bounded mathematical proposal with assumptions,
counterexamples, wake/sleep semantics and source-pinned evidence. Specialist input
in orbital mechanics, dynamical systems or numerical simulation is welcome;
empirical development can proceed as the PoC produces data. Implementation follows
a separately reviewed contract.

## Current contracts and related work

- [Activity and native phase mechanics](../engine/docs/ACTIVE-FIELD-MODEL.md):
  heed its current-profile map; legacy descriptions remain historical.
- [Accepted wide-path motion](../engine/docs/ACCEPTED-MOTION.md):
  published displacement and retained motion, with input invalidation preserved.
- [Cumulative publication](../engine/docs/CUMULATIVE-PUBLICATION.md).
- [Precision and hierarchy direction](precision-exposure-and-concept-catalogue.md):
  #172/#177 own related numerical questions. Regional fp32/fp64 selection and
  analyst overrides are a separate post-PoC follow-up, not a stability proof.
- #156 owns operation integration and #145 actual hardware qualification.

This note preserves an open design question. It does not change the current coding
agent's resource-accounting task or authorize speculative analyst behavior.
