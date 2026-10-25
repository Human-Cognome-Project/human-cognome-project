# Live numerical profile and positional residue separation

Status: **accepted in #186, 2026-10-24; C1 implementation pending**. Owners: [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172)
and [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177),
with the operation matrix under #156/#170. Baseline: `main` at
`1c880cc7e89cec8e4cdcfee7f98762b29f54c794`, the merge of approved #185.
All eight hosted checks passed on reviewed `dce69adaacfc8c4a78d0c8d717438698ee3fed0f`;
the inspected native log reports 74/74 CPU suites in 551.91 seconds.
Contributor evidence remains separately pinned to
`58ccae54da69bede55b49e6e3bc8e05eb58f3f8e`, 74/74 in 454.04 seconds.

The [precision primitives](../engine/docs/NATIVE-PRECISION.md),
[source counts](../engine/docs/SOURCE-COUNTS.md) and
[weighted candidates](../engine/docs/WEIGHTED-CENTROID-CASCADE.md) are built.
They do not yet change live arithmetic. The next accepted deliverable is
**C1: one coherent live f64 trial profile with explicit positional publication**.
Acceptance releases C1 only. Live hierarchical propagation and inherited
response need the following bounded contract after C1's evidence; they are not
silently included in this release.

## Why this slice comes next

The [state audit](../engine/docs/PRECISION-STATE-AUDIT.md) identifies an actual
mixed-purpose buffer: `group_delta[0..2]` accumulates accepted member motion,
then finalization overwrites it with a weighted rounding remainder. Clearing it
would lose physical accounting; retaining it unchanged would reintroduce the
positional detail #172 intends to discard. `group_delta[3..5]` separately holds
previous published centres used by the native wake phase.

Correct this ownership and the numerical boundaries together. Keep the existing
field/contact/integration laws and source graph. Do not treat widening only the
final divide, installing a candidate result into an f32 field, or clearing the
old buffer as a live precision conversion.

C1 operates on the existing independent source fields, one published centroid
per live field record. It adds no child-centroid dependencies, extra comparison
records or analyst matching. Different fields may overlap as before; exact
counts apply within each field, not as a global cap across unrelated fields.
The full hierarchy query remains an optional independent reference.

## Accepted C1 trial numerical contract

Select the numerical profile at prepared-execution creation. Keep the existing
f32 profile and its public behavior as the regression/control path. The new
trial profile owns one authoritative set of numerical arrays; it is not an
f64 shadow copy of a simultaneously running f32 simulation. Reject incompatible
typed views or profile changes before mutation. No implicit mid-run migration.

| Surface | C1 trial choice and reason |
|---|---|
| Body position, velocity, force | f64 storage/calculation. Preserve useful sub-f32 motion and combine fields before positional publication. The current homogeneous body array may keep all eleven rows f64, including mass/radius; inventory that actual choice rather than claiming mixed-width rows. |
| Physical mass/radius and scalar parameters | f64 operative values and operands for force/contact/integration. Existing source values may already be f32; widening them recovers no lost detail. Constants and new typed parameters must enter sensitive calculations at their declared width. |
| Published field centres, participating masses, reductions | f64; recompute from current accepted body positions and exact source ratios. Apply positional precision once at publication; never trim mass. |
| Accepted displacement and previous-centre history | Separate named f64 physical/history surfaces, not a positional remainder. |
| Focus, universal physical mass, update/repair outputs and geometry | f64 sensitive arithmetic and typed publication/readback, consistent with the selected execution. Universal mass still counts physical bodies once. Focal selection retains its supplied field weights, including their existing overlap meaning. |
| Source shares and exact accounting | Keep the legacy f32 share and u64 denominator/count metadata. New live weights use native f64 count/denominator ratios, never a reverse conversion of rounded shares. |
| IDs, source ordinals, CSR, flags and activity | Existing integer widths/native machinery; no blanket doubling. |
| Optional diagnostic/report calculations | f32 only where a declared range/error check establishes adequacy. Such values cannot feed motion, centroid, geometry or activation decisions. The authoritative inspect path remains f64. |

