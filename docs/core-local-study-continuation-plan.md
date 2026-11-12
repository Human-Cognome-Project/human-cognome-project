# Core continuation after local CUDA corrections

**Maintainer integration — 2026-11-12:** #306–#307 are accepted and integrated by the merge containing this record, preserving reviewed `88aa2cd0ce6360b2cc2d9d1f446294bb50203309`. #306 native 43305748693/smoke 43305748766 passed; #307 native 43310337611 passed all 12 jobs and smoke 43310337168 passed. Personally inspected #307's 119 passing regressions (30/30/30/29), including native_pool_test, allocation_record_test, prepared_backing_test and reservation_request_test; database job 131512684418 reports addressed_reads=24/private_reads=0. Main already includes qualified #302–#305; the original #302 initialization failure remains preserved. #308 rounded virtual mapping extents are provisionally source-accepted at `10b3ed7f` in review 5883275400; smoke passed, native qualification remains active. Contributor-local archives were not independently inspected. Older pending/next labels below are historical where they conflict.

**Next bounded task:** remaining prepared/runtime/pool metadata on reviewed #308, then lifetime-valid required-family Harness coverage and canonical backing deduplication with prepaid lookup/enumeration work before checked C+B+H+D admission. Rounded mapping extents are not physical residency; shared chunks and contained requests are not disjoint charges. Preserve native authority and separate work gates; setup cost is acceptable for long-running focal models. No full B or GPU qualification is claimed. Projection-copy audit stays closed; #75/#111/#123 remain held.

**Rounded host-pool extents (awaiting review, 2026-11-12):** continues #307
review5882509096 at88aa2cd0. Native setup captures OS page size; checked rounding is
passed to the OS and retained separately from raw request sizes. Live pool records
report virtual rounded extents; request/view bounds remain unchanged, so padding does
not enlarge valid views. Added metadata is included once by existing sizeof reports.
Main a139c21b integrates qualified #302–#305; normal merge ba723dd0 reconciles the
handoff without discarding prior contribution history. #306/#307 native and smoke
runs now pass; maintainer integration remains pending. Next: remaining metadata, Harness lifetime/
coverage/deduplication/work, then checked C+B+H+D. Linux CPU scope; no full B/GPU claim.

**Maintainer integration — 2026-11-12:** #302–#305 are accepted and integrated by the merge containing this record, preserving reviewed `44c890b78c973be21a5b2e705ac79c334da3ae4e`. Corrected #304 native 43299289153 and smoke 43299289118 passed; #305 native 43302942903 passed all 12 jobs and smoke 43302942605 passed. Personally inspected 119 passing regression tests (30/30/30/29), including allocation_record_test, owner_backing_test, prepared_backing_test and reservation_request_test; database job 131487117070 reports addressed_reads=24/private_reads=0. The original #302 initialization failure remains preserved, not reclassified as green. Contributor-local archives were not independently inspected. #306 prepared canonical import adoption and #307 host-pool identity remain provisionally source-accepted, with native qualification pending; their smoke runs passed. This supersedes older pending/status labels below.

**Next bounded task:** authoritative rounded extents and remaining prepared/runtime/pool metadata on the reviewed #307 stack, followed by trusted Harness required-family coverage, lifetime validity, canonical backing deduplication and prepaid lookup/enumeration work before checked C+B+H+D admission. Shared raw chunks and contained requests are not disjoint charges. Preserve independent work gates and native authority; setup cost is acceptable for long-running focal models. No complete B or GPU qualification is claimed. Projection-copy audit stays closed; #75/#111/#123 remain held.

**Host-pool backing identity (awaiting review, 2026-11-12):** continues #306
review5881942652 at52696f12. Native raw allocation identities survive chunk-index
movement and reject stale tokens after release/reset/reuse. Common Device/Program
queries resolve proven live owner/request ranges; raw imports remain unresolved.
Raw request sizes are exported, rounded OS extents remain unavailable. Shared pool
metadata is reported once by pool identity with explicit exclusions, not per owner.
Lookup work is logarithmic and remains to be metered with Harness enumeration.
Main e11525a0 is already an ancestor; #302–#306 integration awaits hosted qualification.
Next: rounded extents/remaining metadata, trusted Harness coverage/deduplication/work,
then combined C+B+H+D. No full B or GPU claim; held #75/#111/#123 remain unchanged.

**Prepared owned-import adoption (awaiting review, 2026-11-12):** continues #305
review5881732238 at44c890b7. Separate canonical backing queries preserve captured
registration identity, validate bound byte extent against the registration, and
accept only live explicitly proven owned backing for array coverage. Raw imports
remain unresolved; full host metadata remains incomplete. No new retained metadata.
Summary traversal remains input-sized work requiring future metering. Main e11525a0
is already in this stack; #302–#305 remain unmerged pending corrected parent/hosted
qualification. Next: underlying pool identity/rounded extents and remaining metadata,
then trusted Harness lifetime/deduplication/work accounting and combined C+B+H+D.
No full B or GPU claim; held #75/#111/#123 remain unchanged.

**Explicit owned-import backing (awaiting review, 2026-11-12):** continues #304
review5881459672 at d40daac0. Common Device captures same-device Owned provenance at
import creation and resolves canonical live backing with bounded indexed queries.
Raw imports remain unresolved; registrations, backing ownership and lifetime remain
distinct. Embedded metadata is included once in existing sizeof-based row capacity.
Main e11525a0 is already in this stack. #304's source blocker is corrected, but
#302–#304 integration still awaits corrected combined hosted qualification. Next:
underlying pool identity, remaining prepared/runtime metadata and binding adoption,
then a lifetime-valid work-metered Harness snapshot and combined C+B+H+D admission.
No full B or GPU claim; held #75/#111/#123 and projection-copy closure are unchanged.

**CTest qualification correction (awaiting review, 2026-11-12):** #304 review
5881347735 blocks integration on missing runtime environment for three tests.
#302 native run43287680849/job131433550369 failed owner_backing_test during
initialization; its assertions were not reached. Earlier local runs inherited
TI_LIB_DIR and did not detect the configuration omission. Corrected combined head
e0cc9e72 adds owner_backing_test, prepared_backing_test and reservation_request_test
to the existing explicit runtime environment/900-second timeout group. All three
pass with TI_LIB_DIR unset in the parent shell, as does the combined nine-test
backing/reservation subset. Original failures and earlier evidence remain preserved;
#302 is not represented as green. The fix is carried at the combined #304 head,
not by rerunning unchanged parents. Await corrected hosted qualification/review
before integration; pool/alias/metadata work remains next.

