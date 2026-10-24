# Agent guidance

Read this file in full before coding or reviewing. Start from current `main`
unless a task names another integration branch. Component instructions still apply.
For an introduction, see [orientation](docs/onboarding/README.md); for a local
native check, see [first run](docs/onboarding/FIRST-RUN.md).

## Current state

Last checked: **2026-10-23**, after #174 documentation integration and #175 geometry acceptance.

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
| Active follow-up | #162's bounded storage/update foundation is complete and closed; #156 owns remaining implementation and integrated CPU acceptance. The [#170 plan](docs/core-operation-readiness-plan.md), D1–D7 and the contact publication contract are accepted. [Segment 1](kernels/database/warm/STRUCTURAL-RECIPES.md) merged in #171: coordinate-free v2 values/storage/projection and retained-v1 compatibility; all nine hosted checks passed, including 44 native CPU suites. #175 supplies accepted [native geometry preflight](engine/docs/PREPARED-GEOMETRY.md), a partial Segment 2 query with all eight hosted checks and 52 CPU suites passing; it does not install contacts or advance transactional ticks. Engine-owned admission, candidate ticks, enduring owner lifecycle and the integrated CPU exit remain unbuilt. General LoD selection remains deferred. |
| Built optional tooling | #155 completed #151's [read-only viewport](tools/viewport/README.md); #158/#159 added capture controls, actual-backend checks and folder watching. #157 separated database names from snapshot names. Viewer/capture fixtures do not prove natural clustering or settling; the viewer is removable. |
| Local qualification | Patrick deferred full GPU testing until the remaining core operation matrix passes CPU acceptance; see the [accepted plan](docs/core-operation-readiness-plan.md). #145 owns actual GPU execution, allocation/transfer and pinning qualification. Earlier bounded CPU prerequisites are satisfied. [Bounded local observations](engine/docs/CUDA-LOCAL-VALIDATION.md) are not blanket GPU qualification; ordinary host vectors are not pinned. Native browser folder-picker interaction remains a local verification item. |
| Held | #123 general view/identity proposal and #111 command atomicity/restart work have unresolved findings. Neither is an implementation contract or a gate on separately accepted bounded work. |
| Deferred | #75 historical read-only monitor; reconcile its all-edge baseline with current reporting before revival. It is not a viewport/GPU prerequisite. General analyst, dynamic view selection/recomposition and process-failure hosting remain unbuilt. |

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>

## Latest model direction — 2026-10-23

See [precision, exposure and concept catalogue](docs/precision-exposure-and-concept-catalogue.md).
#172 owns the requested math-core work: higher-precision combined calculation,
configurable significand trimming of stored positions, and discard of subthreshold
positional rounding residuals. Retain as much precision as viable; f64 calculation
may retain an f64 storage container with trimmed terminal bits. Do not default to
f64→f32 or any fixed per-LoD reduction ladder. This supersedes indefinite residual carry as intended policy for that
new path; the built arithmetic remains unchanged until reviewed conversion.
Do not confuse rounding residue with physical velocity or centroid mass accounting.
#115 records global/per-object exposure controls, separate from awake/sleep and
browser visibility; visual parity with analytical needs is unproven and these
controls do not add a GPU gate. #173 owns concept.db as the cross-media centroid
catalogue and recipe/download root, with schema/addresses still undecided.

#177 owns the new LoD-local centroid cascade: resolve finest-exposed local fields,
aggregate through successive exposed comparisons to the coarsest level, with
provisional downward whole-object response. Exact matching/propagation/type-phase
ordering remains to be planned. #176's precision primitives are separable;
later live centroid conversion must reconcile #177 and #172.

## Active and deferred work

- #176 accepts the [native precision prerequisite plan](docs/native-precision-truncation-plan.md):
  qualify f64 transport and an experimental significand-clearing primitive,
  then audit the later live conversion. Production rounding remains open.
  #172 and #177 govern the eventual precision/LoD-centroid integration;
  accepting this plan does not implement that integration.


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
