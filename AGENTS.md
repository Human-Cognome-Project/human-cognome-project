# Agent guidance

Start a fresh environment with the [agent onboarding/spec brief](docs/onboarding/AGENT-START.md).
The [acceptance snapshot](docs/history/agent-acceptance-2026-11-05.md) and
[earlier history](docs/history/acceptance-through-201.md) preserve prior reviews
and source pins; they are not the current task queue.

Read this file in full before coding or reviewing. Start from current `main`
unless a task names another integration branch. Component instructions still apply.
For an introduction, see [orientation](docs/onboarding/README.md); for a local
native check, see [first run](docs/onboarding/FIRST-RUN.md).

## Current state

Last checked: **2026-11-07**, accepted integrated CPU implementation through #227 (reviewed head `b9ea490f86333ba1bd572abd74a202fe19c7534a`). Native run 42252528197 and smoke passed. Next: nested map staging reuse, complete owner peak/lifetime/work accounting and remaining-operation integration, then same-execution controls. C2b-1 is accepted in #192 under the [#191 plan](docs/cumulative-publication-integration-plan.md). The [C2b-2 weighted native probe](engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted in #193 with [source-pinned evidence](engine/tests/weighted_handoff/README.md). It qualifies held-input numerical propagation, not production hierarchy or inherited response.

The [production hierarchy/response plan](docs/production-hierarchy-response-plan.md)
is accepted in #194. It specifies comparison and force ownership, production
reader/history lifetimes, combined integration and coupled recovery. Its C3-1
fixed-tree implementation is accepted in #195; C3-2 coherent refresh is accepted
in #198. B1 contact-safe ticks are accepted in #201; private admission/bootstrap and atomic exchange are accepted through #210/#219.

The [accepted C3-1 implementation](engine/docs/LIVE-HIERARCHY.md) now supplies
that opt-in fixed-tree CPU path. Its executable acceptance now follows the
[coherent-dynamics direction](docs/coherent-dynamics-gpu-snapshots.md); see the separately pinned
[revised observations and original investigation](engine/tests/live_hierarchy/README.md).
The active-capture adapter is accepted. #196 supplies scoped local GTX 1070
evidence; full integrated CUDA qualification remains open.

This is the maintained status entry point. Code and executable evidence determine
what is built; accepted decisions govern intended behavior. A conflict between
them and this summary is drift to investigate, not permission to silently pick
one. Check [open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls)
and latest maintainer dispositions before implementing. Update this section when
a merge or review changes Built/Held/Active status; other entry points link here.
Keep this table and the immediate task current rather than adding another milestone
row for every PR. Put detailed run/commit evidence in the owning acceptance record.
When the next task changes, reconcile the onboarding brief and continuation task
in the same documentation change. Preserve provisional versus merged acceptance.

