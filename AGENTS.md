# Agent guidance

Start a fresh environment with the [agent onboarding/spec brief](docs/onboarding/AGENT-START.md).
The [dated acceptance history](docs/history/acceptance-through-201.md) preserves
prior reviews and source pins; it is not the current task queue.

Read this file in full before coding or reviewing. Start from current `main`
unless a task names another integration branch. Component instructions still apply.
For an introduction, see [orientation](docs/onboarding/README.md); for a local
native check, see [first run](docs/onboarding/FIRST-RUN.md).

## Current state

Last checked: **2026-11-01**, main `939632dedb9df0234d7e828e5c0555313ee14cec` (#201). C2b-1 is accepted in #192 under the [#191 plan](docs/cumulative-publication-integration-plan.md). The [C2b-2 weighted native probe](engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted in #193 with [source-pinned evidence](engine/tests/weighted_handoff/README.md). It qualifies held-input numerical propagation, not production hierarchy or inherited response.

The [production hierarchy/response plan](docs/production-hierarchy-response-plan.md)
is accepted in #194. It specifies comparison and force ownership, production
reader/history lifetimes, combined integration and coupled recovery. Its C3-1
fixed-tree implementation is accepted in #195; C3-2 coherent refresh is accepted
in #198. B1 contact-safe ticks are accepted in #201; native admission follows.

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

| State | Scope, limits and owning evidence |
|---|---|
| Retained cold data | Full valid-scalar [UTF-8 snapshot](data/postgres/snapshots/2026-10-03-encoding-tables/MANIFEST.md); [paired letter exports](data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md) from #136 retain 2,772 Linguistics tokens and 2,741 additive core child links. Population is separate from installation into user stores and from held #111. [Writer contract](docs/letter-population-writer.md). |
| Built bounded A–D path | Native PostgreSQL [warm tools](kernels/database/warm/README.md), [letter/byte projection](engine/docs/LETTER-PROJECTION.md), [membership fields](engine/docs/MEMBERSHIP-FIELDS.md) and [active workbench](engine/docs/ACTIVE-WORKBENCH.md), accepted through #141/#143/#146/#149 under the now-completed #139. Supplied-state fixtures and controlled restart are built; production engine-owned placement/live insertion are not. |
| Built field mechanics | [Object support](engine/docs/OBJECT-SUPPORT-RESULTS.md), [effective-motion evidence](engine/docs/EFFECTIVE-MOTION.md) and [type-phase evidence](engine/tests/type_phase/acceptance.json), through #150/#153. Whole-particle response, variable-radius contact, local coordinates and native binary activity have bounded CPU evidence. #91 retains formula/resolution/readback follow-ups. |
| Built prepared backing | #161 native storage investigation, #164 [record mapping](engine/docs/RECORD-MAPPING.md), and #166 [combined backing/bindings](engine/docs/PREPARED-EXECUTION.md) are merged. #166 passed all eight hosted checks, including 37 CPU suites. Empty prepared storage, first nonempty fixtures and 32 ticks retain measured backing with zero observed recurring C++ allocations. This is not general live insertion or complete host/driver allocation accounting. |
| Built bounded updates | #169 implements the accepted [#167 coherent-update plan](docs/prepared-coherent-update-plan.md): [implementation and evidence](engine/docs/PREPARED-UPDATES.md) cover repeated structural commits, independent occurrences, survivor/history preservation, wake effects and rejection/failure rules. All eight hosted checks and 43 CPU suites passed on the reviewed source. This remains a numerical-fixture foundation. |
| Built combined hierarchy refresh | #198's [C3-2 implementation](engine/docs/LIVE-REFRESH.md) commits one prospective source/hierarchy generation, preserving survivor state, dirty history and pending work. All nine hosted checks passed, including 101 CPU suites, both extra no-unroll matrices and 180 integration checks. New-body starts remain private fixtures; #201 adds contact-safe CPU ticks, while operational admission remains pending. |
| Accepted B1 selection gate | #200's [native workspace](engine/docs/NATIVE-SELECTION.md) qualifies scratch selections with the actual C3 builders under accepted #199. [CPU evidence](engine/tests/native_selection/v1/README.md) is source-pinned. At that merge the complete tick API remained unbuilt; #201 now supplies the accepted CPU tick API. Admission remains separate. |
| Accepted CPU B1 implementation | #201 adds the opt-in [contact-safe tick](engine/docs/CONTACT-SAFE-TICKS.md) using prepared fixture inputs. [Local CPU qualification](engine/tests/contact_ticks/v1/README.md) passes 109 suites and all eight modes; hosted validation independently passes 109 suites and all eight checks. Placement, admission and enduring owners remain unbuilt. |
| Active follow-up | #162's bounded storage/update foundation is complete and closed; #156 owns remaining implementation and integrated CPU acceptance. The [#170 plan](docs/core-operation-readiness-plan.md), D1–D7 and the contact publication contract are accepted. [Segment 1](kernels/database/warm/STRUCTURAL-RECIPES.md) merged in #171: coordinate-free v2 values/storage/projection and retained-v1 compatibility; all nine hosted checks passed, including 44 native CPU suites. #175 supplies accepted [native geometry preflight](engine/docs/PREPARED-GEOMETRY.md), a partial Segment 2 query with all eight hosted checks and 52 CPU suites passing. #201's CPU candidate ticks are accepted. Engine-owned placement/admission, enduring owner lifecycle and the integrated CPU exit remain unbuilt. General LoD selection remains deferred. |
| Built optional tooling | #155 completed #151's [read-only viewport](tools/viewport/README.md); #158/#159 added capture controls, actual-backend checks and folder watching. #157 separated database names from snapshot names. Viewer/capture fixtures do not prove natural clustering or settling; the viewer is removable. |
| Local qualification | The release intent is the [bounded Docker/letter study](docs/onboarding/AGENT-START.md#intended-local-study-and-release). Full operational GPU qualification remains distinct; see the [accepted plan](docs/core-operation-readiness-plan.md). #145 owns actual GPU execution, allocation/transfer and pinning qualification. Earlier bounded CPU prerequisites are satisfied. [Bounded local observations](engine/docs/CUDA-LOCAL-VALIDATION.md) are not blanket GPU qualification; ordinary host vectors are not pinned. Native browser folder-picker interaction remains a local verification item. |
| Held | #123 general view/identity proposal and #111 command atomicity/restart work have unresolved findings. Neither is an implementation contract or a gate on separately accepted bounded work. |
| Deferred | #75 historical read-only monitor; reconcile its all-edge baseline with current reporting before revival. It is not a viewport/GPU prerequisite. General analyst, dynamic view selection/recomposition and process-failure hosting remain unbuilt. |

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>

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
governs the following integration plan under #172/#177. Accumulate the tick's
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
C1 descriptions and evidence below remain records of their pinned behavior.
C1 already combines centroid
inputs before publication; audit the actual gating seam rather than assuming
each member is independently trimmed by that reduction. Require cumulative
threshold-crossing, cancellation and multilevel no-lost-input tests. Integrate
with C2a's publication/consume contract without a same-tick global solve.

## Active and deferred work

**Immediate work:** [draft #202](https://github.com/Human-Cognome-Project/human-cognome-project/pull/202)
contains B2 placement at `1214e9a0265eb693e4d605add7c496f16e124d36` as checked
2026-11-01. The contributor reports full qualification outstanding. Inspect its
latest source, comments and checks before continuing; this is not acceptance.
B1 is complete in #201. Follow B2 with B3 admission, enduring owners and
same-execution controls under the [continuation plan](docs/core-local-study-continuation-plan.md).

The stage descriptions below preserve the scope at each acceptance; “following”
and “not released” in those descriptions are not current holds on #194–#201.
Use the current-state table and continuation for today's implementation queue.

- #176 accepts the [native precision prerequisite plan](docs/native-precision-truncation-plan.md):
  qualify f64 transport and an experimental significand-clearing primitive,
  then audit the later live conversion. Production rounding remains open.
  #172 and #177 govern the eventual precision/LoD-centroid integration;
  accepting this plan does not implement that integration.
  The [first native implementation](engine/docs/NATIVE-PRECISION.md) was accepted
  in #178: f64 packed transport, experimental bit clearing and a source state
  audit; all eight hosted checks and 54 CPU suites passed. It leaves live tick behavior unchanged and explicitly rejects
  narrowing values in the fork's unsupported FTZ subnormal/underflow range.
- The [upward centroid integration plan](docs/upward-centroid-integration-plan.md)
  is accepted in #180 under #172/#177. The [candidate-only implementation](engine/docs/CENTROID-CASCADE.md)
  is accepted in #182: current backend f64 hierarchy evaluation, with all eight
  hosted checks and 62 CPU suites passing. This explicit query adds no internal
  distance multiplier and is not a mandatory per-tick solve. Live precision,
  forces, wakes and settling-cycle integration remain outstanding.
- The [weighted comparison accounting plan](docs/weighted-centroid-accounting-plan.md)
  is accepted in #183 under #177/#172. [Segment A](engine/docs/SOURCE-COUNTS.md)
  is accepted in #184: retain exact source counts through prepared commits,
  without changing legacy physics; all eight hosted checks and 66 native CPU suites passed on its
  [pinned source](engine/tests/source_counts/README.md). Segment B's source-compatible fractional/
  overlapping [candidate implementation](engine/docs/WEIGHTED-CENTROID-CASCADE.md)
  is accepted in #185. All eight hosted checks passed; the inspected native log
  confirms 74/74 CPU suites on reviewed `dce69ad`. Its
  [contributor evidence](engine/tests/weighted_cascade/README.md) remains separately
  source-pinned. It retains the original whole-body API and changes no live mechanics.
- The [live numerical profile plan](docs/live-numerical-profile-plan.md) is
  accepted in #186 under #172/#177: a coherent f64 trial profile, separate physical/history
  state and discarded positional detail, with the existing single-level native
  phases. The [C1 implementation](engine/docs/LIVE-NUMERICAL-PROFILE.md) is accepted in
  #187 with [source-pinned CPU evidence](engine/tests/numerical_profile/README.md).
  All nine hosted checks passed on reviewed `fd3c520`; the inspected native log
  confirms 82/82 CPU suites in 685.51 seconds. Contributor evidence is separately pinned.
  Default legacy arithmetic remains unchanged; trial f64 covers live state,
  repair, geometry, optional queries and typed readback. Its provisional persistent
  sub-grid activity is historical evidence; the C2b-1 correction below is accepted in #192.
  Live hierarchy consume/publication cuts and inherited response remain a
  following bounded contract, with #181's settling-across-ticks direction intact.
- The [live hierarchy handoff plan](docs/live-hierarchy-handoff-plan.md)
  follows #187 under #177/#172. It is **accepted in #188**, releasing C2a's native
  same-type collision/publication investigation and probe only. Its
  [diagnostic implementation and findings](engine/docs/NATIVE-HANDOFF.md) are accepted in #189 with [CPU evidence](engine/tests/native_handoff/README.md): unsafe-order
  controls and a CPU-qualified native protocol with one activity set and two
  numerical publication slots. Production runtime and fork are unchanged. No hierarchy installation or
  inherited-response implementation is released. Comparison identity, a proven
  native handoff, explicit response ownership and coupled settling/recovery
  remain the following integration contract.
  The accepted #191 [cumulative publication/activity plan](docs/cumulative-publication-integration-plan.md)
  combines these findings with #190: first correct existing single-level trial
  activity and final-speed publication, then qualify real weighted multilevel
  arithmetic in a separate native probe. Neither slice installs production
  hierarchy or inherited response. The [C2b-1 implementation](engine/docs/CUMULATIVE-PUBLICATION.md)
  and [source-pinned CPU evidence](engine/tests/cumulative_publication/README.md)
  are accepted in #192: contributor regression passed 88/88 suites in 892.49 seconds
  on `8db7289`. Hosted acceptance is recorded in the [acceptance history](docs/history/acceptance-through-201.md). The
  [C2b-2 implementation](engine/docs/WEIGHTED-NATIVE-HANDOFF.md) and
  [CPU evidence](engine/tests/weighted_handoff/README.md) are accepted in #193:
  weighted published-child arithmetic, per-path compiler audit, packet samples,
  native convergence/recovery and prepared reuse. Contributor evidence is 92/92
  CPU suites in 930.44 seconds on `bb5ba2e`, with 29,184 held-input transitions
  resolving in 1–5 polls. Production hierarchy, response
  ownership and coupled physical settling remain the following contract.
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
in accepted A–D work as well as proposals. They are **not all implemented**.
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
For the new precision path, reconcile carried rounding-residue requirements
through #172; the historical contract below is evidence of built behavior, not
a veto on the new direction. Keep pre-integration contact forces and sleeping-target checks on both index sides
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

