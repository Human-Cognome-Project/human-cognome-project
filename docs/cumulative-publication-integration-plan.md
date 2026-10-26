# Cumulative publication and native activity

Status: **accepted in #191, 2026-10-26; C2b-1 implementation released**.
Owners: [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172)
and [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177).
Baseline: `8aa360329ef637d7fcbdf204c94762db0a9b9698`, including accepted C2a
#189 and the cumulative significand clarification #190.

The next bounded change applies the [cumulative-publication direction](precision-exposure-and-concept-catalogue.md#cumulative-significand-clarification--2026-10-26)
to the existing C1 trial profile, then qualifies real weighted centroid arithmetic
with the [C2a native handoff](../engine/docs/NATIVE-HANDOFF.md). These are separate
implementation PRs: **C2b-1, single-level live publication/activity**, followed by
**C2b-2, a test-only multilevel numerical probe** after C2b-1 is accepted. Acceptance
releases C2b-1 now and C2b-2 only after C2b-1 acceptance, not production hierarchy installation.

This deliberately leaves the provisional downward response equation for its
own reviewed contract. The ownership below is concrete for this slice: C2b-1
keeps existing direct field/contact responses and one body integration; C2b-2
produces numerical summaries only and applies no physical response. Neither
adds an ancestor force alongside existing direct forces. Do not report these
slices as completion of coupled live hierarchy or #156's operational exit.

## Accepted evidence and actual seam

The reviewed C2a head is `19919375968eeb73ac4ee2a38fa7bbdd2f118ffe`.
All eight hosted checks passed. The inspected [native CPU rerun](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40238861181/job/121321121785)
reports **84/84 suites in 688.56 seconds**; contributor evidence remains separately
[source-pinned](../engine/tests/native_handoff/README.md) at 84/84 in 528.64 seconds.
The earlier cancelled job is not acceptance evidence. C2a demonstrates coherent
packet delivery, not centroid arithmetic, inherited response or coupled settling.

The current [C1 implementation](../engine/docs/LIVE-NUMERICAL-PROFILE.md) already
combines all field/contact forces before integrating each active body. A dirty
field already performs a fresh ordered full-member reduction, including sleepers,
before its one positional Q publication. It does **not** independently quantize
each member's contribution. Preserve that arithmetic.

The source audit identifies the narrower changes:

- [Integration](../engine/src/field/field.cpp) retains trial particle activity for
  `moved || attempted || velocity_changed`. #190 supersedes the last two as
  independent reasons for movement-triggered activity. `Moving` already denotes
  accepted published displacement and drives contact/field notification.
- The separate determiner currently traverses activity **after integration has
  consumed quiet entries**. Removing velocity-based retention without updating
  this seam could leave a sleeping particle's cached speed stale.
- [Trial centroid marking](../engine/src/field/activity.cpp) already sends every
  dirty field to its complete reduction; finalization sets `Changed` from the
  combined published position. The legacy per-edge gate is not the trial path.
- Count/mass/contact/source invalidation is independent of positional `Changed`.
  [Prepared repair](../engine/src/field/prepared_update.cpp) must retain its own
  wake and history rules even when the repaired centre is unchanged.

## C2b-1: existing live trial profile

### One cumulative destination publication

For an active body, retain the existing combined force accumulation, contact
law, overshoot brake, radial damping and one physical integration. With the
current published position `x`, calculate the complete trial step in f64:

```text
(trial_next, velocity_next) = existing_combined_integration(current_state, inputs)
published_next = Q_body(trial_next)
accepted_displacement = published_next - x
movement_changed = any_component(published_next != x)
discard trial_next - published_next
```

No force is individually filtered before accumulation. Preserve `velocity_next`,
mass, accepted displacement and required history even when publication is quiet.
Keep attempts as diagnostic counts, not work selection. Neither an untrimmed
attempt nor a velocity-only change retains the native bit by itself. With no
other invalidation, a subthreshold body becomes inactive with its velocity
intact; a later valid wake integrates from that stored physical state. This
explicitly replaces C1's continuous sub-grid polling experiment. It adds no
epsilon, speed threshold, velocity clearing or cross-tick positional accumulator.
Inactivity under this publication policy is not a claim of zero velocity or
exact force equilibrium; further integration waits for another valid wake.

Store the trial body's final cached speed from its post-brake/post-damping
velocity within its integration visit, before activity can be consumed. Prepare
and budget the additional output binding once. The ordinary trial tick then
does not need a separate determiner traversal; retain the existing determiner
for initialization/other required preparation and leave legacy tick behavior
unchanged. Do not reuse the pre-brake speed as the final cache value.

Only accepted displacement dirties member fields and propagates movement through
contacts. A field's fresh reduction uses current accepted body coordinates and
`physical_mass * (f64(count)/f64(denominator))` once per selected source row.
Compute its combined candidate before applying `Q_field` once and comparing
with the old published centre. Preserve first-member anchoring, untrimmed mass
and previous-centre history. Do not add accepted displacement to a fresh mean
that already includes it. Trial `group_delta[0..2]` remains transient anchor work,
zero after publication; it must not become a retained positional remainder.

Fine positions that survived their **own** publication remain valid current
inputs to a coarser centroid. Fresh reduction from those accepted positions is
not recovery of a discarded member tail. Parents in C2b-2 likewise consume only
published child positions, never a hidden first moment or unquantized candidate.

### Input invalidation and consumption

Use the existing single particle/centroid NativeSets and ordered type phases.
C2b-1 has no same-type centroid dependencies, overlapping ticks or concurrent
structural writers. All force/contact inputs for the body evaluation belong to
the committed source generation and the completed preceding publication. All
body integrations finish before contact propagation and field publication.

Under that existing phase exclusion, consume a trial body after its complete
evaluation, speed cache and positional publication if it did not publish motion.
Retain its bit for published motion. Later contact/centroid notifications remain
pending for the following particle calculation; no end-of-tick clear consumes
them. Do not transplant this late consumption into C2a's concurrent same-type
poll: its safety here depends on the absence of concurrent notifiers.

Keep parameter/focus/radial-origin invalidation, explicit invalidation, structural
repair, exact-count changes and both contact-index sides. Such work must receive
an evaluation even when the previous coordinate was quiet. A mass/count edit with
unchanged Q position can still change force inputs and cannot be gated away by
`movement_changed`. Invalidating one body does not require keeping every stationary
body active thereafter. Stable ticks acquire no new loaded-array scheduler scan.

### All current readers and writers

The following audit is part of the implementation review. C2b-1 preserves C1's
single published field buffer: it adds no numerical selector or second slot to
production. Native same-type readers/writers do not overlap in this subset.

| Surface | Read/write boundary and required preservation |
|---|---|
| Force and contact | Read completed published state before integration. Existing direct responses accumulate into each body's force once; no centroid is another physical body. |
| Integration, determiner and motion | Integration owns final velocity, accepted displacement and trial speed-cache update before consuming its bit. Legacy determiner/consumption semantics remain intact. |
| Field reduction/finalization | After all body writes, one writer per dirty field; complete member reduction followed by one Q and positional `Changed` decision. History stays separate from transient anchor work. |
| Field-to-particle wake | After finalization, read complete current/previous centres and preserve source/mass invalidation; consume before notifying particles whose calculation phase has finished. |
| Focus and universal mass | Focus reads completed published fields at tick start; changed radial inputs keep existing invalidation. Universal mass counts physical occurrences once, not summaries. |
| Seed and coherent repair | Ordered, nonoverlapping with ticks. Same Q/accounting rules; old published history for repaired survivors, initialized history for new fields, final speed consistent with new physical state. |
| Geometry, optional candidates and inspection | Explicit ordered operations on completed state; no concurrent mutation. Preserve typed f64 reads, generation checks and candidate invalidation. Host observations are not state authority. |
| Reset, retirement/reuse and failures | Reset only at the existing explicit boundary. Ordinary rejection preserves state/activity/history/readiness; failure after mutation marks execution unready. No partial new state is advertised as ready. |

### C2b-1 acceptance

Use both CPU host paths and actual CPU backend checks. Retain legacy regression
and all current trial precision/count/update/geometry tests, revising only tests
whose explicit provisional activity expectation is superseded. The original
C1 retained logs remain historical evidence, not new passing results.

Use independently calculated scalar/long-double and exact binary fixtures:

1. Body field/contact contributions individually below its publication threshold
   but collectively crossing it; opposing contributions cancelling before Q;
   both coordinate signs, cell boundaries and full-bit control. Verify one
   integration and unchanged force, brake and damping calculations.
2. A centroid crossing caused by individually subthreshold **normalized weighted
   contributions**, cancellation, unequal masses/exact ratios, repeated ordinals
   and sleeping members. Check fields against the existing independent oracle.
3. Different body/field retained bits, final subthreshold publication and at least
   32 quiet ticks. No drift/residue recovery, recurring attempts or reductions
   after work has been consumed in the constant-input no-motion fixture. Velocity
   remains intact and the cached final speed remains correct, including a
   velocity-changing step that publishes no displacement.
4. Wake those inactive fixtures through parameter/force changes, explicit
   invalidation, changed focus/radial input, both contact-index sides and
   exact-count/source edits whose Q centre is unchanged. Verify the current
   input is evaluated, publication crossing is preserved, and survivor physical
   state/history is not reset. Coherent fixture insertion remains fixture work.
5. Allocation/backing reuse, independent occurrences, rejection and numerical
   failure boundaries. Keep existing finite/normal-or-zero and FTZ restrictions;
   no production rounding policy or automatic precision ladder is selected.

A concrete centroid witness uses Q_4 in `[1,2)`, spacing `h=1/16`: two equal-mass
members initially at `1` and `1.25` have centre `1.125`. Move both by `3h/2` with
fine body publication. Each normalized contribution is `3h/4`, insufficient
alone, but their sum is `3h/2` and the centroid publishes `1.1875`. Opposing
movements cancel. An unequal-ratio witness starts both members at `1.125`, uses
weights `1/4,3/4` and movements `3h,h`, and produces the same two `3h/4`
contributions. These numbers define test
inputs, not a general tuning of model scales.

## C2b-2: multilevel numerical handoff probe

This separate native C++ test qualifies the cumulative arithmetic across ordinary
polls without installing live hierarchy into `Harness`. Keep C2a's unsafe controls
and source-pinned evidence. The new probe replaces its toy maximum-tag operation
with actual weighted centroid arithmetic and independently checked input packets.
It does not apply forces, advance body motion, modify production field values,
or claim coupled physical settling.

Use an explicitly supplied fixed calculation tree from one prepared source
generation and [#185's compatible accounting](../engine/docs/WEIGHTED-CENTROID-CASCADE.md).
Identify each expressed node by full field handle, comparison context and declared
conceptual level; require distinct native field ownership for expressed outputs,
even though the probe's numerical arrays are separate from live field storage.
Reject aliased expressed outputs, cycles, missing roots, stale handles/rows,
incompatible coverage and edits to a bound generation. Distinct occurrences and
source ordinals remain distinct; aliases of one source row coalesce. Exact parent
per-occurrence coverage equals summed child coverage. Do not infer counts or
repartition overlaps. Restrict this probe to expressed nodes; anonymous internal
arithmetic grouping and arbitrary model graphs are outside its supported binding.
Reuse the existing weighted candidate validation for source identity/coverage
where possible, adding the probe's distinct-output check rather than creating
another general hierarchy or identity API.

Initialize all nodes from current accepted bodies upward through published
children before marking the probe ready. This one-time ordered initialization
may use existing candidate arithmetic. Thereafter each ordinary native poll:

1. Consumes the node bit **at entry**, before input acquisition.
2. Acquires each child's selector with a retained native SC atomic read; reads
   complete published XYZ and untrimmed mass from that slot. Leaf source inputs
   are fixed throughout the poll and include sleeping bodies.
3. Calculates one ordered f64 mass-weighted candidate from the acquired inputs,
   applies that node's declared Q once, and compares the complete published
   packet. A mass-only change still publishes and notifies its parent.
4. Writes the whole inactive packet, atomically publishes its selector, then
   declares the parent's native bit. Never consumes again in a propagation pass.

One writer and at most one publication per node per poll are mandatory. There
are two preallocated numerical slots, one data selector and one native activity
set. A selector chooses data, not work. All readers finish before either slot
may be reused by the following ordered poll; neither poll overlap nor concurrent
source edits are supported. A reader may use a previous/current complete child
publication, and siblings need not form a simultaneous global snapshot. Preserve
the compiled-ordering and finite-domain requirements of C2a. No lock/spin, second
awake mask, epoch, host queue, per-depth ordinary solve or recursive substep.

Source replacement between completed probe polls invalidates the binding;
explicit rebind/reinitialization is required before another poll. No live body
tick may be coupled to stale probe metadata. Ordinary rejected operations preserve
the source execution; probe readiness/observation invalidation is separate.

### Every selector access and lifetime

Maintain a source-to-compiled-access inventory for the actual arithmetic kernels:
own-slot acquisition, every indirect child-selector path, publication exchange,
and any native reader added by the implementation. Inspect compiled IR for
**each required path** and its SC ordering. Aggregate counts such as C2a's
`acquire >= 2` are insufficient. Retain the relevant instructions plus the
argument/index/control-flow mapping; fail qualification if a required atomic
is demoted, missing or cannot be attributed. State that this is retained Program
LLVM before the launcher executable copy, separately from native CPU execution.

Only the poll kernel reads/writes packets concurrently. Initialization runs
before polls; configuration, perturbation, observation, reset and reuse run
after dispatch completion. Inspection reads both slots/selectors only while
writers are stopped. Diagnostic records of each acquired child packet permit
checking that it was a complete pre- or post-poll publication; reconstruct the
node result from its actual acquired inputs, rather than requiring all siblings
to share one instant. Diagnostics never select work.

No force/focus/repair/geometry reader uses these probe arrays. Before a future
production installation, every reader in the C2b-1 table must adopt a qualified
selector access or be excluded by a proven ordered boundary. Production history,
repair and prospective publication lifetimes are not free reuse of the probe's
slots or C1's eight-row field buffer.

### Multilevel numerical acceptance

Run one/two CPU worker configurations, both source host paths, same-word and
adjacent/distant pointer-block orders, holes and the last valid row. Include
two/three-level fixtures and a five-node chain, fan-in, repeated notifications,
initially sleeping/already-active parents and deterministic before/after-consume
orders. Include the cumulative witnesses above at both leaf and parent boundaries,
unequal exact weights, compatible overlaps and independent occurrences.
Skipped conceptual levels introduce no invented scale; internal aggregation and
secondary compression remain 1:1. Do not add a distance multiplier in this probe.

For each fixed-input change, declare a **16-poll limit**, then require the
independently computed hierarchical published reference, correct masses and no
pending work through **8 additional polls**. Also check intermediate reductions
against their recorded acquired inputs. An empty mask with a stale answer fails.
Apply another local/deep perturbation and a mass-only change without reset;
repeat at least **32 prepared reset/reuse rounds**. A limit reached is a failed
qualification, not permission to add a scheduler or silently weaken the criterion.

Use exact integer coverage checks and an independent host reference for the
declared hierarchical Q boundaries. Use exact expected coordinates for binary
fixtures; separately bound count-to-f64, ratio and reduction error for inexact
weights. Cumulative crossing/cancellation witnesses must have a justified result
at their Q boundary, not pass because a floating tolerance hides the wrong cell.
Numerical comparison bounds never become activity or sleep thresholds.
The full candidate query may cross-check
selected frozen observations, never drive each native poll. Record work/polls,
publications, consumed/pending activity, acquired inputs and elapsed cost. This
establishes numerical convergence for held inputs; coupled moving-body settling,
oscillation and inherited-response recovery remain unqualified.

## Resources, implementation review and later release

C2b-1 should add no capacity-sized numerical surface or activity set. Charge the
actual new integration output argument/context reservation and update prepared
binding checks. Keep integer/count widths unchanged and report the removed trial
determiner dispatch separately from numerical work; no performance guarantee.

C2b-2 charges `N * (2*4*8 + 4) = 68N` native publication/selector bytes before
host inspection copies. Inventory selected source identities/counts/denominators,
body snapshots if copied, topology/child/source adjacency, per-node retained-bit
settings, source validation, acquired-input diagnostics, counters, inspection
copies and prepared contexts. Charge host/native and committed/prospective copies
actually present. C2a's diagnostic 27,832-byte payload is not a budget for this
new arithmetic probe. Native pointer/list/pool backing and compiler objects are
measured separately; host vectors are not pinned memory.

Provision and warm arrays, pointer blocks, native list high-water marks, kernels
and contexts before measurements. Require zero observed linked C++ new/new[]
and unchanged measured native backing/layout/compiler counters over at least
32 reuse rounds. Separate setup, first use, ordinary kernels, adjacency/bit work,
inspection/upload/readback and updates. Preserve the existing limits of these
observations: they do not account for arbitrary malloc, page faults or drivers.

Each implementation PR retains source/backend/compiler pins, actual CPU checks,
raw focused results and the complete native regression (currently 84 suites plus
new tests). Run browser validation/build only if report/capture interfaces change;
no new browser surface is proposed. Hosted acceptance remains separate from
contributor evidence. GPU protocol qualification stays deferred under #145.

Following C2b evidence, a new production hierarchy/response plan still must resolve:

- Distinct live comparison ownership, anonymous grouping, complete initialization
  and atomic hierarchy/source refresh or pre-mutation rejection of unsupported edits.
- Every production selector access and publication/history/repair lifetime,
  including actual specialized compiled kernels and future backend qualification.
- A reviewed response producer and units, participating mass/coverage distribution,
  direct-versus-inherited ownership, duplicate-influence identity and sleeping
  recipient invalidation. Numerical examples must cover two levels, repeated
  ordinals and compatible overlap before any inherited equation is implemented.
- One combined physical force accumulation/integration, and coupled multilevel
  settling plus perturbation/source-update/insertion recovery across ordinary ticks.

This plan changes no runtime, database, cache-manager or analyst code. It closes
none of #172/#177/#156/#145 and creates no immediate GPU gate.