Adapt existing native dtype/scalar/transfer/packed-view facilities. Do not create
a second numerical runtime or silently narrow through existing float vectors,
geometry summaries, update candidates, reports or capture. Candidate queries
must read the selected resident body dtype directly and retain their f64 result
contract. Existing f32 interfaces remain valid for their original profile.

The trial requires known, valid accounting on all participating live source rows:
positive count/denominator, declared share conversion and checked per-field,
per-occurrence coverage no greater than the denominator. Unknown or inconsistent
accounting rejects before publication. Legacy execution retains its unknown-
metadata behavior. No new counts, partitions or memberships are inferred.
Use the same participating ratio in force, centroid, response-exclusion/wake
checks and repair. A single-member field cannot gain a fictitious other-member
force from inconsistent weight rounding. Total physical mass still absorbs the
combined forces once.

Expose explicitly configured retained fraction bits for body and field position
publication, within 0..52. Full precision is b=52. Settings belong to the trial
input, not an inferred LoD ladder or DB address rule. Storage remains f64.
Experimental toward-zero Q_b is an opt-in trial policy, not a production rounding
decision. Keep its crossing/origin bias and the fork's FTZ limits explicit.

## Physical state and publication

For an active body, evaluate the existing combined force/contact/integration
calculation in the selected precision. Produce trial position and physical
velocity, then apply the configured positional publication once:

```text
published_next = Q_b(trial_next)
accepted_displacement = published_next - published_old
discard trial_next - published_next
```

The discarded difference is neither velocity nor a future position input. Do
not carry it in a remainder, compensation term or hidden unquantized position.
Do not trim each field's incoming force independently, zero physical velocity,
or use speed as a new sleep threshold.

Provisional C1 activity rule: retain the particle's native bit when the
untrimmed integration trial attempts displacement, the published position
changes, or physical velocity changes. An unchanged Q_b coordinate alone cannot
consume genuine ongoing physical work. Exact fixed-state checks use the declared
calculation width, with no arbitrary epsilon. Keep motion-attempt state distinct
from accepted movement; only accepted displacement changes centroid inputs.
Report cases that keep attempting sub-grid motion instead of labelling them
settled because their published coordinates are quiet.

For each field affected by accepted member motion, declare its existing native
centroid bit. In C1, recompute that field once from all its current accepted
members, including sleepers, using ordered anchored f64 reduction and exact
source ratios. This deliberately replaces the new profile's preliminary
incremental residue gate with one dirty-field reduction; measure the changed
work. It does not alter the legacy profile's gate.

The physical member positions retain all accepted motion across ticks. A fresh
reduction therefore includes collectively relevant small member movements
without retaining discarded centroid rounding detail. Publish Q_b of the
combined centre once, retain its untrimmed participating mass, and discard its
trial-to-published difference. Do not write that difference into pending motion.

Keep the previous published centre until its normal dependency/wake comparison
has consumed it. A changed field qualifies particle candidates through the
existing response/input checks; it does not indiscriminately wake every member.
A candidate still evaluates all connected fields before its combined physical
decision. Preserve both contact endpoint directions and focal invalidation.

The single-level native phase order remains particle calculation/consumption,
contact propagation, dirty-field reduction/publication, then centroid consumption
and particle activation for the following particle phase. These final activations
target a type whose calculation has finished. C1 adds no centroid-to-centroid
reactivation race, second awake mask, epoch, host queue or loaded-array scheduler.

### Trial watch point: persistent sub-grid activity

Patrick delegated this choice to maintainer judgment, with monitoring, on
2026-10-24. Provisionally retain activity for ongoing untrimmed motion attempts
as specified above. This avoids consuming genuine physical motion without a
proven reactivation rule; it is not a permanent decision that every sub-grid
attempt must run indefinitely. Preserve velocity/history and discard positional
rounding detail exactly as required.

