# Core continuation after local CUDA corrections

Status: maintainer implementation sequencing, updated 2026-11-07. This organizes the
accepted #194/#197 hierarchy and #170 operation contracts. The status table
distinguishes completed CPU slices from remaining work. Owners: #172/#177 (hierarchy/numerics), #156 (operations),
#145 (hardware qualification).

## Reviewed baseline and limits

C3-2 is accepted and merged in #198 at `5ea9c76`, reviewed head `f4c5ba9`.
Final hosted run 41123391538/job 124295521781 at `904f2bc` passed 101/101 CPU
suites in 3355.27 seconds, both extra four-mode no-unroll matrices, compiler
collection/audit, 180 PostgreSQL-to-active checks and actual CPU-only backend
verification; all nine checks passed. Its [retained evidence](../engine/tests/live_hierarchy/refresh-v1/README.md)
keeps the earlier initialization failure and job-budget cancellation. Slice A
is complete for CPU structural refresh. CPU B1/B2 and private ordinary/bootstrap admission are accepted through #210;
focus/rebase and existing-field D7 atomic exchange are accepted through #219.
Field lifecycle, persistent exchange ownership and reserved projection reuse are accepted
through #227 with scoped preparation reuse; broader owner integration remains active. No manual
rerun of the accepted baseline is requested. CUDA refresh remains unqualified.

#195 is merged. Review of #196 at
`1af1cacef98d34928d8b0bbbbb5753c6790c61fa` found no new blocking source defect
in its scoped corrections. Its tested code pin is
`758ad053629a6ca9beea5422de659aa38ba15e48`; final documentation is separate.
All nine hosted checks subsequently passed. The maintainer independently
inspected native job 123743238316: 97/97 CPU-only tests in 1482.34 seconds,
all four extra no-unroll modes, 180 integration checks and actual CPU-backend
verification. #196 is accepted and merged at `68e4104`. Do not rerun this
baseline merely to obtain a second result.

