# Live hierarchy handoff and response ownership

Status: **accepted in #188; C2a probe accepted in #189, 2026-10-26**. See the [native investigation](../engine/docs/NATIVE-HANDOFF.md) and
[source-pinned CPU evidence](../engine/tests/native_handoff/README.md).
Owners: [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177)
and [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172).
Baseline: merged C1, `848fa4bb51a103422ba3ec2afce02eb93d91c873` (#187).

C1 supplies the [single-level live f64 profile](../engine/docs/LIVE-NUMERICAL-PROFILE.md).
All nine hosted checks passed on reviewed `fd3c520`; the maintainer-inspected
native job 121160291530 reports 82/82 suites in 685.51 seconds. Its acceptance
commit `145349f` changes six Markdown files only. Contributor evidence remains
separately [source-pinned](../engine/tests/numerical_profile/README.md).

The next seam is same-type centroid propagation. The existing cross-type wake
sequence cannot simply be reused for child-to-parent notifications. This plan
releases **C2a: a bounded native handoff investigation and executable probe**.
It also records the contracts that must be resolved before installing live
hierarchy or inherited response. Acceptance releases C2a only, in its own
implementation PR. The remainder below is a following review boundary.

## C2a acceptance and next action — 2026-10-26

#189 is merged at reviewed `1991937`, after hosted native CPU rerun
[40238861181](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40238861181)
and repository smoke workflows passed. The [probe findings](../engine/docs/NATIVE-HANDOFF.md)
qualify the bounded publication/notification protocol, not live centroid arithmetic
or coupled settling. Contributor test counts remain separately sourced.

The next contribution is a bounded integration-plan PR combining the proven
handoff and its slot/compiler lifetime requirements with the cumulative-publication
clarification below. Resolve response ownership and activity/invalidation boundaries
before releasing live implementation; do not reopen the test-only probe as a math
implementation or silently treat this continuation as already released.

The accepted #191 [cumulative publication/activity plan](cumulative-publication-integration-plan.md)
releases C2b-1 now, with C2b-2 following its acceptance: existing single-level trial gating and
speed-cache correction, followed by a separate test-only weighted arithmetic probe
using C2a's handoff. It keeps direct-response ownership explicit and releases no
inherited equation or production hierarchy. The following installation contract
below remains held until its distinct response and lifetime decisions are reviewed.

## Existing behavior and the specific gap

- [NativeSet](../engine/src/field/native_set.h) uses fork-native pointer and
  bitmasked SNodes, with activate/deactivate/is-active operations. Parent pointer
  blocks are prepared before ordinary work; sparse declaration alone is not
  proof of backing.
- [Bitmasked runtime operations](../engine/taichi/taichi/runtime/llvm/runtime_module/node_bitmasked.h)
  atomically OR/AND activity bits. Those atomics do not establish a coherent
  publication protocol for separate XYZ/mass arrays.
- [Struct-for lowering](../engine/taichi/taichi/transforms/offload.cpp) generates
  container lists. [LLVM loop code](../engine/taichi/taichi/codegen/llvm/codegen_llvm.cpp)
  still tests each live leaf bit before entering its body. The existing
  [CPU probe](../engine/tests/native_activity_test.cpp) demonstrates that a peer
  activated inside traversal can enter that same loop. Container lists are
  not immutable snapshots of active leaf entries.
- C1 reduction/finalization precedes [centroid touch](../engine/src/field/activity.cpp).
  Touch consumes a centroid before activating particles, whose calculation
  phase has already finished. It emits no centroid-to-centroid notifications.

For example, suppose only child record 3 participated in reduction. During a
later propagation traversal it activates sleeping parent record 4. A live-bit
test can now admit parent 4 into propagation even though it never reduced; that
parent can consume its notification without calculating the changed input.
An already-active parent can also consume a later child notification after its
own reduction read the older child. These are ordering questions, not failures
of the existing C1 cross-type path.

Consuming at loop entry is a candidate to investigate, not a sufficient proof:
it must also preserve input/publication coherence and prevent a later phase
from consuming an unevaluated new activation. No worker/record visitation order
is a supported contract. A quiet activity mask cannot demonstrate resolution.

## C2a: accepted investigation and probe release

1. Trace the actual fork facilities available through native C++ IR/runtime:
   list generation and reuse, live leaf tests, native atomic operations,
   publication ordering and ordered dispatch. Separate documented/compiler
   behavior from observations on one CPU. Reuse existing facilities first.
2. Extend the existing native activity investigation with a small C++ collision
   probe. Model centroid calculation, publication, consumption and parent
   notification explicitly, with diagnostic values showing which complete
   child publication the parent used. Probe data are test observations, not a
   production work registry.
3. Demonstrate the harmful orders and investigate a compliant handoff. A change
   published after the parent's consumed input must either be included in a
   complete parent evaluation or leave native work pending for a following
   ordinary poll. A propagation-only visit must not consume an unevaluated
   parent. Published mass/XYZ inputs must not be a torn mixture.
4. Retain source/backend/compiler pins, raw outcomes, work/activation counts,
   input/output values, elapsed cost and prepared-resource observations. State
   a concrete native protocol with its supported ordering, or a demonstrated
   gap and the smallest further facility needed. An unsupported result must
   remain explicit; it does not authorize a fallback scheduler.

C2a changes no production field law, numerical profile, tick order or hierarchy
binding. It does not install child-to-parent dependencies, add inherited force,
or change source data. If investigation requires a fork/runtime extension,
describe it for separate review rather than silently including it. Native
capability findings must be reviewed before the live integration plan is released.

### Probe matrix and acceptance

Exercise an inactive parent, an already-active parent, a consumed parent, multiple
children notifying one parent, repeated notifications, and a short nested chain.
Force before/after-consume orders using bounded deterministic native dispatches;
separately stress concurrent traversal. Do not use an unbounded spin or an assumed
GPU block/worker order to make a test finish. A deterministic negative control
must expose the unsafe protocol; a stress run alone cannot prove safety.

Cover one and multiple CPU workers; same 32-cell bitmask word; adjacent and
separate pointer blocks; padded capacity with holes/final valid records; and
both relevant source/parent record orders. Check that publication precedes its
notification under the proposed ordering, payload remains valid after bit
consumption, duplicate native notifications collapse, and a consequential final
publication cannot disappear when the input becomes quiet. Record whether new
activation can enter the current traversal rather than assuming it waits.

Provision probe arrays, pointers, list high-water marks and prepared contexts
before measured reuse. Run at least 32 reset/reuse rounds; distinguish setup,
first use, adjacency/bit work, arithmetic, transfers and inspection. Require
zero observed recurring linked C++ allocations and unchanged measured native
backing/layout/compiler counters within the existing measurement limits.
Diagnostic per-record counters are allowed only as test evidence, never as
scheduling state. Add no payload proportional to potential address capacity.

Run the existing 82 CPU suites with the new probe on the pinned implementation.
Require actual CPU selection and keep native GPU qualification under #145.
CPU observations do not establish CUDA memory-ordering guarantees. Source-level
ordering obligations remain explicit; GPU execution of the eventual protocol
belongs in the deferred local matrix, with unsupported capability rejected.

## Latest cumulative-publication direction — 2026-10-26

The [cumulative significand clarification](precision-exposure-and-concept-catalogue.md#cumulative-significand-clarification--2026-10-26)
governs the following integration plan under #172/#177. Accumulate the tick's
applicable contributions at calculation precision before the destination's
significand filter, compare the net with its stored position, and base
movement-triggered activity on that cumulative published change. Preserve
pending-input invalidation, physical velocity, mass and required history.
No discarded positional tail is carried between ticks or recovered from members.
Overshoot braking and radial damping are unchanged.

This supersedes the provisional any-untrimmed-attempt activity policy as intended
direction. The C1 descriptions and evidence below remain records of built behavior,
not evidence that this revision is implemented. C1 already combines centroid
inputs before publication; audit the actual gating seam rather than assuming
each member is independently trimmed by that reduction. Require cumulative
threshold-crossing, cancellation and multilevel no-lost-input tests. Integrate
with C2a's publication/consume contract without a same-tick global solve.

## Following live installation contract

These proposed bounds make the next release reviewable; **they are not released
by C2a acceptance**. Bring back an integration plan with the proven handoff and
resolved response ownership before changing live fields.

### Comparison identity and numerical ownership

Start with an explicitly supplied fixed calculation tree rooted in one prepared
execution/source generation, using #185's checked compatible coverage. It is a
bounded calculation tree, not a claim that memberships form a global tree.

Use full field/occurrence handles, comparison identity and declared exposed
level. Propose the first live subset with one expressed comparison per distinct
native field record, rejecting two expressed contexts that would publish into
the same record. Candidate-only contexts may share source fields; candidate
acceptance does not permit their live outputs to alias. Do not manufacture new
DB identities or infer conceptual levels from record slots/SNode depth.

Preserve exact source row/generation/ordinal identities, alias rules and
per-occurrence count/D coverage. Distinct copies and distinct ordinals stay
distinct. For each parent, child coverage must equal its declared source cohort
exactly; reject incompatible overlaps, cycles, missing roots and stale/recycled
references. First-subset limitations do not impose a model-wide ban on overlaps.
Anonymous same-comparison internal grouping is temporary arithmetic, not a
second independently published live field; its representation must be specified
before allowing it in the installed subset.

No centroid becomes an extra physical body. Universal mass still counts each
physical occurrence once. Keep direct source provenance separate from live
child dependency adjacency and charge both explicitly. A bound hierarchy cannot
silently remain valid across a coherent source edit; define an atomic refresh
or reject the unsupported edit before mutation. Do not advance body motion
against stale hierarchy metadata and repair it later.

### Initialization, ordinary polls and precision

Initialize all expressed centroids before hierarchy readiness, from finest
current body contributions through published child inputs to one root. A
one-time ordered initialization may reuse the candidate arithmetic; it is not
permission to run a full hierarchy solve every tick.

Thereafter use the proven native poll/publication cut. Leaves use accepted body
state, including sleepers; parents use complete published child centres and
untrimmed participating masses. No second participation share, recovered
positional tail or distance multiplier is applied to a child summary. Dirty
dependencies propagate across ordinary ticks; no per-depth barrier loop,
recursive substeps or same-tick global-fidelity requirement is introduced.
Specify which published inputs a poll can observe and when a later change is
guaranteed another opportunity to calculate.

Retain C1's f64 physical/history state, explicit experimental Q at expressed
publication only, discard of positional detail, finite/FTZ limits and failure
policy. Propose an explicit retained-bit value per expressed comparison, with
the execution's body profile unchanged. No inferred precision ladder or
production rounding is selected. Conceptual transitions alone can carry their
separately declared distance scale; internal grouping and compression/naming
remain 1:1. Do not introduce new scaling or unqualified live rebase here.

### Local and inherited response decision

The provisional downward direction does not yet define an equation. Before a
hierarchy replaces any force-consumed field, the following table must have one
reviewed concrete answer, including numerical examples for two levels, repeated
ordinals and compatible overlap:

| Obligation | Required answer before release |
|---|---|
| Producer and units | Which comparison produces a force or an acceleration, its represented mass, and which complete published inputs it uses. Do not mix these units implicitly. |
| Recipient coverage | How that response applies proportionally to exposed physical occurrences using validated coverage/current physical mass; what happens when a body participates through several child contexts. |
| Local versus inherited ownership | Which direct C1 field responses remain and which are replaced, so the same influence is not applied both directly and through an ancestor. Equal vectors alone are not duplicate identity. |
| Timing and integration | The normal phase delivering the response, its publication lifetime/invalidation, and one combined force accumulation followed by one physical body integration. No second integration or artificial delay coefficient. |
| Wake and updates | How changed inherited input reaches sleeping recipients without a second awake registry, and how insertion, count edits, retirement/reuse and failed publication affect pending work. |

This plan deliberately releases no guessed distribution equation, matching rule,
overlap repartition or duplicate all-level force pass. Upward summaries alone
cannot claim a complete coupled live hierarchy. Do not alter the cache manager,
analyst, private-input boundary or cold database to resolve this numerical seam.

## Eventual coupled acceptance and release boundary

After the integration contract is accepted, retain two/three-level fixtures,
independent occurrences, repeated/fractional source contributions, compatible
overlaps, skipped levels and 1:1 internal grouping. Check complete initialization,
accounting and published-child Q boundaries against independent references.

For held body inputs, require the live published hierarchy to reach its declared
reference within a stated bounded number of ordinary polls and remain there
through an additional observation window; no pending consequential change may
be stranded behind an empty mask. For coupled moving fixtures, declare the
tested settling criterion and observation limit before running. Record position,
velocity, published-input stability, attempts, pending native activity, oscillation,
ticks, elapsed time and particle/field/adjacency work. Report a limit reached
honestly rather than manufacturing a universal epsilon or a passing sleep rule.
Repeat after local and deeper perturbations, source edits and bounded fixture
insertion. Fixture insertion is not #170 production admission.

Use the full candidate query only as an explicit reference after selected
observation points. Require single-level regression, survivor/history preservation,
coherent ordinary rejection and unready post-mutation failure. Preserve C1's
persistent sub-grid activity watch point: quiet coordinates with continued
attempts/work are not complete computational settling.

Budget every comparison record, dependency/source adjacency, previous/candidate
publication value, mass/history, response buffer and context actually required by
the chosen protocol before implementation. Charge simultaneous committed and
prospective storage, typed host/native copies, transfers and setup/first-use/reuse
costs. Do not allocate, rebuild layout or compile per ordinary poll/update.
One native activity set per existing type remains the constraint; no second
awake mask, scheduling epoch, custom queue or loaded-array scheduler is released.

The review sequence is accepted C2a plan → native probe implementation/evidence
and review → concrete live hierarchy/response contract → separately reviewed
implementation and settling/recovery evidence. This plan closes none of
#177/#172/#156/#145 and adds no immediate GPU gate.
