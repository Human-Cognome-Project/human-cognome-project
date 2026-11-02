# Native admission and contact-safe tick publication

Status: **plan accepted in #199, 2026-10-30; CPU B1 accepted in #200/#201**.
B2 is in [draft #202](https://github.com/Human-Cognome-Project/human-cognome-project/pull/202)
as checked 2026-11-01; qualification/review remains pending. B3 is not yet built.
The source-audit and imperative sections below specify the original plan; they
do not instruct a new agent to repeat accepted B1. This develops
slice B of the [core continuation](core-local-study-continuation-plan.md#b-native-admission-and-contact-safe-ticks)
under the accepted [#170 operation contract](core-operation-readiness-plan.md).
It specifies implementation portions of the released work; it changes no engine
behavior. Owners remain #156, #172/#177 and, for subsequent hardware evidence,
#145. Individual retained letters remain the test objects.

## 1. Accepted starting point

Baseline: main `5ea9c7674f87ef01d5cad3024785b574bc79a75f`, the merge of
#198. C3-2 is accepted for CPU combined structural/hierarchy refresh, reviewed at
`f4c5ba9`, with executable workflow source `904f2bc`. Independently inspected
[hosted job 124295521781](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41123391538/job/124295521781)
completed all its steps: 101/101 tests in 3355.27 seconds, four additional
no-unroll hierarchy modes and four refresh modes, compiler collection/audit,
180 PostgreSQL-to-active checks with zero private reads, and CPU-only backend
verification. All nine checks passed. The [retained record](../engine/tests/live_hierarchy/refresh-v1/README.md)
keeps the earlier initialization failure and job-budget cancellation separately.
No additional baseline native run is requested for this proposal.

#198 preserves survivor state, unchanged dirty packet/history lifetimes and
pending work, validates one prospective source ledger, and uses native claims
and one publication boundary. Its new-body starts remain private fixtures.
#175's current geometry query reads resident occupants/focus and constructs a
candidate contact union; it neither places bodies nor makes a moving tick safe.

Preserve #196's accepted wide-profile rule: retain accepted displacement/dt for
positive dt, and incoming velocity for zero dt. Preserve force/contact equations,
ordered cumulative publication, calibrated significand clearing and native
type-phase timing. Continued motion is an observation, not a new settling gate.
No mixed-precision repair or CUDA acceptance is included here.

## 2. Source audit and native facilities

| Existing surface | Reuse and required extension |
|---|---|
| `field.cpp::tick_values` | Currently begins activity, clears forces, integrates and polls directly into live state. Factor its numerical builders and bindings into explicit execution surfaces before enabling a candidate tick. A geometry query after this function cannot supply ordinary unchanged rejection. |
| `activity.cpp::{begin,touch_contacts,mark,touch}` | Begin changes focal/motion history; integration consumes particle bits; subsequent contact/centroid phases create later work. Stage every one of these effects, including parameter invalidation. |
| `native_set.h`, fork IR/runtime | Existing `create_struct_for`, `is_active`, native activation/deactivation and native list generation supply selection. Pointer ancestors must be activated explicitly and provisioned in advance. There is no audited ready-made transaction/clone API to assume. |
| `prepared_geometry.cpp`, `contact_math.h` | Reuse source-pair deduplication, canonical derived ordering, both CSR directions and the existing contact predicate. Refactor private numerical views so discovery can read either current or candidate positions. The public observation query keeps its own tokens/lifetime. |
| `live_hierarchy.cpp` and `live_hierarchy_refresh.inc` | Reuse the installed force/poll arithmetic, packet protocol and combined source/repair commit. Factor builder targets; preserve selectors, previous packets and unchanged dirty subtrees. |
| `prepared_update.cpp`, `prepared_storage.*`, `PreparedRecordMap` | Reuse structural preflight, exact provenance, provisional references, native claims and late-failure disposition. Add complete bootstrap input and generated numerical initialization without exposing the fixture start supplier. |

The first implementation must qualify the native selection mechanism described
below before routing a production tick through it. It must inspect actual fork
IR/runtime functions and emitted modules; a proposed wrapper name is not evidence
that a native facility exists. No second Program, host numerical replay, custom
work queue or potential-address scan is part of this proposal.

## 3. Candidate tick and publication boundary

Add an opt-in, typed contact-safe tick operation for prepared f64 trial executions,
including the installed hierarchy. The provisional result carries status, named
failing reservation, attempted work/demand and completed generation/tick identity.
The last successful `Demand` stays available separately on rejection. Legacy
fixture execution retains its existing entry points and regression behavior.
Operational admission requires this capability; its execution must refuse the
direct live tick entry that would bypass candidate contact validation.

Under exclusive ownership, perform these phases:

1. Validate readiness, parameters, generations and all reservation bounds. Read
   current backend state and validate the current explicit/derived contact union
   before evaluating forces. Sleeping physical bodies participate in discovery.
   Explicit source rows retain their identities, ordinals and original ordering.
   Stage any current-coverage/input invalidation before selecting candidate phase
   work, while leaving committed bits unchanged.
2. Initialize prepared candidate numerical/control surfaces from committed state
   through native copies. Run the existing begin, clear, force, contact,
   integration/determiner, contact propagation and hierarchy poll sequence against
   these surfaces. Compile the shared numerical builders for these targets once
   during preparation. Retain one physical integration and the current per-type
   publication/consume ordering; do not implement a second set of equations.
3. Discover the prospective explicit/derived union from the actual candidate
   **published** positions. Include stationary/sleeping peers. Check exact pair
   demand, source mappings, both CSR directions, ordering/compaction work,
   reference peaks and candidate numerical status before committed writes.
   Post-integration discoveries affect the following particle phase. Changed
   coverage conservatively invalidates the affected endpoints for that phase.
4. After successful completion/status synchronization, copy the candidate
   numerical/control outputs, complete contact/CSR surface and ordered native
   activity effects. Finish publication before exposing a report or permitting
   any tick, query, refresh or capture. Update previous parameters, invalidation
   state, timings and completed-tick count only at successful completion.

The initial correctness baseline may perform bounded present-body pair discovery
at both geometry boundaries. Record its cost separately from awake numerical
work; use native presence rather than potential V. Reuse unchanged coverage only
with an explicit proof covering both candidate motion and retained explicit rows.
An optimization cannot silently skip source-empty discovery or sleeping targets.
Attempt-local geometry uses private workspace and preserves published query and
capture tokens until successful publication.

Keep the explicit source contact ledger separate from the physical union. Unique
explicit pairs lead in source order/orientation; derived-only pairs follow in
canonical record order, as in #175. Removing derived coverage never removes an
explicit request. If installed union rows/mappings change, advance the prepared
generation and installed hierarchy/source-reference stamps at the same completed
boundary. Preserve numerical packets/history while updating stamps. Stable native
handles remain stable. An unchanged union retains generation; every accepted
tick advances completed-tick observation. Stale row/query/capture references must
reject coherently. No generation counter selects work.

### Native activity during an attempt

The proposed mechanism uses prepared, transaction-local native bitmasked selection
workspace and bounded consume/wake intents. Populate it by native traversal of
the committed sets; run candidate phases through native `struct_for`, activation
and deactivation against that workspace. Only the committed particle/centroid
sets remain authoritative across completed boundaries. Clear scratch membership
before each attempt; an abandoned attempt contributes no committed wake or consume.
These are selection metadata, not another population of physical particles.

Qualify this mechanism explicitly against the one-persistent-set-per-type
contract. A flat P/G loop testing staged awake flags is not a substitute for
native active traversal. Whole-view invalidation and bounded numerical copying
remain separately charged operations. Warm every pointer block and native list
high-water before readiness; copying opaque NodeManager/runtime memory is not an
assumed native cloning operation.

The qualification must preserve the particle calculation cut, contact propagation
after physical calculation, and centroid notifications pending for the next
particle phase. A newly notified sleeping particle must not calculate early or
propagate through a phase it did not calculate. Native container lists are not
frozen snapshots of individual bits: retain the [accepted handoff](../engine/docs/NATIVE-HANDOFF.md)
allowing a newly admitted centroid to perform its full consume/read/calculate/
publish operation in the same poll, with at most one visit and later notifications
remaining pending. Preserve current pending bits and dirty packet lag. If this native
workspace cannot satisfy the contract, retain the failing witness and revise
the mechanism before production integration; do not fall back to a queue,
whole-population awake scan or live mutation plus restore.

### Rejection and unexpected failure

Contact capacity, reference and discovery/compaction-work exhaustion are typed,
healthy rejections before publication. Compare the entire committed snapshot:
body position/velocity/force/speed/displacement/motion, field accumulators,
packets/selectors/history, focal/universal values, native bits, source/union/CSR,
counts, parameters/invalidation bookkeeping, frame, generations and last report.
The attempted demand is separate; neither a full numerical host round trip nor
a reset/reseed/restore obtains this guarantee. A valid retry must match an
independent execution that never attempted the rejected tick.

Invalid input/domain rejection before mutation remains explicit. Unexpected
backend failure that prevents trustworthy candidate completion, or failure after
committed copying begins, makes execution unready. Teardown remains valid;
healthy rollback after publication begins is not claimed.

## 4. Certified nearest-free placement

Implement placement as a native candidate operation before using it to initialize
new rows. Its public inputs contain intrinsic mass/type/radius, complete-geometry
provenance and prospective structural references; no ordinary input supplies
world position, velocity or a callback that fills numerical starts. Numerical
mass and exact structural contribution counts remain distinct.

Occupancy consists of current backend present physical bodies, including sleepers,
and earlier accepted placements in the prospective batch. Fields, inactive
alternatives and unused addresses add no obstacle. Exclude only bodies explicitly
removed by the validated prospective ledger. Old-plus-new native claims still
include outgoing records until publication. Missing geometry rejects; genuine
zero extent remains zero.

For target F and new radius r, minimize distance to F subject to separation from
each occupied `(c_i,R_i)`. Use F if its stored published coordinates are valid.
Otherwise obtain a checked finite feasible upper bound and run a bounded adaptive
box/interval search inside it. A box's distance to F supplies a conservative lower
bound; certified exclusion containment and objective bounds prune it. Deterministic
subdivision refines remaining regions. Sphere projections/intersections may improve
the upper bound, but every proposed stored point is checked against all relevant
occupants. A ray or direction list supplies an initial candidate, never a proof.

The implementation must derive outward rounding and distance bounds for the
actual strict f64 operations, radius sums and position-publication mask. Include
the coordinate quantization error introduced by the configured retained bits.
Report the best feasible distance upper bound, unresolved global lower bound,
coordinate/ULP-derived certificate tolerance and tested stored position. Successful
termination requires that their gap satisfy that derived bound. The tolerance
cannot be a physical clearance, a user-tuned epsilon or enlarged to excuse work
exhaustion. Tangency is admissible under the documented outward extent checks.
Unrepresentable/FTZ-domain arithmetic rejects rather than producing an unproved
certificate; this work does not change the backend's numerical domain.

Use deterministic operative-frame coordinate ordering for equally admissible
minima, then canonical source order. Qualify coincident/contained exclusions,
overlap, zero extents, tangency and multi-boundary minima. Search boxes are bounded
numerical workspace, not particle locations or a model lattice. Reserve cell and
work limits at setup. Exhaustion before certification rejects the whole attempt
before claims, even when a plausible feasible point has been found.

The placement implementation PR must present the bound/error derivation and an
independent small C++ sphere-boundary/intersection reference. Analytic examples
alone do not establish global coverage. Record worst-case finite work and measured
supported profiles; no claim of arbitrary-size convergence is made.

## 5. Bootstrap and combined admission

After candidate ticks and placement are qualified, add one coordinated admission
entry using #198's prospective source/hierarchy validation and native commit.
An ordinary request fits Batch. An initial complete submission has its own named
`BootstrapCommands` reservation; it may exceed Batch while fitting P/G/E/C/F.
Stage the entire input, pending references, generated rows and full tree before
claims, dispatch native blocks within that reservation, and publish once. A series
of individually committed partial submissions is not a bootstrap transaction.

First admission validates an explicit nonempty prospective focal selection.
Apply D1 exactly: the first exposed participant of the first selected field in
canonical occurrence/path order becomes the local-zero anchor. Place remaining
bodies relative to the already placed focal contributions; use the anchor until
another such contribution exists. All selected fields need positive mass before
populated readiness. Empty prepared execution remains valid.

For D2 ordinary batches, freeze the validated current combined focus at entry.
Process newcomers in canonical occurrence/path order, extending prospective
occupancy after each placement. Request serialization and native claim order
cannot select placement order. The engine consumes bounded canonical structural
ordering; the subsequent owner adapter binds it to full source occurrence paths.
Recompute affected comparisons and final focal/universal controls once for
publication. Apply D3 zero velocity/force/displacement/speed and initial awake
state only to genuinely new bodies; preserve survivors exactly at admission.

Validate prospective contacts, response/coverage ownership, placement certificates,
work and simultaneous source/tree/contact/reference peaks before native claims.
Bootstrap must validate its first hierarchy against that same prospective source;
do not commit source first and then call the public installation API as a second
potentially rejecting operation. Ordinary admission uses the installed-tree refresh
boundary. Keep fixture initialization private and ordinary update/reset guards.

Failure before claims retains the healthy old execution or empty state. Unexpected
failure after claims follows #198's unready disposition. On success publish source,
hierarchy/history, initialized rows, installed contact union, activity, focus and
one generation together; admission advances no simulation tick. Tick immediately
before/after admission in the same execution.

This completes the engine boundary for new occurrences, not the enduring owner
or operational Add command. D5 restart, D6 focus/rebase and D7 representation
exchange retain their accepted contracts for slice C. In particular, later D7
replacement targets the outgoing occurrence's current centre; this admission
interface must not turn replacement into a new placement at study focus.

## 6. Reservations and observation

Add named `TickWork`, `PlacementWork`, `PlacementCells` and `BootstrapCommands`
limits as their corresponding implementation portions land. Use GeometryWork
for exact contact discovery/order work and existing P/G/E/C/R/F/B/S limits for
their current meanings. Check arithmetic/ABI bounds, current/final demand and
old-plus-new claims separately. Disabled opt-in families preserve existing
numerical/native payload layouts and kernel signatures, and return Unsupported
through their new entry points. Report fixed C++ control-object overhead separately.
Charge both geometry boundaries against one per-attempt GeometryWork allowance;
TickWork separately charges bounded copies, numerical phases and publication.

The actual manifest must charge all simultaneous committed/candidate copies:
particle values, speed/displacement and flags; group values/flags/history;
hierarchy two-slot packets, selectors, previous packets and visit/counter outputs;
focal/universal controls; temporary native selection/intents and native metadata;
explicit/current/candidate contact union and source mappings, both CSR directions;
placement cells/certificates; full bootstrap/source/identity/remap buffers; and
every host inspection/transport surface and prepared context.

For f64, a complete candidate numerical subtotal is
`8*(15*p + 14*g + 12 + 12*h)` bytes plus
`4*(2*p + 2*g + h)` for the corresponding flags/selectors, where p/g/h are
physical sentinel strides. Add actual counter/visit widths, native selection
storage, geometry/topology and all other surfaces above. This is a subtotal,
not total scratch. Derive actual extents from current layouts and constants;
the old #170 f32/18-counter illustration is not the new manifest.

Compile, bind, provision and warm the enabled families, including unused blocks,
maximum supported transactions and rejection paths, before readiness. Reuse
scratch only with disjoint-lifetime proof: current and candidate contact views
and live/candidate packet history overlap during rejection validation.

Measure setup, first use, ordinary/rejected operations and 32 reuse cycles
separately. Retain known native backing/layout/compiler/binding counters and
linked C++ allocation observations. Separate numerical work, native selection,
contact/index/search work, copies, bytes and synchronization; claim neither
driver residency nor pinned transfer from these CPU measurements. Define ownership,
completion and CPU/capability-negative checks for each added transport surface.
Actual GPU pinning, driver behavior and CUDA execution remain later #145 evidence.

## 7. Implementation portions and acceptance

Each portion has its own focused implementation PR and evidence. This order
makes contact-safe continued motion available before operational admission.

1. **B1 — contact-safe candidate ticks.** Factor shared native numerical builders,
   qualify transient native phase selection, reserve full candidate state and
   contact surfaces, implement typed rejection/one publication, and retain compiler
   and end-to-end parity evidence. Fixture inputs are sufficient for this portion.
2. **B2 — certified native placement.** Implement the bounded current/prospective
   occupancy view, interval/error bounds, search certificate and deterministic
   stored-coordinate validation. It remains a candidate operation until B3.
3. **B3 — bootstrap and admission.** Supply generated initialization, full bootstrap
   staging and one source/hierarchy/contact publication using B1/B2 and C3-2.
   Exercise retained letter occurrences; then release the engine boundary for C's
   enduring owner integration. B1 or B2 alone is not operational letter insertion.

Minimum executable witnesses, for BulkCopy/CpuDirect and one/two CPU workers:

| Area | Required proof |
|---|---|
| Numerical refactor | Accepted legacy/trial regressions unchanged; candidate/committed shared builders agree with independent references, including positive/zero dt, cancellation and dirty multilevel lag/history. |
| Native phase selection | Particle consume, contact propagation, centroid publication and later notifications retain their ordering; newly notified particle sleepers never calculate early. Preserve full same-poll centroid progress and at-most-one visits. Inspect actual LLVM and preserve unsafe/safe controls in default/scalar shapes. |
| Moving contact overflow | Start within capacity; candidate motion creates excess pairs. Named C rejection preserves the entire committed snapshot and last report; valid retry matches an execution with no failed attempt. Exercise sleeping peers on both index sides and across pointer blocks. |
| Discovery/order limits | Independently exhaust work with final pair demand fitting C, references and scratch reservations. Validate both CSR directions, source duplicates/orientation, source-empty coverage and contact entry/exit. |
| Placement | Free/occupied focus, tangency, variable/missing/zero extents, sleepers, holes/block boundaries/last row, contained/coincident/overlapping exclusions and a multi-boundary minimum. Check local frames translated around 0 and 2^27, configured retained precision, and certificate/stored geometry against analytic and independent C++ references. |
| Placement rejection | Cell/work/domain exhaustion cannot publish an uncertified candidate or claim a record. Valid retry preserves the earlier execution. |
| Bootstrap/batches | First transaction larger than Batch, invalid/empty selection and late-field rejection; one publication only. D1 anchor versus D2 frozen focus, canonical-order permutations and prospective batch occupancy. |
| Occurrences | Two complete occurrences of a retained letter run while a third is admitted. Preserve survivors and exact counts; tick before/after, retire/reuse through supported updates, exercise 32 rounds and unused native blocks. |
| Observation/failure | Reject stale source/geometry/capture references after successful publication; preserve bindings/report on rejection. Inject pre-claim/candidate and post-claim/publication failures with the correct healthy/unready result and safe cleanup. |

Use the existing 70/4,096 capacity profiles where applicable; bounded search
profiles must state achieved work/certificate limits rather than assume maximum
P always certifies. Run the existing full CPU regression and PostgreSQL-to-active
integration for each final implementation source, with retained cold data read-only.
Keep raw failure controls, source/backend/compiler pins, resource measurements,
actual emitted modules and complete per-mode output. Budget hosted jobs for their
whole workload; do not remove modes or repeat unchanged baseline CI for this plan.

After CPU acceptance, the same implemented operation matrix and final-source
LLVM/PTX need actual GTX 1070 evidence. Preserve the CUDA qualification guard.
This proposal does not implement owner commands, cache-manager functions, new
database population, analyst behavior, a GUI or Docker packaging, and it closes
none of #172/#177/#156/#145.