**Native reservation exports (awaiting review, 2026-11-12):** continues #303 review
5880964608. Common ProgramImpl exposes bounded CPU node/list/ambient request views,
with Present/Absent/Invalid/Unavailable states and explicit unresolved pool identity/
rounded extents. Ambient request metadata is reported once per runtime; enumeration
remains caller-metered work, not a hidden scan. Main e11525a0 (#301 integrated) is
reconciled by normal merge b86533e5; #302/#303 remain provisional and unmerged.
Next: pool/import alias resolution, remaining metadata and a trusted lifetime-valid,
work-metered Harness snapshot before combined C+B+H+D. No full B or GPU claim.

**Maintainer integration — 2026-11-12:** #301 common/CPU allocation records are accepted and integrated by the merge containing this record, preserving reviewed `63a46aab6db261d2b8ecf189d06999d87f2f2dd2`. Native 43282494612 passed all 12 jobs and smoke 43282494900 passed. Personally inspected 116 passing regression tests (29/29/29/29), including allocation_record_test, and database job 131415289289 with addressed_reads=24/private_reads=0. #302 array/root exports remain provisionally accepted at `aa48a2e4` (review 5880581671); #303 prepared CPU exports at `bb0dfd28` are provisionally accepted in [review 5880964608](https://github.com/Human-Cognome-Project/human-cognome-project/pull/303#pullrequestreview-5880964608). Both remain unmerged with native qualification active; their smoke checks passed. Contributor-local artifacts were not independently inspected. This does not establish full B, combined admission or GPU qualification.

**Next bounded task:** native node/list/ambient request exports on reviewed #303, then remaining prepared metadata and imported-to-owner alias resolution. Preserve explicit required-family coverage and account retained metadata once at its actual owner. The trusted Harness snapshot must establish lifetime/backing validity, deduplicate actual backing rather than registrations, and meter input-sized enumeration/validation before checked C+B+H+D enforcement. Retain separate work gates and native authority; remaining operation/control integration follows. Projection-copy audit stays closed and additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending/next labels below are historical where they conflict.

**Prepared CPU backing exports (awaiting review, 2026-11-12):** continues #302
review 5880581671. Common launch-context queries expose actual host buffers and
captured array identities, with separate family coverage and explicit incomplete
host metadata. Imports/external/raw bindings cannot certify complete owned backing.
Records are embedded in existing binding rows and included by existing sizeof-based
capacity accounting. Main 0e82ebe0 (#300 integrated) is reconciled by normal merge
f0e1e4b8; #301/#302 remain provisionally accepted and unmerged. Next: node/list/ambient
exports, remaining metadata and alias resolution, trusted Harness lifetime/coverage,
then combined admission. No full B or GPU qualification is claimed.

**Maintainer integration — 2026-11-12:** #300's borrowed-resource audit is accepted and integrated by the merge containing this record, preserving `e540a8712c176c06d2f5729da2aa6602d67ec4cb`. Native 43276619607 passed all 12 jobs; smoke 43276619735 passed. Personally inspected 115 passing regression tests (29/29/29/28) and database job 131388302021 with addressed_reads=24/private_reads=0. Main includes #299 owner bootstrap admission and now the [borrowed-resource audit](../engine/docs/BORROWED-RESOURCES.md). #301 CPU record interface remains provisionally accepted at `63a46aab` (review 5880096613), and #302 array/root exports at `aa48a2e4` in [review 5880581671](https://github.com/Human-Cognome-Project/human-cognome-project/pull/302#pullrequestreview-5880581671). Both remain unmerged with native qualification running; their smoke checks passed. Local archives remain contributor-reported, not independently inspected. Neither a complete B ledger nor combined admission is built.

**Next bounded task:** prepared-invocation backing exports and node/list/ambient reservations on reviewed #302. Preserve explicit required-family coverage, owner lifetime and request/rounded extent distinctions. Resolve imported registrations to actual backing owners before deduplication; account runtime metadata once. Missing coverage or unresolved ownership keeps the complete ledger unavailable. Then assemble the trusted lifetime-valid Harness snapshot, enforce checked C+B+H+D with separate work gates and complete remaining operation/control integration. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Array/root owner exports (awaiting review, 2026-11-12):** continues #301 review
5880096613 with captured Ndarray identities and native tree-root exports through
common ProgramImpl. Released/reset/foreign records remain explicit; imports are
unresolved registrations, not certified backing. Incremental root-record metadata
is reported once per runtime. Prepared invocation and node/list coverage remain
unavailable; no complete B ledger or combined admission is claimed. Main b2eb86c7
(#299 integrated) is reconciled by normal merge ca24735f; #300/#301 remain unmerged.
Next: prepared invocation and node/list exports, alias resolution and trusted
Harness coverage/lifetime, then combined admission. The projection-copy audit stays closed.

**Maintainer integration — 2026-11-12:** #299 is accepted and integrated by the merge containing this record, preserving reviewed head `5771326e4992889c3b9c16b8b98ee752ae4d0d1a`. Native 43274126767 passed all 12 jobs; smoke 43274126747 and warm 43274126853 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131379495998 reports addressed_reads=24/private_reads=0. #298 is already merged at `319f892a`. #300 audit is source-accepted at `e540a871` under review 5879635043 and #301 common/CPU allocation interface is provisionally accepted at `63a46aab` under [review 5880096613](https://github.com/Human-Cognome-Project/human-cognome-project/pull/301#pullrequestreview-5880096613); both remain unmerged with native qualification active. Local archives remain contributor-reported, not independently inspected. Main now includes owner bootstrap admission, not the pending native record interface or complete B ledger.

**Next bounded task:** actual native owner backing exports on reviewed #301, including array/tree-root, node/list and prepared-launch families. Resolve imported registrations to actual owners before deduplication; record identity alone is not backing identity. Preserve explicit coverage and unavailable status, lifetime validity and retained metadata accounting. Then assemble the trusted Harness snapshot, enforce checked disjoint C+B+H+D admission and complete remaining operation/control integration. HCP model code remains backend-independent. Initial-projection copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next labels below are historical where they conflict.

**Previous disposition (historical):**

**Maintainer disposition — 2026-11-12:** #298 is accepted and merged at `319f892a1aa35e29597e89e461c0512eba233e33`. Native 43268613665 passed all 12 jobs; smoke 43268613606 and warm 43268613622 passed. Personally inspected regression logs: 115 passing tests (29/29/29/28); database job 131360157711 reports addressed_reads=24/private_reads=0. #299 owner integration is provisionally source-accepted at `5771326e` (review 5879484896), but native qualification remains active and it is unmerged. #300's five-file documentation audit is accepted at `e540a871` in [review 5879635043](https://github.com/Human-Cognome-Project/human-cognome-project/pull/300#pullrequestreview-5879635043); smoke passed, native is active, and it remains unmerged. Local probe archives are contributor-reported, not independently inspected.

**Next bounded task:** implement the common native allocation-record contract and CPU implementation/tests authorized by #300: original request versus rounded extent, live/retired/unknown status, explicit import ownership and validity across device lifetime/reset/reuse. Unsupported coverage is unavailable, never zero; HCP model code consumes one interface. Then export actual owner backing (including tree/node/list and prepared-launch families), assemble the trusted Harness ledger and enforce checked combined C+B+H+D admission. Resolve aliases and pool/suballocation overlap at the owning boundary; include new ledger storage in the policy and keep backing validity distinct from publication generation. Do not query dead device pointers to test expiry. This is scoped accounted-byte work, not a process-wide profiler. Remaining operation/control integration follows. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #295–#297 are accepted and integrated by the merge containing this record, preserving original commits through `272e160d56201fd1f93353c5ba22efd88f0ef69d`. #297 review [5878815959](https://github.com/Human-Cognome-Project/human-cognome-project/pull/297#pullrequestreview-5878815959) found no blocking source defect. Native 43252923816 passed all 12 jobs; smoke 43252923333 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131305209894 reports addressed_reads=24/private_reads=0. #295 native 43245334785/smoke 43245334673 and #296 native 43249326537/smoke 43249326530 also passed. Local archives remain contributor-reported, not independently inspected. This accepts binding setup, the accounting inventory and checked initial-map work prepayment, not complete owner-resource, GPU or performance qualification.

**Next bounded task:** track actual domain bootstrap request bytes with an outer tracker whose lifetime safely covers domain/control-block destruction, and establish a trusted disjoint borrowed-resource ledger. Then enforce combined accounted-byte admission using those disjoint components and selected live requests; do not sum overlapping snapshot, retained, peak or cumulative-work counters. Preserve checked arithmetic, pre-allocation refusal, reclamation, native state and exact retry controls. Remaining operation/control integration follows. The projection copy audit is closed. Additional setup work remains acceptable for long-running focal models; reducing setup totals is not an acceptance gate. #75/#111/#123 remain deliberately held/deferred.

**Native allocation records (awaiting review, 2026-11-12):** #300 review 5879635043
accepts a CPU-first common native contract. This contribution adds explicit record
status/ownership and separate original-request/extent fields, with device-instance
and reset-epoch value tokens checked only through a known-live owner. CPU supplies
the records and retained metadata; unsupported backends report Unavailable. Tests
cover alias/import/retirement/reset/address reuse and registration-failure retry.
See [native record contract](../engine/docs/BORROWED-RESOURCES.md#cpu-allocation-records-awaiting-review-2026-11-12). Main `9025c301` (#298 integrated) is reconciled by normal merge `2a919e22`;
#299/#300 remain reviewed but unmerged. Next owner backing exports, trusted Harness
coverage/lifetime, then combined admission; no complete B/GPU acceptance is claimed.
**Borrowed backing audit (historical contribution, 2026-11-12):** #299 review 5879484896
accepts owner bootstrap provisionally and authorizes the trusted native B ledger.
The source audit identifies missing common allocation extent/lifecycle queries,
private SNode/node-pool and prepared-launch backing, and incomplete per-harness
coverage. A focused CPU probe confirms alias overcount, cross-device numeric-ID
collision and retained size metadata after release. See [borrowed resources](../engine/docs/BORROWED-RESOURCES.md) for source pointers,
explicit exclusions, validity requirements and the bounded native-hook sequence.
No B ledger or combined gate is implemented by this audit. Next implement native
request/lifecycle observation and owner backing exports before assembling trusted B;
then enforce combined admission. #298/#299 remain open with native CI active.
**Owner bootstrap integration (historical contribution, 2026-11-12):** continues from
#298 review 5879152987 on `386be0a0`; #298 remains open while hosted qualification
finishes. Owner admission now gates actual outer tracker/domain control-block
requests and reports them separately from selected owner requests. Both bootstrap
boundaries precede owner construction/native mutation. Later constructor refusals
and allocation exceptions release bootstrap storage while preserving prepaid work;
selected weak references retain the tracker through final release. See [adoption accounting](../engine/docs/ADOPTION-ACCOUNTING.md#owner-bootstrap-integration-awaiting-review).
Next: the trusted disjoint borrowed-resource ledger, then combined accounted-byte
admission and remaining operation/control integration. Both builds and all 47 scoped CPU checks passed without warnings; this new
integration evidence is recorded separately from the accepted helper evidence.
**Bootstrap tracker prerequisite (historical contribution, 2026-11-12):** a new shared
native helper accounts for its own outer allocation and the actual rebound
reservation-domain/control-block request, with lifetime through the final weak
release. It keeps bootstrap and selected requests disjoint. See
[shared support](../common/README.md#domain-bootstrap-tracker-awaiting-review) for the contract and standalone refusal/unwind/lifetime coverage.
This contribution starts from integrated main `f20acbb8`; owner adoption still
uses its existing factory. Next wire the helper into owner admission with native
unchanged-state/retry evidence, establish the trusted borrowed-resource ledger,
then enforce combined accounted bytes. No complete owner-resource claim is made.
**Historical handoff records below:** earlier pending, awaiting-review and next-task statements are source-pinned history, superseded by the disposition above.

**Maintainer integration — 2026-11-12:** reviewed #290–#294 are integrated by the merge containing this record, preserving original commits through `32fe9928cdbed11b5092724d81f3bfbff07f0b9d`. #290's retry 43225419105 and smoke passed. #291/#292 native/smoke passed. Corrected #294 native 43240639938 passed all 12 jobs and smoke 43240639817 passed. Personally inspected #294 logs: 115 regression tests (29/29/29/28), and database job 131262194888 reports addressed_reads=24/private_reads=0. #293's original GCC 11 cleanup failure and #290's original unavailable-log failure remain historical; corrected/retried evidence does not relabel them.

#295 remains provisionally source-accepted at `c118e6ef` under [review 5877025531](https://github.com/Human-Cognome-Project/human-cognome-project/pull/295#pullrequestreview-5877025531); its native qualification is still running. #296's documentation inventory at `c43de241` is source-accepted in [review 5877302279](https://github.com/Human-Cognome-Project/human-cognome-project/pull/296#pullrequestreview-5877302279); its smoke passed and native run is active. Neither is integrated here. This does not establish complete owner-resource, GPU or performance acceptance.

**Next bounded task:** implement the reviewed work-component correction in #296: preserve the input byte census, replace only the two initial map work components with checked fresh-map demand, and prepay the projection copy once. Expose the breakdown and define the bounded fixed-construction policy explicitly. Extend existing zero/one-short/exact and overflow/refusal/reclamation/native-state/retry controls. Then establish domain bootstrap tracking and a trusted disjoint borrowed-resource ledger before enforcing combined accounted-byte admission. Do not sum overlapping snapshot, retained, peak and cumulative-work counters. Remaining operation/control integration follows.

Additional setup work is acceptable where it benefits long-running focal models; lower setup totals are not an acceptance gate. The initial-projection copy audit is closed. #75/#111/#123 remain held/deferred. Earlier dated pending, integrated and “next” labels below are historical and superseded by this record where they conflict.

**Map copy-work contribution (awaiting review, 2026-11-12):** implements #296
review 5877302279's checked component replacement. Projection work is retained
once; fresh occurrence/field map work is prepaid separately and exposed in the
result. The byte census and selected request/allocation-work limits are unchanged.
Fixed units explicitly cover bounded header/scalar/control initialization. Empty,
unequal-input, overflow and pre-allocation refusal/retry controls accompany the
change. Both builds, 47 scoped CPU checks and six standalone compiler CTests passed
without warnings; see admission for source-pinned evidence. Main through #294 is
included by normal merge `9acdd5fb`. Next: domain bootstrap tracking and a trusted
disjoint borrowed ledger before combined admission; operation integration follows.

**Binding setup contribution (awaiting review, 2026-11-12):** continues from #294
review 5876572990 at `32fe9928`, including current main `7bf08034` by normal merge
`cdcf28e0`. The shared owner setup meter now covers binding/coordinator buffer
reserves, map construction/reservation, staging and adoption capture under a
Bindings phase. Later operational staging/capture/matching allowances remain
separate; standalone unrestricted wrappers stay explicit. Expanded map/owner
refusal controls passed with both builds and all 47 scoped CPU checks, without
warnings. Source-pinned qualification is recorded in admission.
Next after review: complete the remaining setup/resource inventory and combined
caller/owner/borrowed accounting, then remaining operation/control integration.
This is not complete owner-resource acceptance; projection copy cleanup stays closed.

**Accounting inventory contribution (awaiting review, 2026-11-12):**
[The source audit](../engine/docs/ADOPTION-ACCOUNTING.md) maps all adoption operations after #295 and
resolves the initial-map distinction: input prepayment covers one logical row copy,
while the metered map constructor additionally counts reservation and nil-slot
initialization. Selected allocations are already gated. The proposed next change
replaces only map work components, retaining projection prepayment once and leaving
the input byte census distinct. Fixed-construction policy, domain bootstrap tracking
and a trusted disjoint borrowed-resource ledger must be explicit before a combined
limit; the inventory specifies that finite implementation boundary. Runtime code is
unchanged and accepted native baselines are not rerun for this documentation slice.

**Historical records below:** earlier pending, integrated and “next” statements are source-pinned history and are superseded by this disposition where they conflict. #75, #111 and #123 remain deliberately held/deferred. CPU acceptance does not qualify the full GPU path.


**Maintainer acceptance — 2026-11-12:** #285 and #286 are integrated into main at `087b6aff14d454c9ebafcda28c9e9243f596e4d2`, reviewed tip `49643f2a22bbb0076584e01d990a32ff7fe0acda`. Exact-head #286 native 43130677310 and smoke 43130677322 passed; #285 native 43121173964 and smoke 43121173792 also passed. All 12 #286 native jobs succeeded. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 130894721312 reports addressed_reads=24/private_reads=0. At that integration the active review queue was clear; #75, #111 and #123 remain deliberately held. Earlier provisional/pending labels below are historical and superseded by this acceptance. No GPU or performance qualification is implied.

**Earlier next task (superseded):** declaration and binding setup now have scoped source reviews through #295. Follow the current #296 accounting task above.


**Maintainer integration — 2026-11-11:** Reviewed #275–#277 and #279–#284 are integrated into main at `5beeb2cacf28c75cbecef385a0de1e877154bbc0`, preserving original commits through reviewed tip `09bafe48869ddf239e6e245d766c0c0e41a43364`. #279's fresh native 43115507493, viewport 43115507480 and smoke 43115507510 succeeded; #280–#284 completed their applicable hosted checks, including #284 native 43116769390 and smoke 43116769371. Original failed and stuck queued attempts remain historical, not relabeled. Earlier pending/integration labels below are superseded by this record. #285 is provisionally source-accepted at `7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` (review 5866915489), targets main and awaits native qualification; its smoke passed. Next: initial tool/occurrence indexes and publication/setup costs, then membership callable/recursion, declaration/binding budgets, combined resource accounting and operation integration. No blanket GPU or performance acceptance.


**Maintainer reconciliation — 2026-11-11:** #278 is accepted and merged at `3cadbff286b0c4553808595f624f8a98add36682`, reviewed head `07f2d66763631afe8d62438351bfd7ece1f149c8`. Native 43087248989, smoke 43087248327 and viewport 43087248550 passed. Inspected native and viewport installation logs confirm the mirror-list rewrite and archive.ubuntu.com traffic. The earlier pending labels are historical. #279 identity/reference input text is source-reviewed and provisionally accepted at `11a4813759c1e384fa6b077499c9f06998c66f58` (review 5864859891); smoke/viewport are green and native matrix qualification is pending. #275–#277 and #279 remain unmerged. Current main at 6067c641 is brought into this stack by a normal merge; the combined head a75e3408 passed 45 scoped local CPU checks plus mirror/CI contract checks. Updated-head hosted qualification and maintainer integration remain pending. Next bounded work: remaining identity/comparison-formatting and membership traversal/callable audit, then complete setup, combined accounting and operation/control integration.


Status: maintainer implementation sequencing, updated 2026-11-10. This organizes the
accepted #194/#197 hierarchy and #170 operation contracts. The status table
distinguishes completed CPU slices from remaining work. Owners: #172/#177 (hierarchy/numerics), #156 (operations),
#145 (hardware qualification).

**Queue reconciliation — 2026-11-11:** Reviewed #268–#274 are integrated into main at `b05db31c2e591f8fbcd5c14dc7ddff14ef46b2e9`, reviewed tip `ce3642ee463beb76948a2497c9aeb50172c3267d`. Every head in that stack has successful native and smoke workflows; #272 warm also passed. #274 native 43030713058 passed all 12 jobs; inspected regression logs total 115 tests and database reports addressed_reads=24/private_reads=0. Earlier provisional/pending labels below are historical and superseded by this acceptance.

**Earlier review queue (preserved history):** #275 parent-value text, #276 contact/membership/focus text and #277 selection text are source-reviewed and provisionally accepted, but remain unmerged. Inspected native jobs 130564239034 (#275), 130638929459 (#276 retry) and 130648619590 (#277) failed during toolchain installation because azure.archive.ubuntu.com could not resolve, before product compilation. #276 viewport retry job 130638930090 was cancelled during dependency installation with the same DNS failures. Preserve these attempts; complete successful hosted qualification before integration. The active contribution attaches definition/occurrence identities and references to owner reservations, per #277 review 5863228338. Other identities/comparison formatting, complete setup, combined accounting and operation/control integration remain open. #75, #111 and #123 remain deliberately held.

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
through #259 with scoped resource/preparation gates; broader owner integration remains active. No manual
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
| C | Enduring operational owner and letter commands | Private exchange owner and scoped resource/preparation gates accepted through #259. Complete remaining constructor/setup limits and combined resources, then remaining-operation integration. |
| D | Observable same-execution study | Attach-before-start, step/run/pause and optional pacing; fixed-study controls may develop earlier, but live Add depends on C. |
| P | CUDA mixed-precision conversion repair | Independent narrow backend work; local GPU evidence required. |
| E | Integrated operation and hardware evidence | Exercise completed core slices on CPU, then the same path on GTX 1070. Docker release follows separately. |

Each slice has a focused PR and retained source-pinned evidence. Do not fold
Docker, a new GUI, new data population or a new numerical policy into A.
No routine permission step is added to the accepted implementation scope.

## Next agent task — 2026-11-05

Updated **2026-11-10**. Integrated main is accepted through #259,
reviewed source `a2636cad6e85b9582f7e37aa1dd537bbd7b5c430`.
Native 42874078745 and smoke 42874078677 passed. Inspected regression logs cover
115 tests; database records 180 checks with zero private reads; all compiler
jobs passed. [Acceptance record](../engine/tests/admission/README.md#maintainer-integrated-acceptance--247-through-259-2026-11-10)
preserves source/run pins and local/hosted distinctions.
Do not rerun the accepted baseline merely to continue.

Preparation gates cover hierarchy sealing, capture, commands and staging.
Adoption input admission and selected actual allocation limits cover binding/
coordinator buffers, map outer stores and setup copies, declaration-table backing
and shared declaration/catalogue object/control-block requests. Shared allocators
retain accounting through final weak release; detached table backing does not
borrow the source owner's quota. These are not complete constructor limits.

**Accepted continuation:** #242's five unique-object gates and #243's retained projection-table gates are integrated. Both projections explicitly attach occurrence, exposure and active/spare parent backing; ordinary snapshots detach accounting and moves retain the domain lifetime. These completed gates need not be reimplemented.

**Combined acceptance:** #245's projection setup work gate and #246's membership contribution-table/temporary graph-node allocation limits are integrated on #246's full green stack. The parent #245 workflow remains recorded as failed with all 11 listed jobs passing and no final aggregate job; cause is unestablished. Do not relabel it green or rerun its baseline merely to continue.

**Completed scope:** retained contribution backing and temporary map/set node requests are admitted before allocation. Nested payload, traversal/callable/stack costs and other setup work remain open.

**Integrated acceptance — 2026-11-10:** Integrated at `d85213d2744a9ad4bb7cb733d7ad61dfef7e1ea9` through #259, reviewed head `a2636cad6e85b9582f7e37aa1dd537bbd7b5c430`. Native 42874078745 and smoke 42874078677 passed; all 12 native jobs succeeded. Inspected regression shards passed 29 + 29 + 29 + 28 = 115 tests; database log reports addressed_reads=24/private_reads=0. This accepts the reviewed #247/#250/#252–#259 allocation stack together. Earlier provisional reviews and source-pinned local evidence remain in the PRs and admission record. No GPU qualification is inferred.

**Provisional dependency:** #261 at `942c93aaa5f4487bd7abcecd0b6f9fb4cca64c9e` is accepted for bounded continuation by [review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/261#pullrequestreview-5849608177), with no blocking findings. Temporary catalogue graph addresses need not be repeated. Smoke 42903689287 passed; native 42903689200 remains running at review inspection. The earlier missing-focus fixture failure remains preserved. Integrated acceptance remains through #259.

**Provisional dependency:** #262 at `3e04a02b07a4aa2872e2d974c39cc0d98b01a7da` is accepted for bounded continuation by [review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/262#pullrequestreview-5850532407), with no blocking findings. Retained recipe addresses need not be repeated. Smoke 42912109232 passed; native 42912109277 remains running at review inspection. Integrated acceptance remains through #259; #261's earlier fixture failure remains preserved.

**Provisional dependency:** #263 at `8647cfc77fd00f9cb2cf7ac0a8b3d9619c06dfdf` is accepted for bounded continuation by [review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/263#pullrequestreview-5851136271), with no blocking source findings. Smoke 42917037200 passed; native 42917037269 was in progress at review inspection and is now confirmed passed, alongside smoke. That review confirms #262 native 42912109277 passed; earlier pending observations remain historical. Preserve #263's post-PASS cache-lock warning and #261's fixture failure. Integrated implementation acceptance remains through #259 pending stack integration.

**Continuation dependency:** #263 is now maintainer-approved in the local continuation instruction, but still open/unmerged at inspection. #264 at `f52506c2f4a4e3ee5a683f1624651330763ec650` supplies top-level recipe tables and awaits review; do not label it accepted. Its smoke 42925890864 and warm 42925891058 passed, while native 42925891067 was running at inspection. Its 40 local CPU checks and two preserved compiler failures remain source-pinned in admission evidence. Continuing work is stacked and does not merge these dependencies.

**Provisional dependency (2026-11-11):** #265 at `21ae236fe19f8c4721ef46fbc602004014e730cb` is accepted for bounded continuation by [review 5851571884](https://github.com/Human-Cognome-Project/human-cognome-project/pull/265#pullrequestreview-5851571884), with no blocking findings. #264 is also provisionally accepted by [review 5851571748](https://github.com/Human-Cognome-Project/human-cognome-project/pull/264#pullrequestreview-5851571748); earlier awaiting-review statements are historical. #265 smoke 42928854772 and warm 42928854774 passed; native 42928854860 was running at review inspection. Integrated acceptance remains through #259 pending dependency integration. Preserve earlier compilation failures and cache warnings separately.

**Provisional dependency:** #266 at `97ab2aafd4406f4d049397509de1ca4ccf54d4e1` is accepted by [review 5851957425](https://github.com/Human-Cognome-Project/human-cognome-project/pull/266#pullrequestreview-5851957425), with no blocking findings and explicit permission for retained recipe UUID work. Smoke 42935015351 and replacement viewport 42935016359 passed; warm 42935015340 is now confirmed passed. Native 42935015396 remains running at continuation inspection. The earlier unsupported-runner failure is preserved, with replacement-browser acceptance superseding its pending status. Integrated implementation acceptance remains through #259 pending stack reconciliation.

**Accepted contribution (#267):** retained recipe UUIDs select the destination allocation domain before payload copying. Uuid stores an engine-independent allocator-aware string; str() returns its const reference and construction accepts string_view with immediate copying/normalization. Explicit ordinary-string conversions preserve SQL, key formatting and viewport encoding. Recipe-only attachment covers publication/owner/creator, tool refs/epochs/authors, parent/member refs, geometry and occurrence/focal-field scopes. Ordinary copies detach and moves retain lifetime. The shared Uuid type/layout changes affect all containing rows; other UUID payload remains unattached and separately scoped. Independent request, complete recipe/encoding and v1/v2 refusal/reclamation/exact retry/lifetime checks cover the selected recipe scope. See [scope](../engine/docs/OWNER-RESOURCES.md#recipe-uuid-allocation-domain-contribution-awaiting-review).

**Integration update (2026-11-11):** Reviewed #261–#267 are integrated into main at `4c0c3c56ea83c7868a62c7dd41de10d183053ec5`, preserving #260. #267 review 5858105951 found no blockers. All four hosted workflows at reviewed head `7d9a7e484d923d17edc223d95007171b273c2680` passed: native 42937702239 (12 jobs), warm 42937702260, viewport 42937702136 and smoke 42937702124. This supersedes earlier provisional/pending status observations below; their failures and source-pinned evidence remain historical. Continue from current main with the remaining projection UUID slice below.

**Provisionally accepted (#268):** remaining retained projection UUID fields: study_, occurrence/provenance owner-study IDs, occurrence/byte/complete-character ToolRefs, and active/spare parent contribution study/field scopes. Reuse explicit pre-allocation Uuid copies, preserve already attached recipe/address/path/text stores and detached snapshots/moves, and extend independent request and refusal/reclamation/exact-retry/lifetime controls. Declaration, catalogue, map, other owner UUIDs and input payload remain separate after this slice, ahead of complete setup, combined accounting and operation/control integration.

**Remaining sequence:** finish the inventoried map/recipe/input and UUID/address payload, preceding catalogue/declaration construction, graph temporaries and complete setup-work limits; then combined caller/borrowed accounting; then remaining-operation integration and same-execution controls (C/D below). Each handoff must name the next concrete slice and the remaining inventory. Do not reimplement accepted gates or add their overlapping domain report to the retained census. Gate before allocation/work and preserve cleanup, retry and exclusive synchronous access. Shared counters are not thread-safe; keep domain overhead and allocator/exception costs separate. See [OWNER-RESOURCES](../engine/docs/OWNER-RESOURCES.md).

**GPU sequencing:** the complete observable PoC still needs C/D integration before the same operation path is qualified on GTX 1070 in E. Existing bounded GPU experiments and the independent conversion repair in P remain separately available. A declared f64-only study does not depend on mixed-precision repair. Transfer/pinning qualification remains under #145; the mini-release's supported-operation/backend list must be explicit. No new GPU task replaces the bounded allocation work above. Awake/asleep/state-on-demand research (#248/#251) remains deferred.

The harness remains borrowed. #229 stays open for further factoring; its first
kernel extraction is complete. Layered onboarding is integrated in #236.

Remaining work is **slice C: finish resource gates and operation integration**:

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

**Dependency review (2026-11-11):** #268 review 5858272490 accepts head
`864eabe731f3238302ac6505516fa6cad4bd4e8e` for bounded continuation, with no
blocking findings. Smoke 43015222103 passed; native 43015222274 remains running
at inspection. Its post-PASS cache warning is preserved. Integrated acceptance
remains through #267; this contribution is stacked on #268 without merging it.
**Provisionally accepted (#269):** retained declaration UUID
payload in LetterExchangeContract owner/recipe identity, outgoing/incoming source
occurrences and references, parent references and leaf references. Audit both
collect and explicit domain-copy paths before allocation; preserve ordinary
snapshot detachment, moves, exact declaration/matching behavior and independent
request/refusal/reclamation/retry/lifetime controls. Catalogue/map/other owner
UUIDs and input payload remain separate; no complete setup or combined resource
claim follows from this contribution.

**Dependency review (2026-11-11):** #269 review 5858547912 accepts head
`f6830b9d404126cb14ee3a2a07db9896cee47936` for bounded continuation with no
blocking findings. Smoke 43017931238 passed; native 43017931472 remains running
at inspection. #268 remains provisional. Integrated acceptance is through #267.

**Provisionally accepted (#270):** retained catalogue UUID payload in
MembershipFields: study/recipe identities and contribution occurrence/group
scope/revision references. Construct identities directly with the destination
domain and select it before contribution payload allocation. Preserve ordinary
detached snapshots, moved lifetime, exact graph outputs, address/table attachment,
independent requests, refusal/reclamation/retry and cycle/deduplication controls.
Temporary graph-key UUIDs deliberately remain detached in this bounded slice.

**Dependency review (2026-11-11):** #270 review 5858692092 accepts head
`065958cb068f453ecc17693850693275aede3b21` for bounded continuation with no blockers.
Smoke 43020338422 passed; native 43020338513 remains running at inspection.
#268/#269 also remain provisional; integrated acceptance remains through #267.
**Provisionally accepted (#271):** temporary catalogue graph-key UUIDs in tools, state,
reached and ancestor-target sets. Extend the independent graph witness to count
UUID requests separately from addresses, distinguish temporary/retained overlap,
and prove prepayment, diamond deduplication, duplicate insertion, cycle rejection,
all allocation-failure cleanup and exact retry. Map/other owner UUIDs and input
payload remain separate before full setup, combined accounting and operation/
control integration.
**Dependency review (2026-11-11):** #271 review 5859029648 accepts head
`768d530b4e140c9fc96bfc7ce13fe7a57c4b739c` for bounded continuation with no blockers.
Smoke 43022851916 passed; native 43022851847 remains running at inspection.
The graph witness combines allocation observations with domain telemetry; only
its expected structural counts are independent of that telemetry. Its initial
failure and comparison-formatting gap remain preserved. Integration is through #267.
**Provisionally accepted (#272):** ExchangeMap retained slot UUIDs
for occurrence study and field scope across owner/coordinator maps. Audit default
slot construction, explicit setup copies, reservation growth and assignment
before attaching payload; attachment after detached resize would be too late.
Preserve inactive-slot backing, allocation-free reuse/erase/swap, domain lifetime,
detached snapshots, refusal cleanup and exact retry. Other owner identities, comparison-formatting temporaries and
input payload remain separate before complete setup, combined resources and
operation/control integration.

**Provisionally accepted (#273):** retained projection input top-level
tables in StudyInput and StructuralStudyInput (nodes, definitions, occurrences,
selections, contacts, memberships and focal fields). Audit shared interfaces and
consumers before adopting allocator-aware storage; select the destination domain
before explicit owner copies. Preserve detached public snapshots, exact v1/v2
values and native handoff. Nested input arrays/text, comparison-formatting and
other setup payload remain separate before combined resources and operations.

**Dependency review (2026-11-11):** #272 review 5859234213 accepts head
`5ea60585234fbdea12a6f4441103d82884eb9417` for bounded continuation with no blockers.
Smoke 43026470580 and warm 43026470847 passed; native 43026470568 remains running
at inspection. Integrated acceptance remains through #267.
**Provisionally accepted (#274):** nested retained input arrays:
Definition components/parents and Selection parts. Preserve complete values,
detached public snapshots, domain lifetime and native handoff; attach storage
before explicit owner copies. Input text, other identities/comparison formatting,
full setup and combined resources remain separate before operation integration.

**Dependency review (2026-11-11):** #273 review 5859431800 accepts head
`8c142c3056736ba6a1a683b14ced8c79163a08ab` for bounded continuation with no blockers.
Smoke 43028610415 passed; native 43028610546 remains running at inspection.
Integrated acceptance remains through #267.
**Authorized text workstream after nested-input review:** retained input text in definition
IDs/parent values, occurrence IDs/definitions, selection aggregates/parts, contacts,
memberships and focal fields. Audit shared consumers; attach before copying, cover
inline/heap strings and terminators, preserve complete values and detached snapshots.
Other identities/comparison formatting, full setup and combined accounting remain
separate before operation/control integration.

**Dependency review (2026-11-11):** #274 review 5859635152 accepts head
`ce3642ee463beb76948a2497c9aeb50172c3267d` for bounded continuation with no blockers.
Smoke 43030712985 passed; native 43030713058 remains pending at inspection.
Integrated acceptance remains through #267.
**Provisionally accepted (#275):** retained input parent-value text.
The shared-consumer audit splits the text workstream into reviewable slices:
parent values first, preserving prepared public string outputs through explicit
conversions. Other input identifiers cross additional lookup/serialization paths.
**Authorized remaining text workstream:** remaining retained input text (definition and occurrence
IDs/references, selection aggregates/parts, contacts, membership and focal fields).
Audit the shared lookup and serialization consumers before adopting allocator-aware
strings; preserve exact values, detached snapshots and native handoff. Other
identities/comparison formatting, full setup and combined accounting remain separate.

**Dependency review (2026-11-11):** #275 review 5859805974 accepts head
`6bbae550f6e63a53f832a34449cf14008192e966` for bounded continuation with no blockers.
Smoke 43032906551 passed; native 43032906525 remains pending at review inspection.
Integrated acceptance remains through #267.
**Provisionally accepted (#276):** retained contact, membership and focal
field text, grouped by their shared preparation lookup boundary. Preserve public
lookup/output strings through explicit conversions and audit viewport callers.
**Authorized remaining work:** definition/occurrence IDs and references plus selection
aggregate/parts text. Audit public identity and serialization paths before adopting
allocator-aware strings. Other identities/comparison formatting, full setup,
combined accounting and operation integration remain separately inventoried.

**Dependency review (2026-11-11):** #276 review 5861623800 accepts head
`755595b21b3193c5a236c2c3c5c5fb5a30be1cf7` provisionally with no source blockers.
Smoke and native projection-values passed. Native 43034799670 failed before
product compilation on apt DNS resolution; viewport 43034798156 had the same
failure during Playwright dependencies and was cancelled before browser tests.
Maintainer requested retries successfully; preserve these failed/cancelled attempts.
Integrated acceptance remains through #267.
**Provisionally accepted (#277):** selection aggregate/parts input text,
plus requested membership.group.empty() cleanup avoiding an unnecessary copy.
**Active contribution (awaiting review):** definition/occurrence identity and reference text, with explicit preparation/public-output/viewport conversions. All 45 scoped local CPU checks passed; one post-PASS cache warning and both corrected initial failures remain in the admission evidence. This completes retained input text attachment, not all owner resources. Next: audit remaining owner identities/comparison formatting and setup work, then combined accounting and operation/control integration.


**CI dependency (2026-11-11):** #278 review 5864410183 accepts the source
correction at `07f2d66763631afe8d62438351bfd7ece1f149c8`; the maintainer inspected
native and viewport logs using archive.ubuntu.com after the mirror-list rewrite.
The full required hosted run and integration remain pending at inspection. After
integration, carry the repair into the #275–#277 stack and qualify the updated
code before merging. The identity contribution is based on #277 and does not
silently include the independent repair. Preserve earlier failed/cancelled runs.


**Combined-head audit (2026-11-11):** main `6067c641` is merged normally into
#279 at `a75e34084bed5fcfc24280b54111081d6440d58d`. The remaining-cost audit in
[OWNER-RESOURCES](../engine/docs/OWNER-RESOURCES.md#remaining-identitycomparison-and-setup-cost-audit-2026-11-11)
identifies comparison rendering, nil/temporary identity copies, by-value setup
arguments, graph recursion/callable costs and unmetered declaration/binding work.
Next concrete implementation: allocation-free ObjectKey ordering preserving
encoded ordering and invalid-address rejection, with an independent rendering
oracle and no-allocation controls. This audit does not change production behavior
or establish complete constructor/combined accounting.


**Ordering contribution (awaiting review):** #279 updated-head review 5865153858
accepts combined head `5f1ca0c3a6fdf3d4316ff861c521e92e15907dc1` provisionally
and authorizes allocation-free ObjectKey ordering. #279 is retargeted to main as
the integration PR; #275–#277 remain pending reconciliation after integration.
All 46 scoped local CPU checks passed, with one preserved post-PASS cache warning.
The ordering contribution preserves the rendered-key oracle and both-address
validation, with zero-allocation and malformed-input controls. Nil/containment,
temporary copies and traversal/setup accounting remain separate subsequent work.


**Validation contribution (awaiting review):** #280 review 5865274115 accepts
head `5e37be82a547dd4e88371cb927bea3effa3bb6e4` provisionally and authorizes
nonallocating nil/containment validation. The contribution removes their temporary
UUID/key rendering, preserving malformed-address rejection and containment
semantics with independent allocation and rendered-prefix controls. Both builds,
47 scoped CPU checks and two prior-source negative controls passed without warnings. Subsequent
scope remains detached identity/argument copies, membership callable/recursion
and declaration/binding setup work before combined accounting and operations.
#279 remains the main-targeted text-stack integration PR; queued hosted jobs do
not qualify integration and dependent PRs stay open until disposition.


**Adoption-copy contribution (awaiting review):** #281 review 5865407113 accepts
head `15db4b816456fb9ff299801daac42e1624b50ead` provisionally and authorizes
detached temporary identity/argument-copy work. Owner/binding constructors now
borrow row/field inputs during construction, retaining independent selected maps;
the temporary aggregate declaration UUID is explicitly reserved. Caller
preservation/source-death and existing refusal/retry/lifetime controls cover the
boundary. Remaining temporary identities and callable/recursive traversal plus
declaration/binding setup budgets follow before combined resources and operations.

**Projection temporary-identity contribution (awaiting review):** #282 review
5865626048 provisionally accepts `29b82129` with no blocking findings. Private
byte-expansion rows now borrow pinned character/byte references while source tools
remain stable; published outputs retain independent ownership. The direct probe
rejects prior-source UUID copies. Remaining initial projection indexes/provenance
and rendering temporaries need audit before claiming this boundary complete;
membership callable/recursion and declaration/binding setup budgets remain next.
No stack merge is inferred from provisional review or local evidence.

**Remediation and rendering continuation (2026-11-11):** #283 review 5866277807
reports no source blocker and green exact-head native/smoke; #280–#282 are also
green. #279's old runs were stuck queued; cancel/force-cancel returned 409 and
rerun 403. Close/reopen preserved its reviewed head and triggered native
43115507493, viewport 43115507480 and smoke 43115507510. Old attempts remain
preserved. Integration still requires new qualification and maintainer disposition.
The next bounded change removes temporary OccurrenceKey construction from initial
projection rendering, sharing unchanged validation with the public formatter.
Remaining indexes/provenance/equivalence and traversal/setup budgets remain open.

**Provenance/equivalence continuation (awaiting review):** #284 review 5866703067
provisionally accepts `09bafe48` with no blocker. Temporary initial-projection
lookup values now borrow pinned ToolRefs from stable input tools and retain only
owner entry/ordinals; published records still copy owned identities. Source-death
checks compare full references/revisions. Initial tool/occurrence indexes and
publication copies/setup remain separate; membership callable/recursion and
declaration/binding setup budgets precede combined resource/operation integration.
#279 fresh native qualification remains in progress; smoke/viewport are green.
No pending stack merge is inferred.

**Tool-index continuation (awaiting review):** #285 review 5866915489 accepts its
source provisionally at `7105c22e`; hosted native remains separate. Current main
`c2c168b3` is included by normal merge `9f88ce31`, preserving the integration-through-
#284 record and #285 evidence. Only four Markdown files differ in that merge.
The initial Tools index now borrows stable source ObjectKey values with the same
value comparator; separately copied lookup keys and duplicate source orders are
checked. Initial occurrence indexes/publication setup and membership callable/
recursion plus declaration/binding budgets remain before combined operations.

**Occurrence-index value continuation (awaiting review, 2026-11-12):** starts from
main `35814207` after accepted integration through #286. The ordered index keeps
its owned key but replaces duplicated StructuralOccurrence identity values with
stable source definition references and node metadata. Both recipe-version source-
death tests check complete published definitions/revisions and nodes. Owned index
keys, legacy/used-definition lookups and publication/setup costs remain separate,
followed by membership callable/recursion and declaration/binding budgets before
combined resources/operations. No prior accepted baseline is rerun merely to onboard.

**Legacy lookup continuation (awaiting review, 2026-11-12):** #287 review 5874475520
provisionally accepts `291e721b` with no blocker. The temporary ordered occurrence
value now carries the legacy source-row pointer directly, eliminating the second
legacy map and its duplicate keys. V1 source-death handoff checks distinct exact
positions/velocities and occurrence association; structural output remains independent.
Remaining owned occurrence/used-definition keys and publication/setup costs precede
membership callable/recursion, declaration/binding budgets and combined operations.
Main remains `35814207`; #287 is unmerged pending full hosted qualification.

**Used-definition lookup continuation (awaiting review, 2026-11-12):** #288 review
5874675639 permits bounded continuation at `8772552d`. The temporary set now borrows
stable definition keys from const input occurrence rows using value ordering;
publication still creates independent definitions. V1/v2 source-death tests compare
published IDs with an independent encoded-key set, covering deduplication/order.
The local source comparison removes 12 UUID-sized requests and 24 total allocation
calls for 12 distinct used definitions; public layouts are unchanged. See admission
for source-pinned qualification and limits. Main remains `35814207`; #287/#288 are
unmerged, with #288 smoke passed and native running at inspection. Remaining owned
occurrence-index keys and publication/setup costs precede membership callable/
recursion, declaration/binding budgets and combined resources/operation integration.

**Stable occurrence-key continuation (awaiting review, 2026-11-12):** #289 review
5874861380 permits bounded continuation at `34db84f1`. Ordered-occurrence keys
borrow IDs in source occurrence rows; provenance keys borrow selection part IDs.
The comparator preserves value order and lookup across distinct equal objects.
Synthesized aggregates are lookup arguments only in the changed ordered map;
active/equivalence indexes still own their keys. V1/v2 source-death controls reverse
input rows and verify published occurrence order. The local source comparison
removes 26 path-sized allocations over 13 byte rows, with unchanged public layouts.
See admission for qualification. Main remains `35814207`; #287–#289 are unmerged.
Next: active/equivalence synthetic-key lifetime and publication/setup costs, then
membership callable/recursion, declaration/binding budgets and combined resources/
operation integration.

**Selection-metadata continuation (awaiting review, 2026-11-12):** #290 review
5875081581 directs consolidation into the existing ordered metadata. Each row starts
inactive with no complete-character pointer; selection processing resolves aggregate/
part IDs by value, sets activity and attaches the stable single-parent ToolRef even
for inactive aggregates. Publication copies it into owned optional output. The
active/equivalence containers are removed; provenance remains unchanged. Local
source comparison records 8/28 fewer calls for aggregate/mixed exposures, while
ordered nodes grow 64→72 bytes. See admission for complete qualification and limits.

The remaining initial-publication audit is bounded: inspect the local
ProjectedOccurrence record copied into occurrences_ and the reusable OccurrenceKey
k; distinguish redundant staging copies from independently owned output identities.
Definition/Selection staging already moves. Retained recipe_, structural input,
legacy handoff and parent records have independent ownership requirements and must
not silently become views. Stop expanding initial-projection cleanup once this
boundary is explicit. Then membership callable/recursion and declaration/binding
setup budgets precede combined resources and operation/control integration;
projection allocation reductions do not complete the owner setup gate.

**Publication-copy boundary (awaiting review, 2026-11-12):** #291 review 5875329810
permits continuation at `f340c720` and confirms #287/#288 integrated at `b234514d`.
Normal merge `7cb31173` includes main without changing the tree. The occurrence
record constructs its owned key directly; a const reference to that local key serves
input text and parent construction before the completed record moves into output.
No moved-from read or output-vector reference crosses reallocation. V1/v2 source-
death tests check every parent key, field, ordinal and count. Source comparisons
remove 177 calls in both exposure modes; public and ordered-node layouts are unchanged.
See admission for qualification. #289–#291 remain unmerged; review/hosted checks apply.

**Initial-projection audit stop:** Definition and Selection staging already moves;
occurrence staging now moves after all consumers. Recipe, occurrence/provenance,
parent and v1/v2 public inputs require independent ownership; those stores and
remaining temporary lookup machinery are not reopened as a new cleanup sequence.
The next concrete owner-setup task is MembershipFields::collect's type-erased
recursive follow callable and traversal/stack budget. Preserve pinned-reference
checks, cycle detection, memoized reached sets, diamonds/repeated references and
exact contribution ordering/ownership. Its existing allocation domains are not a
complete traversal/setup limit. Then finish declaration/binding construction budgets,
combined resources and remaining-operation/control integration. Do not label these
initial-projection allocation reductions as completion of the owner setup gate.

**Bounded membership traversal (awaiting review, 2026-11-12):** #292 review
5875540117 authorizes callable storage and traversal limits together. Accepted main
`a4de248e` through #289 is included by normal merge `7be5b888`, with no file delta.
MembershipFields::collect now uses explicit DFS frames allocated through the existing
domain, bounded by tool inventory and the domain's byte/allocation-work allowance.
Owner adoption supplies its PreparationWork meter and reports a Membership phase.
Comparisons charge key payloads first; visits, memoized pin validation, insertion,
ancestor merging, owner lookup and output/relocation work are admitted before execution.
Cycle/memoization state and deterministic target order are retained. Standalone
collection and standalone bindings retain unrestricted work allowances; their paths
are not claimed as owner admission. Existing allocation domains remain attached.

Focused tests sweep all work and allocation refusal boundaries, teardown and exact
retry, check stale memoized pins/cycles, and traverse a 192-level chain with repeated
terminal reachability. The initial deep fixture used invalid private dependencies;
that failure is preserved and only fixture identities were corrected. The owner
setup sweep now covers Membership alongside the existing phases. See admission
for source pins, actual resource changes and qualification; this is not a timing or
physical-memory guarantee. Next: declaration collection and ExchangeBindings setup
work, then combined accounting and operation/control integration. The projection
copy audit remains closed and the complete owner setup gate remains outstanding.

**Declaration setup admission (awaiting review, 2026-11-12):** #293 review 5875958258
permits this bounded continuation. LetterExchangeContract::collect receives the
owner's setup meter, admitting scalar selection scans, occurrence-path/tool-reference
comparisons, source/parent/leaf copies, growth/relocation and match-work accumulation.
Exposure lookup is conservatively prepaid for the full selection count, and its
current owner-local value is reused. Reverse declaration copying and retained
declaration-table publication are metered as well. Declarations is a distinct owner
setup phase. Standalone collection/copies remain unrestricted and existing later
matches/capture allowances are not repurposed. See admission for qualification.

The declaration continuation also repairs #293 hosted projection-values job
131239777540 (native run 43234968384): GCC 11 exposed partial-allocation cleanup
failures hidden by the local Clang build. Named lifetime owners now precede nested
aggregate publication. Initial failure evidence is retained, and standalone GCC 11
and GCC 13 checks accompany the combined native CPU qualification. See admission
for final pins/results; this correction does not reopen the projection copy audit.

Next concrete slice: ExchangeBindings construction (buffer reservation, trusted/staged
maps and reserve_exchange_maps), then stage/capture under the shared owner setup
allowance. Replace owner-path UINT64_MAX gaps while keeping separately budgeted
operational capture and matching behavior intact. Exact refusal, cleanup, retry and
unchanged caller/native state controls remain mandatory. Declaration and membership
admission are scoped gates, not complete constructor or combined-resource acceptance.
Projection cleanup remains closed; operation/control integration follows the budgets.

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

Before publication, refresh applicable security fixes in builder/runtime and
PostgreSQL images, record base digests/package inventories and scan findings,
and validate the patched artifacts under the [release security requirement](onboarding/RELEASE.md#security-updates-before-publication).

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

**Superseded sequencing, 2026-11-08:** Patrick authorized layered onboarding while
the coding agent continues. The [onboarding plan](onboarding/ONBOARDING-PLAN.md)
moves status and detailed contracts into focused files, keeps universal rules in
root AGENTS and preserves existing anchors and history. This changes documentation
sequencing, not the engine task or qualification. The earlier decision follows
as historical context.

Reviewed 2026-11-07: defer broad AGENTS.md restructuring until after the engine
PoC release; correct misleading guidance as it is found. Keep the current queue,
authority/reading order and repository-wide invariants in AGENTS.md. Consolidate
detailed numerical, placement and model requirements into their owning contracts,
with explicit required-reading links for affected work. Preserve dated decisions,
acceptance evidence and existing link anchors; historical records explain the
development journey and are not disposable onboarding material. Verify that the
shortened entry point still leads new contributors to every applicable constraint.
This documentation work does not block the PoC or replace engine qualification.

### Hosted infrastructure follow-up — 2026-11-11

Review #277/5863228338 authorizes a separate bounded investigation of repeated
package-mirror DNS failures. Main `05635f07` reconciles accepted #268–#274; #275–#277
remain provisional and unmerged. The active CI-only repair replaces exact Azure
Ubuntu archive URLs with the primary archive before existing native/viewport
package installation, preserving signing configuration, toolchain versions and
acceptance gates. Local mirror and orchestration checks pass; hosted qualification
remains required. See [evidence and limits](../engine/docs/HOSTED-CI.md#ubuntu-archive-resolution-repair-2026-11-11-awaiting-review).
Definition/occurrence identity text remains queued after this independent repair;
no allocation implementation is changed by the infrastructure contribution.