A constant velocity can repeatedly attempt a displacement below the Q_b cell
while its published position stays fixed. Record that as positional quiescence
with continuing activity, not complete computational settling. Require a bounded
constant-velocity fixture, changing-force/velocity cases that eventually cross a
publication boundary, and subsequent contact/field perturbations. Report attempt,
publication-change and active-work counts plus elapsed cost using observations
of the existing native work, not an additional scheduler or activity state.

C1 acceptance checks correct behavior and reports this cost; it does not require
all trial fixtures to become inactive. If persistent sub-grid work is material,
revisit the activity/publication policy under #172/#177 with measured evidence.
Do not add an epsilon, zero physical velocity, preserve a hidden positional
remainder, or force sleep merely to make a settling metric pass.

## Initialization, edits and the other numerical surfaces

- Seed every exposed source field before populated readiness using the same
  f64 weight/reduction/publication rule as repair and ordinary ticks. Empty
  prepared execution remains valid. Source starts remain private test fixtures;
  this plan adds no mandatory coordinates to structural recipes.
- Carry exact source metadata through the existing prospective commit boundary.
  Prepare any native u64 descriptors and validation workspaces up front. Count,
  mass/topology and profile validation precedes numerical publication; native
  descriptors cannot silently lag a committed generation or compacted row.
  A count-only edit can change the f64 weight while leaving the legacy f32 share
  identical. In the trial profile it must repair mass/centres and invalidate
  affected force/wake inputs, even when Q_b leaves the published centre unchanged.
  It remains metadata-only for legacy physics where the legacy inputs are equal.
- Update/repair must preserve survivor position, velocity, accepted displacement
  and required previous-published history. A fresh repair includes pending
  accepted body state exactly once. New/retired rows initialize/clear only their
  own state, and recycled records inherit no former metadata or history.
- Geometry/contact preflight must use the same current f64 body/focal values
  that the trial tick uses. Preserve outward source extent bounds, exact contact
  predicates and the existing finite-domain rejection policy. No placement or
  transactional moving-contact implementation is supplied by C1.
- Focus is an untrimmed f64 combination of selected published field centres.
  Do not add another deliberate Q_b stage at that intermediate control value.
  Keep the double frame anchor and the existing local operative frame. Source
  conversion has already happened; no new distance multiplier is introduced.
- C1 does not implement the currently unbuilt operational rebase. Later #170 D6
  must translate current/previous centres, body state and focal controls coherently
  and qualify the quantization grid or reject before mutation. Old residue-
  preservation assertions do not authorize carrying a new positional remainder.
- Reports/capture identify the actual selected profile and publication precision.
  Check sensitive f64 detail survives their authoritative readback. Formatting or
  diagnostics cannot become another live numerical state authority.

Ordinary pre-publication rejection preserves committed arrays, exact metadata,
source generation, focus/anchor, activity/history, Demand and readiness. A
nonfinite calculation or backend/invariant failure after an in-place tick has
mutated state makes execution unready; it cannot return a healthy rejection or
claim rollback. Check the qualified finite/FTZ domain explicitly. Keep #170's
future candidate-motion/contact validation contract intact; C1 does not claim
that the current in-place tick has acquired that transaction.

## Prepared resource contract

Use the existing maps, native activation/struct traversal, ndarray dtype and
strict ordered IR operations. Provision every selected typed array, source
descriptor, precision mask, repair/geometry/candidate/inspection workspace and
prepared context before first use. Ordinary updates and ticks cannot allocate,
recompile, change layouts or silently expand capacity.

Derive each manifest term from actual count, width and copy ownership. Record
committed/prospective peak storage and lifetimes, including native exact-count
descriptors, accepted-motion/history and temporary publication work. Charge
binding ABI/cache metadata and transfer bytes separately. Do not double integer
budgets or omit old/new overlap during a structural commit. Fixed objects,
runtime/compiler/pool overhead and physical residency remain separately stated.

## C1 implementation and acceptance

Publish C1 as its own reviewed implementation PR after this plan is accepted.
Do not release a partially converted public f64 tick while repair, geometry or
inspection still narrows its authoritative state.

1. Add the immutable profile and typed prepared surfaces, explicit numerical
   constants/parameters, native source-ratio descriptors and manifest/binding
   accounting. Preserve the legacy ABI behavior and default path.