The [contributor evidence](https://github.com/Human-Cognome-Project/human-cognome-project/blob/1af1cacef98d34928d8b0bbbbb5753c6790c61fa/engine/tests/cuda_local/README.md)
records 100/100 CPU tests on a CUDA-enabled substrate, 4/4 CUDA suites,
ten requested C3 GPU case selections, 180 NAS integration checks and 89 browser
checks. These are inspected local records, not reviewer-executed GPU tests or
hosted results. Full C3 logs and compiler artifacts remain local; committed
summaries/hashes identify them. Historical PTX audits are not final-source
reruns. The actual GPU scope is the explicitly enabled GTX 1070/sm_61 path;
normal builds retain the C3 CUDA guard.

The authorized wide-profile motion rule retains accepted displacement/dt for
positive dt and incoming velocity for zero dt. It intentionally discards
unpublished momentum. Preserve that rule in new references from accepted #196;
do not silently restore the earlier untrimmed-velocity policy. Force aggregation,
one integration, body publication precision and native activity remain coherent.
Exact stillness/crystallization is not required of every moving fixture.

The existing f32-to-f64 conversion probe still loses `0x1p-149` on CUDA.
No mixed-precision parity claim is accepted. The f64 hierarchy and this failing
conversion path must remain separately described.

## Delivery order

| Slice | Deliverable | Dependency and execution |
|---|---|---|
| A | C3-2 combined source/hierarchy refresh | Accepted in #198 with complete hosted CPU evidence; new-body starts remain private fixtures. |
| B | Native admission and candidate contact/tick transaction | Private CPU ticks, placement, admission/bootstrap, focus/rebase and existing-field atomic exchange are accepted through #219. Field lifecycle is accepted in #221. |
| C | Enduring operational owner and letter commands | Private exchange owner and scoped preparation reuse accepted through #227. Nested map reuse is provisionally accepted in #228; complete total owner resources and remaining-operation integration next. |
| D | Observable same-execution study | Attach-before-start, step/run/pause and optional pacing; fixed-study controls may develop earlier, but live Add depends on C. |
| P | CUDA mixed-precision conversion repair | Independent narrow backend work; local GPU evidence required. |
| E | Integrated operation and hardware evidence | Exercise completed core slices on CPU, then the same path on GTX 1070. Docker release follows separately. |

Each slice has a focused PR and retained source-pinned evidence. Do not fold
Docker, a new GUI, new data population or a new numerical policy into A.
No routine permission step is added to the accepted implementation scope.

## Next agent task — 2026-11-05

Updated **2026-11-07**. Start from current main, with reviewed head
`b9ea490f86333ba1bd572abd74a202fe19c7534a` accepted through #227.
Native run 42252528197 and smoke 42252525837 passed: inspected logs show 115 unique
regression tests exactly once and 180 database checks with zero private reads;
all compiler jobs passed. #225 and #226 native runs also passed. #224's cancelled
run remains incomplete historical evidence, qualified by successful descendant runs.
[Acceptance evidence](../engine/tests/admission/README.md#maintainer-integrated-acceptance--224227-2026-11-07)
retains source pins, cancellation details and distinct contributor/hosted results.
Do not rerun the accepted baseline merely to continue.

Accepted reuse now covers #224 retained coordinator command/intrinsic/hierarchy backing, #225 immutable declarations, #226 binding/source buffers and #227 all-alternative membership catalogue with allocation-free source capture after map staging.
Nested identity-map staging reuse is provisionally accepted in #228; complete owner accounting is next. Catalogue construction
and graph temporaries move to adoption; retained catalogue ownership, nested keys/pins,
coordinator maps and caller preparation are not covered by the projection census.
The separate first #229 factoring slice moves numerical-kernel construction into
its own compilation unit without changing ownership or numerical expressions.
Complete owner peak/lifetime/work accounting remains open. Its inventory guides
further responsibility separation; this mechanical move does not replace that task. The harness remains borrowed
and unrelated generation changes make the owner stale.

Provisionally accepted #228 retains nested map slots and key backing across
staging, retirement and publication, guarding first/repeated successful owner
preparation against C++ `new`. Its bounded reuse evidence does not complete the
owner resource budget or other operations. Accepted scope remains through #227.

The first #229 extraction is provisionally accepted in #230. Provisionally accepted
#231 adds a combined retained-capacity census and [resource/lifetime/work inventory](../engine/docs/OWNER-RESOURCES.md).
This covers retained payload families, not setup peak, allocator/control-block
bytes, cleanup measurement or complete byte/work budget enforcement. Complete
those gates next; use the inventory to guide subsequent responsibility factoring.
Provisionally accepted #232 measures adoption's scoped C++ requested-byte
peak and tests allocation-failure unwind and final-alias reclamation. This does
not establish machine peak or complete resource qualification. Byte and
preparation-work budget enforcement remain next.

Provisionally accepted #233 gates hierarchy-sealing input/destination payload bytes
and copy work before table access, preserving rejection/retry. This is one
bounded preparation operation, not complete owner budgeting. Adoption and other
preparation gates plus combined borrowed/caller policy remain outstanding.

Provisionally accepted #234 gates source-capture structural work incrementally before
scans, variable-length comparisons, masks and final validation. Exhaustion leaves
partial capture invalid and preserves committed state for retry. This does not
gate earlier declaration/projection/map staging or later command construction;
adoption-byte/setup-work and combined resource policy remain outstanding.

The next contribution adds a separate command-construction work allowance for
coordinator revalidation, searches, copies, scans and field-retirement moves.
Phase results retain capture and command charges independently; partial candidates
remain consumed/unsealed until successful retry. Preceding staging, adoption and
the combined caller/borrowed policy still require gates and qualification.

On the explicit provisional #228/#230/#231/#232/#233/#234 dependency, remaining work is **slice C: complete owner resource/work accounting and operation integration**:

1. Maintain one prepared execution with trusted logical occurrence/field bindings
   derived from actual admission claims. Publish projection exposure and mappings
   coherently after successful native edits; rejected edits retain the old owner.
2. Make preparation reusable across admission, explicit exchange, removal/reuse,
   focus/rebase and controlled restart under D1–D7. Preserve survivors, dirty
   history, pending work and generation semantics.
3. Account for complete owner resources, including host commands, alternate maps,
   projection data and hierarchy preparation alongside native storage. Private
   transaction vectors outside the scratch manifest are not complete accounting.
4. Qualify repeated operations and rejection/retry on the same execution, then wire
   same-execution controls in slice D. Do not use legacy FixedStudy reconstruction
   as operational Add.

The accepted [declaration](../engine/docs/EXCHANGE-CONTRACT.md),
[source binding](../engine/docs/EXCHANGE-BINDINGS.md),
[native accounting](../engine/docs/EXCHANGE-ACCOUNTING.md) and
[atomic coordinator](../engine/docs/ATOMIC-EXCHANGE.md) are the existing primitives.
Do not reimplement their completed D7 checks. #221 adds field lifecycle but does
not itself create an enduring owner or qualify complete owner resources. The
owner increment adds the repeated-exchange projection lifecycle. Projection reuse
and its capacity census do not account for all remaining owner resources.

The contracts below retain required behavior for each slice; imperative wording
in an accepted slice does not reopen its completed implementation. Local evidence
permits scoped provisional continuation; hosted findings must be reconciled before
final acceptance. Actual GPU qualification and Docker release remain later work.

## A. Coherent source/hierarchy refresh

The [C3-2 implementation plan](live-hierarchy-refresh-plan.md) is accepted in
#197 at merge `0eea42a`. It maps this slice onto prospective source references,
stable comparison identity, native repair/commit phases and executable acceptance.
The implementation is accepted in #198 at merge `5ea9c76`; the contract below
remains the boundary of that accepted numerical-fixture refresh foundation.

Owning contract: [production plan, section 4](production-hierarchy-response-plan.md#4-initialization-structural-changes-and-failure).
Start at `engine/src/field/prepared_update.cpp`, `live_hierarchy.cpp` and
their prepared storage/binding interfaces. The nonempty-update guard stays until
the combined operation is complete; removing it or uninstalling/reinstalling
around an ordinary update is not the implementation.

Provide one bounded owner-exclusive operation taking expected generation,
structural commands and a complete replacement hierarchy specification.
Refactor validation/staging so source and hierarchy inspect the same prospective
ledger before claims. Resolve pending command references there; derive exact
counts/coverage from the ledger. Account for simultaneous committed/prospective
surfaces and old-plus-new claims before any live mutation.

Match surviving comparisons by full identity, not row index. Preserve their
slots, selectors, old XYZ/mass history and pending work when unchanged.
Repair changed leaves and affected ancestors once from complete current or
prospective packets; do not globally reseed or erase unaffected lag.
Commit source, topology, ownership, history, adjacency, generation and required
invalidation together. Preserve surviving physical state. Ordinary rejection
leaves the old execution healthy; unexpected failure after live writes makes
it unready.

Acceptance: count-only changes above f32 distinguishability, mass changes,
reorder, contact edits, insertion/retirement/reuse, changed comparison/root
coverage, stale references, capacity/work exhaustion and injected late failure.
Exercise two/three levels, both CPU host paths and one/two workers, 32 reuse
rounds, no lost input, preserved survivors/history and stable prepared resources.
Fixture starts remain explicitly test-only here. This slice does not claim
nearest-free operational admission.

## B. Native admission and contact-safe ticks

The [accepted #199 implementation plan](native-admission-contact-transaction-plan.md)
maps this released slice to B1 candidate ticks, B2 certified placement and B3
combined bootstrap/admission. B1 is accepted in #200/#201; see
[contact-safe ticks](../engine/docs/CONTACT-SAFE-TICKS.md). B2 is accepted in
[#203](https://github.com/Human-Cognome-Project/human-cognome-project/pull/203),
superseding #202. B3 implementation is now released; candidate placement and
private generated-row staging alone are not structural admission. The accepted
#170 requirements below remain authoritative.

Owning contract: [core operation plan, sections 2–3](core-operation-readiness-plan.md#2-native-engine-admission-and-occupancy).
Reuse #175 current geometry/contact queries and the native numerical kernels.

Implement D1–D4 bootstrap, canonical batch order and nearest-unoccupied placement
using actual extents and current awake/sleeping occupants. New bodies start with
zero motion and an awake bit. Ordinary source recipes provide no world starts.
Use the accepted bounded nearest-distance certificate and explicit work budget;
a first-free ray or guessed radius does not satisfy the contract. Reject an
uncertified or over-budget result before native claims.

Stage candidate motion, contact coverage and consume/wake effects without
changing committed numerical state. Discover the explicit/derived contact union
against candidate positions, including sleeping peers; validate both adjacency
directions and all work/capacity limits before publishing the completed tick.
Reuse engine equations; no CPU replay, host rollback or second persistent
activity scheduler. Dynamic contact discovery feeds the following particle phase.

Acceptance includes empty/occupied focus, tangency, overlapping exclusion
surfaces, incomplete extents, batch occupancy, source-empty contact lists,
new contacts during motion and both index/block sides. Force candidate motion
to exceed contact capacity and separately exhaust discovery work: the operation
must reject with committed state, history, activity, generation and last report
unchanged, then permit a valid retry. Charge all candidate/live peak storage.
A/B must compose as one coherent admission boundary before operational Add ships.

## C. Enduring owner and source-to-live operation

Owning contract: [core operation plan, section 4](core-operation-readiness-plan.md#4-enduring-ownerworkbench-integration).
The current `assembly::ActiveSession` still owns a legacy `FixedStudy`;
its refresh replaces the assembly generation before validation. Do not wire
interactive Add to that rebuilding/republication path.

Maintain one prepared execution and bounded owner-local source/occurrence/native
bindings. Build a prospective diff and publish owner mappings only after the
engine commit succeeds. Two instances of one letter remain distinct; adding or
removing a third must not reset either survivor. Rejection retains the healthy
old owner handle and last completed report; successful changes invalidate stale
references coherently.

Wire retained coordinate-free letter recipes through native admission. Implement
the accepted removal/reuse, supported explicit aggregate/parts exchange, focus/
rebase and controlled restart operations as reviewable portions of this slice.
D7 exchanges target the outgoing logical centre, not study focus; genuinely new
occurrences target focus. Apply current-profile history/numerical requirements
rather than restoring obsolete discarded-residue behavior.

A fresh run requires no prior model state. Optional restart positions are
validated owner-bound hints under D5, not a general checkpoint implementation.
Preserve the manager's reject-before-lookup privacy boundary and existing stores.

## D. Observable study and command-line control

Use one active run owner and the existing completed-state capture interface.
A command-line operator or coding agent is the intended audience; a polished
launcher or new GUI is not an acceptance requirement.

Provide attach-before-start plus explicit start/pause/step/stop, bounded Add
through C, and optional wall-clock pacing independent of simulation dt.
Define when owner commands take effect at completed boundaries. Pacing must not
alter physical parameters, numerical tick order or activity predicates.
Expose actual backend, completed tick/generation and source identity.
Atomic `latest.json` updates must come from that same execution.

Test paced/unpaced equivalence for the same explicit tick/command sequence,
pause/step accounting, invalid commands, orderly stop and coherent captures
across successful/rejected edits. Keep native observation neutral.
Locally verify actual folder attachment and visible changes after Add; automated
fixture imports are not evidence that a person observed the running GPU flow.

## P. Mixed-precision CUDA repair

Start with `engine/tests/precision_test.cpp`, native precision lowering and
CUDA JIT FTZ/function attributes. Preserve the failing sign/boundary witnesses.
Isolate conversion from storage/transport and significand clearing; inspect
actual LLVM/PTX to establish where the subnormal input is lost. The blanket f32
FTZ setting is a diagnostic lead, not a proven final correction.

Implement the smallest backend correction that satisfies the existing
conversion contract. Do not alter the oracle, flush expected results, redefine
the domain or change all fast/strict math behavior merely to green the test.
Test both signs, signed zero, subnormal/normal boundaries, all supported
conversion directions, invalid/domain outcomes and mixed strict/default kernels.
Run CPU regression and actual GTX 1070 tests; retain final-source emitted-code
evidence. Keep mixed precision unqualified until this passes. This work need
not hold up CPU implementation of A–D or a declared f64-only study.

## E. Full operation evidence and separate release

For #156 completion, exercise every accepted operation row in #170 or record an
explicitly reviewed unsupported disposition; a static three-body capture does
not complete that matrix. Carry the same insertion, contact rejection,
survivor/history, owner, restart and resource cases to the actual GTX 1070.
Retain final-source LLVM/PTX, ordering negative controls, packet/reuse checks,
actual backend and no-fallback checks. Distinguish C++ allocation counters from
driver/residency observations. Pageable versus genuinely pinned transfer and
lifetime/cleanup remain explicit #145 work; no speed claim follows from
functional success. GPU1/GTX 750 Ti remains deferred.

Development and visual studies use the local native build and NAS PostgreSQL.
Use RAM-backed transient work within a measured memory budget, avoiding swap
pressure on the old disks. Keep credentials and NAS endpoints out of artifacts.

Docker is the release state: prebuilt application image(s), separate PostgreSQL
service initialized from retained paired snapshots, launch configuration,
notation/commands, source/image/fixture hashes and optional prior-model inputs.
File bundles under `docker/snapshots/<date>-<source-revision>/` using appropriate
large-artifact handling. Verify clean load/restore/run without rebuilding or
requiring a previous model state. Application/build/Docker work stays in a
separate PR.

The bounded mini-release gate is reproducible letter physics and observable
active execution with an honest supported-operation/backend list. It does not
require completion of analyst implementation/calibration or all deferred LoD work,
nor automatically close #156/#145. Analyst functions are conceptually designed;
their implementation structure must be engineered and calibrated against engine
results on real data structures, as clarified in
[architecture](../engine/ARCHITECTURE.md#analyst-design-and-engineering-status).
Any unsupported #156 operation remains explicitly tracked. Do not publish a
release merely because this plan exists.


## Documentation factoring after the engine PoC

Reviewed 2026-11-07: defer broad AGENTS.md restructuring until after the engine
PoC release; correct misleading guidance as it is found. Keep the current queue,
authority/reading order and repository-wide invariants in AGENTS.md. Consolidate
detailed numerical, placement and model requirements into their owning contracts,
with explicit required-reading links for affected work. Preserve dated decisions,
acceptance evidence and existing link anchors; historical records explain the
development journey and are not disposable onboarding material. Verify that the
shortened entry point still leads new contributors to every applicable constraint.
This documentation work does not block the PoC or replace engine qualification.
