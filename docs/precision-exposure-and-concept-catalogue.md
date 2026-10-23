# Precision, exposure and the concept catalogue

Status: **direction confirmed by Patrick, 2026-10-23; implementation and calibration pending**.
Owning issues: [precision #172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172),
[view scaling #115](https://github.com/Human-Cognome-Project/human-cognome-project/issues/115),
[concept catalogue #173](https://github.com/Human-Cognome-Project/human-cognome-project/issues/173).

## Calculation precision and stored movement

Pair calculation/storage precision with the expressed conceptual LoD. The focal
example is f64 combined field calculation with f32 stored positions: promote the
stored value exactly, calculate the combined effect, then round the resulting
position to storage precision. If the stored value does not change, discard the
subthreshold positional residual as noise. Do not accumulate that discarded
rounding detail into later ticks. Contributions combine before quantization;
individual field contributions are not independently filtered out.

Calculation precision and effective stored precision are independently adjustable;
the f64-to-f32 example is not a fixed pairing or a requirement to halve widths.
Patrick's “f30 versus f32” example means a finer-grained reduction of retained
precision to suppress positional jitter, rather than restricting the control to
native f16/f32/f64 choices. Such a setting could be represented by quantizing
within a wider native container; it does not imply native hardware f30 support
or an already selected custom floating-point format.

Any reduction of retained precision provides a quantization/noise-filter control.
Choose the reduction at each scale empirically for useful model physics and
recoverable relationships, not solely for computational convenience. Exact
significand/exponent treatment, rounding rule, container/encoding and LoD mapping
remain undecided. Coarser precision suppresses subthreshold changes but still
requires boundary/oscillation tests; it is not a blanket guarantee of stability.

This is quantization damping, not gradual decay of an accumulated remainder.
It is an intentional resolution-dependent control, not a claim of physically
equivalent trajectories across precision profiles. Outer-LoD precision pairings,
rounding policy and calibration remain to be established. Floating-point spacing
also depends on coordinate magnitude; retain relative frames and test changes of
focus/reference. Secondary compression and naming stay 1:1; storage nesting does
not itself create a conceptual step or a coarser precision requirement.

The fork already defines f64 and advertises data64 for x64, arm64 and CUDA.
HCP's packed boundary currently requires f32/i32, and centroid finalization uses
f64 intermediates before narrowing to f32. Audit the complete HCP numerical path,
rather than assuming that 64-bit addressing supplies double-precision arithmetic
or that every backend/device provides equally fast f64.

The new intended policy supersedes indefinite positional rounding-residue carry
for the new precision path. The built [effective-motion evidence](../engine/docs/EFFECTIVE-MOTION.md)
and #169 updater still describe the previous carried-residue implementation.
A bounded conversion plan must identify the exact state to discard and reconcile
centroid repair, activation, reductions, rebasing, transfer ABI and resource budgets.
Do not erase physical velocity, accepted displacement or necessary mass accounting
under the name of noise removal. Tests must distinguish discarded rounding detail
from real physical state and verify that it cannot leak back through another
remainder. The decision does not authorize a blanket float-to-double substitution
or replacement of every integer/flag with 64-bit storage.

Patrick requested math-core work first. Reconcile affected arithmetic requirements
in the [core operation plan](core-operation-readiness-plan.md) through #172 before
implementing the revised precision path. Existing evidence remains valid for the
source/profile it tested; it does not prove the new policy. Measure backend
capability, precision, cost and memory use explicitly.

### Two independent precision controls

Patrick clarified the distinction on 2026-10-23:

- **Native calculation width:** larger container steps select the calculation
  precision and hardware cost appropriate to the work.
- **Retained significand precision:** trimming terminal binary digits provides
  finer control of stored-position quantization and jitter within a container.

Tune these independently. Reducing retained bits inside an f32 container does
not by itself reduce allocation or transfer bytes, nor promise faster arithmetic.
It changes the expressed resolution. Changing the actual native format changes
the storage/compute tradeoff and requires backend capability and cost checks.
“Trimming” names the intended precision reduction, not a settled instruction to
truncate bits: the rounding rule remains to be calibrated. The per-LoD choices
remain open; no fixed container pairing or reduction ladder is established.

## Model exposure and analytical view

Provide two levels of exposure control: a global study switch and a per-object
switch. The proposed effective request is global enabled AND object enabled;
global concealment retains individual selections for later restoration.
Object/LoD targeting, defaults and unsupported combinations need a bounded plan.

Exposure is separate from both the native awake/sleep gate and browser visibility.
A concealed interior is represented through its rolled-up object and relevant
explicit component fields; exposing it changes the representation through validated
exchange, preserving contribution accounting and avoiding aggregate/part double
counting. Ordinary new occurrences target study focus; existing representation
changes use the outgoing logical centre/current position and nearest-free placement.

The number of simultaneously exposed LoDs can depend on current calculation
capacity and the fields/relationships in play. Include supporting resources and
work, not just particle counts. This does not specify an automatic analyst or
fixed distance-ring policy. Calculable detail and analytically sufficient detail
may differ from what a human can usefully see. Visual parity is a working
assumption, not an accepted universal correspondence.

These view-scaling controls are not an additional GPU-testing gate. They remain
separate from the requested math-core work and the existing operational CPU exit.

## concept.db: compositional recipes across media

concept.db is intended as the master catalogue of established conceptual centroid
identities/labels and compositional recipes. It supplies a download root for the
cold-cache swarm and an established recipe book for assembly across media.
Language, image, sound and other representations can connect to a shared concept
while retaining their own structure and distinctions.

Catalogue entries are reusable definitions, not a single live master centroid or
its current coordinates. A recipe can instantiate many independent occurrences.
Warm remains the shared toolbox and live numerical state belongs to execution.
This direction does not permit private/local content to pass through the shared
cache manager. Concrete schema, address roots, revisions, establishment criteria
and cross-media identity rules remain plan-development work under #173; no new
database allocation or swarm implementation is authorized here.

Preserve the agreed discovery sequence: letter testing, structural word tools
from a separate word list to be acquired later, then LDOCE ingestion through its
provided phases to seed concept.db through analysis. Kaikki is deferred until
that path stabilizes because of its noise. Source documents connect to word
occurrences; the future analysis routine must detect and encode the centroid
relationships that permit compression.

Formally encoded source relationships are the immutable recovery target, not
relationships preseeded from JSON keys into answer-bearing centroid fields.
The model must allow those relationships to be derived; visual clustering alone
does not establish that result. This records the goal without inventing the
analysis algorithm, selecting the word list or defining new ingestion phases.

## Issue and PR discipline

The owning issue retains the problem, accepted direction, unresolved decisions,
dependencies and acceptance criteria. Bounded plan/implementation PRs reference
that issue and state which part they deliver. Use closing keywords only when the
entire issue's acceptance is met. A merged plan or partial implementation does
not close an umbrella concern. Keep accepted contracts in repository documents
and current status in AGENTS; link evidence and source revisions from both the
PR disposition and owning issue.