| State | Current scope and authoritative evidence |
|---|---|
| Accepted data and preparation | Retained [letter/core snapshots](data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md), native [warm tools](kernels/database/warm/README.md), coordinate-free [v2 recipes](kernels/database/warm/STRUCTURAL-RECIPES.md), [projection](engine/docs/LETTER-PROJECTION.md) and prepared native backing/updates. Preserve existing stores; population and operational ownership are separate. |
| Accepted CPU numerical path | Cumulative publication, [accepted motion](engine/docs/ACCEPTED-MOTION.md), [installed hierarchy](engine/docs/LIVE-HIERARCHY.md), coherent [refresh](engine/docs/LIVE-REFRESH.md) and [contact-safe ticks](engine/docs/CONTACT-SAFE-TICKS.md), accepted through #198/#201. Scope-specific GPU evidence does not qualify the complete path. |
| Accepted private operations | Certified placement (#203), ordinary admission/bootstrap (#210), live focus (#211), exact full/reduced f64 rebase (#212/#213), and source-bound mass/momentum/placement accounting with [atomic exchange](engine/docs/ATOMIC-EXCHANGE.md) through #219. These are bounded private primitives; an enduring operational owner/public Add API is not accepted. |
| Latest accepted engine evidence | #227 head `b9ea490f86333ba1bd572abd74a202fe19c7534a`: [native run 42252528197](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42252528197) and smoke 42252525837 passed. Inspected regression logs cover 115 unique tests exactly once; all compiler jobs passed; database records 180 checks, addressed_reads=24 and private_reads=0. [Admission evidence](engine/tests/admission/README.md#maintainer-integrated-acceptance--224227-2026-11-07) preserves distinct local/hosted evidence and cancelled #224 history. |
| Accepted exchange owner | Field lifecycle, persistent study identity and reserved projections through #223, plus #224 retained coordinator command/intrinsic/hierarchy backing, #225 immutable declarations, #226 binding/source buffers and #227 all-alternative membership catalogue with allocation-free source capture after map staging, are integrated. The harness remains borrowed; nested maps, total owner resources and other operations remain open. |
| Next implementation | Start from current main. Complete nested map staging reuse, complete owner peak/lifetime/work accounting and remaining-operation integration, then same-execution controls. See [continuation](docs/core-local-study-continuation-plan.md#next-agent-task--2026-11-05). |
| Accepted tooling | [Local evidence runner](engine/docs/LOCAL-CI.md) (#204), [parallel hosted CI](engine/docs/HOSTED-CI.md) (#206), and optional [offline viewport](tools/viewport/README.md). #220's AJV 8.18.0 fix is merged: hosted 15 model tests, reproducible bundle and 125 browser checks passed. Observation is not engine control or proof of natural clustering. |
| Outstanding qualification | #156 owns integrated operation acceptance; #145 owns actual GPU, transfer/pinning and resource qualification. Scoped [GTX 1070 evidence](engine/docs/CUDA-LOCAL-VALIDATION.md) is not blanket GPU acceptance. Mixed-precision repair, complete operation/hardware matrix and [Docker release](docs/onboarding/AGENT-START.md#intended-local-study-and-release) remain separately scoped. |
| Held/deferred | #111 and #123 remain held; draft #75 remains deferred. They do not block separately accepted core work. Analyst reasoning, dynamic LoD selection and process-failure hosting remain unbuilt. |

Detailed milestone pins and their historical limits are preserved in the
[2026-11-05 acceptance snapshot](docs/history/agent-acceptance-2026-11-05.md)
and [earlier history](docs/history/acceptance-through-201.md).
Do not copy historical “next” instructions back into this table.

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>

Private source bindings, atomic exchange, field lifecycle, persistent exchange owner
and scoped preparation reuse are accepted through #227. Continue nested map staging reuse, complete owner peak/lifetime/work accounting and remaining-operation integration, then same-execution controls. Do not duplicate the accepted work.

## Latest model direction — 2026-10-23

The directions below have bounded CPU implementations through #201. Read
profile-specific statements with the [accepted motion rule](engine/docs/ACCEPTED-MOTION.md)
and current state above; they do not reopen completed prerequisites.

See [precision, exposure and concept catalogue](docs/precision-exposure-and-concept-catalogue.md).
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

The [cumulative significand clarification](docs/precision-exposure-and-concept-catalogue.md#cumulative-significand-clarification--2026-10-26)
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
[C2b-1 correction](engine/docs/CUMULATIVE-PUBLICATION.md) is accepted in #192;
C1 descriptions and archived evidence remain records of their pinned behavior.
C1 already combines centroid
inputs before publication; audit the actual gating seam rather than assuming
each member is independently trimmed by that reduction. Require cumulative
threshold-crossing, cancellation and multilevel no-lost-input tests. Integrate
with C2a's publication/consume contract without a same-tick global solve.

## Active and deferred work

**Immediate work:** continue from current main through accepted #227. Complete
nested map staging reuse, complete owner peak/lifetime/work accounting and remaining-operation integration, then same-execution controls. Reuse the accepted private owner and
preparation components under the [continuation plan](docs/core-local-study-continuation-plan.md).

- Continue #156 through the accepted #170 stages. #162's bounded foundation and
  #139's original A–D sequence are complete; their closures do not close #156/#145. Held #123 must not
  defer required preservation of existing live state during insertion.
- Use retained letters as current test objects. Broader linguistic coverage and
  word-level seeding require their own accepted data contracts; do not infer
  production allocations or new writes from an onboarding summary.
- #134 tracks exact range-compressed reverse child navigation; it is not built
  and does not change the accepted row-based seed writer. Common words retain
  complete cold references even when traversal value is low.
- #138 tracks endpoint subclassification and digit/operator placement separately.
- Dynamic LoD selection, slopes/transitions, initial test granularities and
  private/temp root subdivisions await discussion. Do not invent policies.

<a id="placement-correction--2026-10-20"></a>
<a id="allocation-foundation-correction--2026-10-20"></a>

## Required placement and allocation behavior

These requirements supersede caller-placement and fixed/equal-count assumptions
in accepted A–D work as well as proposals. Private CPU primitives implement the
bounded placement/publication path; enduring owner integration, full resource
accounting and integrated hardware qualification remain outstanding.
The [#156 plan](docs/pooled-space-live-insertion-plan.md) owns the correction;
the [#163 plan](docs/prepared-capacity-foundation-plan.md) and
[#165 section](docs/prepared-execution-backing-plan.md) own bounded preparation.

- New objects enter at the nearest unoccupied space to the current centre of
  focus, respecting extents and sleeping occupants. Subsequent motion follows
  mass and fields. Do not substitute random starts, a lattice or semantic placement.
- Structural definitions/warm recipes must not require initial positions or
  velocities. Current mutable state belongs to Taichi under engine protocols;
  staging/readback/report copies are not independent state authorities.
- Optional restart positions are a calculation convenience, separate from
  immutable recipes and fixture starts. Normal refresh must not reset survivors.
  This exception does not require a general checkpoint subsystem.
- Ordinary streaming claims prepared capacity without new backing allocation,
  layout rebuilding or compilation per arrival. Prepare particle, field,
  relationship, contact, activity, adjacency and reporting capacity together.
  Exhaustion/expansion is a separate resource operation.
- Potential capacity, present objects and awake calculation work are distinct.
  Unused `0x` declarations have no mass/presence/fields and need no populated
  payload per entry. Sleeping objects retain numerical state, mass and membership.
- The SNode structure under `ti.root` supplies available space; live configuration
  expresses representation. Slot topology does not prescribe physical coordinates.
  Preserve LoD changes within capacity, distinguishing primary scale from 1:1
  compression. Reuse data while hot; avoid allocation clerical work in streams.
- Native sparse address capacity does not prove backing is provisioned. Measure
  setup, first touch and reuse separately. Acceptance must exercise unused blocks,
  repeated insertion while ticking, survivor state, and separate exhaustion on CPU
  and local GPU. Removing supplied coordinates alone does not complete #156.
- One assembled definition may have many simultaneous live occurrences. Each has
  its own handle/state and contributes its own mass and participation. Deduplicate
  membership paths per target field **and occurrence**, not across copies.
  Inserting, changing or retiring one copy must preserve the others; see
  [#162 acceptance clarification](https://github.com/Human-Cognome-Project/human-cognome-project/issues/162#issuecomment-6340088128).
- #170's review accepts the bounded D1–D6 bootstrap, motion, batch, geometry,
  restart and focus/rebase conventions in the [remaining plan](docs/core-operation-readiness-plan.md#decision-status).
  Patrick confirmed D7: expansion/compression targets current position or nearest
  unoccupied; nearest-to-focus applies to genuinely new placements. The follow-up
  review accepts the complete contract, including staged contact validation before
  numerical publication. Keep these accepted conventions separate from built
  capability; source v2 support does not implement native admission or ticks.

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
The [active field model](engine/docs/ACTIVE-FIELD-MODEL.md) owns exact predicates.

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

## Addressing

`00` is common computational core; `02` is shared multilingual Linguistics;
`02.FS.LR.00.*` is reserved for language-frame word components. The old English
`01` / common `01.00` layout is superseded; no new meaning is assigned to `01`.
The #132 seed map is accepted; further allocations require review.
`y*` personal and `z*` temporary are first-symbol prefix families, not stored
wildcards or concrete object allocations. Concrete bindings need authorization;
internal subdivision, delegation, lifetime and expansion blocks remain deferred.

## Architecture invariants

- The modified Taichi **runtime/compiler substrate**, HCP **physics engine**, and HCP **engine harness** are distinct concepts.
- The canonical engine path is native **C++**. Taichi's Python frontend is upstream/vendor tooling, not the HCP runtime surface.
- Project Python is authorized only for bulk database population scripts during bootstrap. Their source preparation and validation stay within that population workflow. Offline operation alone does not authorize Python. Do not implement reusable selection/assembly components, physics, database/WAL kernels, endpoint/network infrastructure, or analyst behavior in Python.
- The **engine harness is the control interface for the physics engine**. It is not the DB manager, WAL manager, messaging system, topology system or thread manager.
- **Analyst functions do not exist yet.** Do not invent an analyst architecture or implement speculative analyst behaviour.
- Warm is the shared tool/part box; hot is the analyst-owned host assembly workbench; GPU memory is the execution surface. Private/local inputs enter only through the analyst-owned active input boundary. The cache manager must not access private DB codes/data, including for content inspection or filtering.
- Database/cache/record/WAL kernels exist to support the future analyst and keep its working surfaces current.
- Kernel components communicate through active inbox/outbox endpoints and should remain location-blind: local versus remote placement is a topology/bridge concern.
- The future configuration routine resolves endpoint paths; the future thread manager handles lower-frequency/system-facing bridge activation.
- `kernels/database/`, `kernels/wal/`, and `network/endpoint/` are peers. Do not collapse WAL or shared endpoint infrastructure back under the database/cache hierarchy.

## Do not reintroduce

- Do not infer conceptual scale from raw SNode depth or secondary compression nesting.
- Do not implement the superseded English `01` / per-language database layout.
- Do not substitute a shared core character for a language-letter endpoint: the linguistic endpoint has its own identity and ordered core parents.
- Do not materialize reverse DB child-reference listings as live particles. They support navigation/assembly; ordered parent occurrences and separately prepared fields determine live participation.
- Do not extend Python beyond early bulk database bootstrap population and its preparation/validation. Offline operation alone is no exception; operational selection and assembly are native.
- Do not invent analyst behaviour or treat held #123 proposals as an accepted contract.
- Do not restore universal fixed diameter, beads-on-string geometry, positional parent offsets or rotation.
- Do not reset/drop persistent databases as routine validation.

<a id="current-model-realignment-2026-10-09"></a>

## Model contract

Follow the [conversion and continuation plan](docs/model-realignment-plan.md)
for sequencing, dependencies and verification. Its dated decisions supersede
conflicting historical model requirements; code still determines what is built.

- Primary SNode structure expresses conceptual LoD. Attached secondary
  compression structures inherit that scale at **1:1**, including nested
  dedupe; storage depth alone never creates a conceptual scale step.
- Component-derived spherical extent replaces universal fixed diameter.
  Supplied-layout enclosure is built; packing and broader extent questions remain open (#120).
- Parent response is proportional and whole-particle. No internal application
  offsets, rotation, rotary alignment or beads-on-string requirement.
- Corona-disabled variable-radius contact is built in the bounded object-support
  slice. Scaled attraction supplies intended cohesion; broader calibration
  remains empirical.
- Whitespace is recorded as positional gaps; optional model separators have
  only local connecting participation, no structural relationships or gathering
  mass, and no field with other whitespace occurrences.
- Track bridge/identity in #106, distance calibration in #115, whitespace in
  #116, response in #118, contact in #119 and extent in #120.
- Dictionary structures and SNode assembly are the near-term objective.
  Follow the [language component plan](docs/language-component-unicode-plan.md):
  `00` is the common computational core; `02` is the shared multilingual
  Linguistics base, with `02.FS.LR.00.*` reserved for language-frame word
  components. Patrick confirmed this supersedes English `01` / common `01.00`
  on 2026-10-11. No new meaning for `01` is assigned here. Formal cross-store
  writes still require the plan's bounded reference/write implementation.
- Linguistic aggregation uses reciprocal `members` / `member_of` links.
  Shared core character values retain one identity. Each language's letter or
  punctuation endpoint has its own linguistic identity, with ordered core
  character parents and matching additions to those core endpoints' child
  listings (Patrick clarified 2026-10-12). The linguistic endpoints join their
  language and alphabet/punctuation groupings. English variants belong to English, which
  belongs to its broader groups. Each grouping has its own centroid
  representation; alphabet and punctuation groups are distinct. Literal
  `parent` / `child` links describe construction only. Do not duplicate the
  underlying core character values or encode group containment as parents.
  Existing record-integrity findings are not cleared by
  documentation merges; check current PR/issue state before execution.

## Reading paths

The [orientation role table](docs/onboarding/README.md#reading-paths) is the
canonical reading map. Agents read this file in full, then the owning component
README, accepted plan and tests. Read the [Covenant](covenant.md) and
[Charter](charter.md) before contributing. Research and recovered history are
references for relevant tasks, not a prerequisite for a first native check.

<a id="current-development-state"></a>

## Local evidence and provisional continuation — 2026-11-02

Patrick confirmed that contributions from the app on his local machine will
include local CI evidence for provisional maintainer acceptance and continued
work. Review the source and evidence appropriate to the changed scope; do not
hold otherwise accepted follow-on work solely for hosted CI latency.

Record the tested commit, actual backend, configuration, commands, results,
retained evidence and the profiles not run. A focused local pass supports only
its tested scope. Distinguish contributor-reported results from raw evidence
personally inspected, and local execution from hosted execution. Machine-local
hashes identify evidence but do not make the raw bundle remotely available.

Hosted CI remains authoritative for the checks it can perform. Provisional
acceptance is permission to continue the reviewed scope, not a claim that
pending hosted checks passed or that untested behavior is qualified. Reconcile
later hosted findings before final acceptance/merge; do not duplicate a running
suite merely to permit local progress. Keep dependent work in reviewable
commits or stacked PRs, with its provisional dependencies explicit. GPU behavior
requires actual local hardware evidence and is not inferred from hosted CPU CI.

## Work discipline

- Code and executable tests determine what is built. Design notes determine intent only where they explicitly say a decision is settled.
- Preserve distinctions such as **BUILT**, **planned**, **deferred**, and **experimental**. Do not promote a plan to implementation by paraphrase.
- When moving kernel families, migrate their include paths, build instructions and tests together. Preserve working interfaces before refactoring behaviour.
- Preserve the native engine boundary under `engine/src/`: `engine_support` wraps the Taichi runtime/compiler substrate; `field_core` owns field/tick mechanics; the wider analyst-facing harness remains a distinct control layer. Do not promote the archived Python prototype back into the runtime path.
- Never force-push or rewrite shared history for cleanup. Normal commits, merges and moves keep prior versions recoverable.
- Do not add tests or utilities that reset, drop, or truncate a database or schema as part of routine execution. The core snapshot is retained and may grow; validate it read-only unless a specific data change is authorized.
- Keep credentials and private data out of Git. PostgreSQL reproducibility exports belong under `data/postgres/snapshots/` and use Git LFS for compressed dumps.
- Add or preserve tests for behavioural changes. Large generated run artifacts should not be treated as source code merely because an experiment produced them.

## Contribution flow

Use focused branches and reviewable commits. For structural changes, prefer small commits that do one of:

- history-preserving integration;
- pure structural move;
- documentation correction;
- behaviour refactor;
- test/build repair.

Do not combine all of those in a single cleanup commit.