2. Convert the complete selected tick/seed/repair/geometry/query/readback path,
   separating accepted physical/history state from discarded positional detail.
   Reuse the built Q_b and native u64/f64 facilities. Add no new force law.
3. Retain the following CPU evidence, run the existing regression matrix, and
   submit the bounded implementation and source-pinned results for review.

Required tests establish implementation behavior, not a new proof of the model:

- Useful sub-f32 increments survive native f64 calculation, storage, scalar
  transport, update/geometry/candidate access and both host paths. Compare against
  independent high-precision references with the same publication cuts.
- Known full/fractional counts, thirds, repeated ordinals and independent copies;
  unequal physical mass versus structural denominator; no accidental f32-share
  product or duplicated universal mass. Preserve source-generation rejection.
- Full b=52 and representative trimmed profiles at both signs, within a cell,
  at crossings and with cancellation. Several contributions can combine before
  publication. Repeated ticks cannot recover discarded positional detail.
- Preserve physical velocity, accepted displacement and previous-centre history
  independently. Exercise sub-grid motion attempts, exact fixed-state consumption,
  collective accepted member motion and later perturbation of sleepers. A quiet
  mask/coordinate alone is not a settling result.
- All-connected-field response, contact invalidation on both index sides,
  sleeping members, focal changes and source edits while ticking. Verify normal
  type-phase delivery; no same-phase early work or lost next-phase activation.
- First use, holes/final records, maximal selected capacities, repeated commits,
  retirement/reuse, ordinary rejection and unexpected failure. Verify survival
  of numerical/source/control/activity/history state at each promised boundary.
- Independent scratch/reference/work/profile limits, prepared descriptor/binding
  stability, zero observed recurring linked C++ allocations and stable known
  native backing/layout/compiler counters. Separate setup/first use/reuse and
  arithmetic work from selection/adjacency/transfer costs.
- Existing legacy suites remain unchanged. Compare equal input laws with an
  independent f64/Q_b reference for the new profile; expected precision/count-
  ratio differences are not hidden behind a demand for legacy bitwise trajectories.

Retain exact source/backend/compiler pins, actual CPU selection, configuration,
raw output, typed inventories and measurement limits. No Python runtime/test
tooling, DB writes or GPU performance claim. Full local GPU qualification remains
after the #156 CPU operation exit, under #145.

## Following live hierarchy contract

After C1 acceptance, propose the next small release under #177 using #185's
validated source-compatible comparisons and the new live typed state. It must
resolve these actual remaining seams before installing a hierarchy:

1. Map comparison identity to published native field records. Candidate contexts
   can share a source field; the live layout must not accidentally alias their
   centres. State the supported subset and reject unsupported mappings.
2. Initialize all exposed centroids once, then read currently published child
   inputs through ordinary native phases. Preserve exact coverage, published
   child coordinates and untrimmed participating masses; no second share,
   recovered tail or internal distance scaling.
3. Prove a native consume/publication/reactivation cut for same-type parents.
   The current `NativeSet` exposes activate/deactivate/is-active and struct
   traversal; it does not establish a frozen work snapshot across kernels.
   A child activating a parent while that parent consumes its bit can lose work.
   Check fork-native facilities first and retain a native collision probe;
   do not assert safety from the existing cross-type particle wake sequence.
4. Specify local/inherited whole-particle response ownership and integrate a
   physical body once. Downward response is still provisional; this plan selects
   no inherited force equation, duplicated force pass or overlap matcher.
5. Qualify multi-depth settling and perturbation/insertion recovery across ticks,
   reporting the fixture's declared criterion, ticks/time/work, oscillation and
   lost-wake checks. The full candidate query may check a settled result, but
   cannot become a mandatory per-tick solver or barrier loop.

These are subsequent release obligations, not unresolved implementation details
to fill in silently during C1. General matching/exposure, production rounding,
frame calibration, operational admission/contact transactions and full GPU
qualification remain open. This plan closes none of #172/#177/#156/#145.
