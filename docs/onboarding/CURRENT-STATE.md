# Current implementation and work

Read when selecting work, reviewing acceptance or asking what is built. For the next bounded task, use the [continuation handoff](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). You do not need the conceptual introduction first.

## Current state

**Maintainer integration — 2026-11-12:** #300's borrowed-resource audit is accepted and integrated by the merge containing this record, preserving `e540a8712c176c06d2f5729da2aa6602d67ec4cb`. Native 43276619607 passed all 12 jobs; smoke 43276619735 passed. Personally inspected 115 passing regression tests (29/29/29/28) and database job 131388302021 with addressed_reads=24/private_reads=0. Main includes #299 owner bootstrap admission and now the [borrowed-resource audit](../../engine/docs/BORROWED-RESOURCES.md). #301 CPU record interface remains provisionally accepted at `63a46aab` (review 5880096613), and #302 array/root exports at `aa48a2e4` in [review 5880581671](https://github.com/Human-Cognome-Project/human-cognome-project/pull/302#pullrequestreview-5880581671). Both remain unmerged with native qualification running; their smoke checks passed. Local archives remain contributor-reported, not independently inspected. Neither a complete B ledger nor combined admission is built.

**Next bounded task:** prepared-invocation backing exports and node/list/ambient reservations on reviewed #302. Preserve explicit required-family coverage, owner lifetime and request/rounded extent distinctions. Resolve imported registrations to actual backing owners before deduplication; account runtime metadata once. Missing coverage or unresolved ownership keeps the complete ledger unavailable. Then assemble the trusted lifetime-valid Harness snapshot, enforce checked C+B+H+D with separate work gates and complete remaining operation/control integration. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #299 is accepted and integrated by the merge containing this record, preserving reviewed head `5771326e4992889c3b9c16b8b98ee752ae4d0d1a`. Native 43274126767 passed all 12 jobs; smoke 43274126747 and warm 43274126853 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131379495998 reports addressed_reads=24/private_reads=0. #298 is already merged at `319f892a`. #300 audit is source-accepted at `e540a871` under review 5879635043 and #301 common/CPU allocation interface is provisionally accepted at `63a46aab` under [review 5880096613](https://github.com/Human-Cognome-Project/human-cognome-project/pull/301#pullrequestreview-5880096613); both remain unmerged with native qualification active. Local archives remain contributor-reported, not independently inspected. Main now includes owner bootstrap admission, not the pending native record interface or complete B ledger.

**Next bounded task:** actual native owner backing exports on reviewed #301, including array/tree-root, node/list and prepared-launch families. Resolve imported registrations to actual owners before deduplication; record identity alone is not backing identity. Preserve explicit coverage and unavailable status, lifetime validity and retained metadata accounting. Then assemble the trusted Harness snapshot, enforce checked disjoint C+B+H+D admission and complete remaining operation/control integration. HCP model code remains backend-independent. Initial-projection copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next labels below are historical where they conflict.

**Previous disposition (historical):**

**Maintainer disposition — 2026-11-12:** #298 is accepted and merged at `319f892a1aa35e29597e89e461c0512eba233e33`. Native 43268613665 passed all 12 jobs; smoke 43268613606 and warm 43268613622 passed. Personally inspected regression logs: 115 passing tests (29/29/29/28); database job 131360157711 reports addressed_reads=24/private_reads=0. #299 owner integration is provisionally source-accepted at `5771326e` (review 5879484896), but native qualification remains active and it is unmerged. #300's five-file documentation audit is accepted at `e540a871` in [review 5879635043](https://github.com/Human-Cognome-Project/human-cognome-project/pull/300#pullrequestreview-5879635043); smoke passed, native is active, and it remains unmerged. Local probe archives are contributor-reported, not independently inspected.

**Next bounded task:** implement the common native allocation-record contract and CPU implementation/tests authorized by #300: original request versus rounded extent, live/retired/unknown status, explicit import ownership and validity across device lifetime/reset/reuse. Unsupported coverage is unavailable, never zero; HCP model code consumes one interface. Then export actual owner backing (including tree/node/list and prepared-launch families), assemble the trusted Harness ledger and enforce checked combined C+B+H+D admission. Resolve aliases and pool/suballocation overlap at the owning boundary; include new ledger storage in the policy and keep backing validity distinct from publication generation. Do not query dead device pointers to test expiry. This is scoped accounted-byte work, not a process-wide profiler. Remaining operation/control integration follows. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #295–#297 are accepted and integrated by the merge containing this record, preserving original commits through `272e160d56201fd1f93353c5ba22efd88f0ef69d`. #297 review [5878815959](https://github.com/Human-Cognome-Project/human-cognome-project/pull/297#pullrequestreview-5878815959) found no blocking source defect. Native 43252923816 passed all 12 jobs; smoke 43252923333 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131305209894 reports addressed_reads=24/private_reads=0. #295 native 43245334785/smoke 43245334673 and #296 native 43249326537/smoke 43249326530 also passed. Local archives remain contributor-reported, not independently inspected. This accepts binding setup, the accounting inventory and checked initial-map work prepayment, not complete owner-resource, GPU or performance qualification.

**Next bounded task:** track actual domain bootstrap request bytes with an outer tracker whose lifetime safely covers domain/control-block destruction, and establish a trusted disjoint borrowed-resource ledger. Then enforce combined accounted-byte admission using those disjoint components and selected live requests; do not sum overlapping snapshot, retained, peak or cumulative-work counters. Preserve checked arithmetic, pre-allocation refusal, reclamation, native state and exact retry controls. Remaining operation/control integration follows. The projection copy audit is closed. Additional setup work remains acceptable for long-running focal models; reducing setup totals is not an acceptance gate. #75/#111/#123 remain deliberately held/deferred.

**Owner bootstrap integration (awaiting review, 2026-11-12):** continues from
#298 review 5879152987 on `386be0a0`; #298 remains open while hosted qualification
finishes. Owner admission now gates actual outer tracker/domain control-block
requests and reports them separately from selected owner requests. Both bootstrap
boundaries precede owner construction/native mutation. Later constructor refusals
and allocation exceptions release bootstrap storage while preserving prepaid work;
selected weak references retain the tracker through final release. See [adoption accounting](../../engine/docs/ADOPTION-ACCOUNTING.md#owner-bootstrap-integration-awaiting-review).
Next: the trusted disjoint borrowed-resource ledger, then combined accounted-byte
admission and remaining operation/control integration. Both builds and all 47 scoped CPU checks passed without warnings; this new
integration evidence is recorded separately from the accepted helper evidence.
**Bootstrap tracker prerequisite (historical contribution, 2026-11-12):** a new shared
native helper accounts for its own outer allocation and the actual rebound
reservation-domain/control-block request, with lifetime through the final weak
release. It keeps bootstrap and selected requests disjoint. See
[shared support](../../common/README.md#domain-bootstrap-tracker-awaiting-review) for the contract and standalone refusal/unwind/lifetime coverage.
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
[The source audit](../../engine/docs/ADOPTION-ACCOUNTING.md) maps all adoption operations after #295 and
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

**Active declaration setup contribution (2026-11-12):** #293 `b0f2f289` is
provisionally accepted by review 5875958258. Main remains `a4de248e`, integrated
through #289; #290–#293 are unmerged. Declaration collection and reverse-copy work
now share the owner setup meter under a distinct Declarations phase. Selection,
occurrence/tool searches, payload comparisons/copies, source/leaf accumulation,
relocation and declaration publication are admitted before work. Standalone paths
remain unrestricted. This continuation also corrects GCC 11 partial-allocation
cleanup failures exposed by #293 hosted projection-values job 131239777540; the
original failed run is preserved. Standalone GCC 11/13 checks now supplement the
native CPU qualification. Both builds, 47 scoped CPU checks and six standalone
compiler CTests passed; one post-PASS cache-lock warning is preserved. See admission
for source-pinned qualification.

Next: ExchangeBindings constructor reservation/copies, staging and capture under
the shared setup allowance, replacing owner-path UINT64_MAX gaps while preserving
separate later operational capture/matching budgets. Combined accounting and
operation/control integration follow. Projection cleanup stays closed; this is
not the complete owner setup gate. Hosted qualification/review remain required.

**Active tool-index contribution (2026-11-11):** continues from source-accepted
#285 `7105c22e` under review 5866915489, with accepted main `c2c168b3` included by
normal merge `9f88ce31`. Temporary normalization/projection tool-index keys borrow
stable source objects and retain value ordering/validation. A focused prior-source
probe records 22→0 UUID-sized requests and validates independent lookup keys;
source-death and duplicate-order controls remain active. Both builds and 47 scoped
CPU checks passed; one post-PASS cache warning is preserved. Detailed qualification
is in the admission record; hosted qualification/review remain pending. Next:
initial occurrence indexes/publication setup, then membership callable/recursion,
declaration/binding budgets and combined resource/operation integration.


**#285 qualification update (2026-11-11):** Patrick has accepted the contribution;
exact-head native 43121173964 and smoke 43121173792 now both report success.
The PR remains open at inspection; no merge is inferred. Earlier pending labels
are historical. This does not qualify the new tool-index continuation.

**Maintainer integration — 2026-11-11:** Reviewed #275–#277 and #279–#284 are integrated into main at `5beeb2cacf28c75cbecef385a0de1e877154bbc0`, preserving original commits through reviewed tip `09bafe48869ddf239e6e245d766c0c0e41a43364`. #279's fresh native 43115507493, viewport 43115507480 and smoke 43115507510 succeeded; #280–#284 completed their applicable hosted checks, including #284 native 43116769390 and smoke 43116769371. Original failed and stuck queued attempts remain historical, not relabeled. Earlier pending/integration labels below are superseded by this record. #285 is provisionally source-accepted at `7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` (review 5866915489), targets main and awaits native qualification; its smoke passed. Next: initial tool/occurrence indexes and publication/setup costs, then membership callable/recursion, declaration/binding budgets, combined resource accounting and operation integration. No blanket GPU or performance acceptance.


**Active rendering-copy contribution (2026-11-11):** #283 `38e81a95` has no blocking
source finding and green exact-head native/smoke per review 5866277807. This
continuation removes owning occurrence-key temporaries used only for initial
projection text rendering; public validation and independent output remain intact.
Both builds and 47 scoped CPU checks passed; a post-PASS cache warning is retained.
Detailed evidence is in the admission record; hosted qualification/review remain
pending. Initial projection indexes/provenance/equivalence, membership callable/
recursion and declaration/binding setup budgets precede combined resources and
remaining-operation integration.

**Active provenance/equivalence contribution (2026-11-11):** #284 `09bafe48` is
provisionally accepted with no blocker by review 5866703067. This continuation
borrows stable source ToolRefs in temporary projection lookup values, constructing
independent published identities at consumption. Source-death tests compare full
pinned references; a source-to-source probe records 45 fewer UUID-sized requests.
Both builds and all 47 scoped CPU checks passed without warnings; detailed evidence
is in the admission record. Hosted qualification/review remain pending. Initial tool/occurrence indexes and publication setup, membership callable/
recursion and declaration/binding setup budgets precede combined resource/operation
integration. #279 remains unmerged; its fresh native matrix is running while
smoke/viewport are green. #284 smoke 43116769371 is green and native 43116769390
was running at maintainer review.

**#279 queue remediation (2026-11-11):** old native/viewport/smoke attempts remained
queued, yet cancel/force-cancel returned 409 and rerun 403. Close/reopen triggered
fresh native 43115507493, viewport 43115507480 and smoke 43115507510 at unchanged
reviewed `5f1ca0c3`; old attempts remain preserved. Smoke and viewport passed at inspection;
native had passed build/projection-values and was running its matrix. #280–#283 green checks do not replace #279's
combined-head qualification. The stack remains unmerged pending maintainer action.

**Provisionally accepted #282 (2026-11-11):** review 5865626048 accepts the adoption
argument-copy slice at `29b82129`. Its 47 scoped CPU checks passed locally; the
initial obsolete-assertion failure is preserved. Maintainer inspection reports
smoke 43104595184 green, native 43104595076 running; #280/#281 native still running
and #279 latest-head workflows queued. These are not integration acceptance.

**Provisionally accepted #281 (2026-11-11):** review 5865407113 accepts the
nil/containment source provisionally at `15db4b81`, with no blocking findings.
Its 47 scoped CPU checks and two prior-source negative controls passed locally.
At inspection, smoke 43101300624 and warm 43101300677 succeeded; native
43101300728 was queued. These states do not imply integration or qualify the
new adoption-copy head.

**Provisionally accepted #280 (2026-11-11):** allocation-free ObjectKey ordering on
provisional #279 head `5f1ca0c3`, authorized by updated-head review 5865153858.
Both-address validation and the rendered-key ordering oracle are preserved.
All 46 scoped local CPU checks passed; a post-PASS cache warning remains recorded.
Awaiting review and hosted qualification. Next: nil/containment validation and
temporary identity copies, then traversal/setup budgets and combined resources.
#279 is the main-targeted integration PR for the pending text stack; dependent PRs
remain unmerged until its hosted qualification and maintainer disposition.

**Maintainer reconciliation — 2026-11-11:** #278 is accepted and merged at `3cadbff286b0c4553808595f624f8a98add36682`, reviewed head `07f2d66763631afe8d62438351bfd7ece1f149c8`. Native 43087248989, smoke 43087248327 and viewport 43087248550 passed. Inspected native and viewport installation logs confirm the mirror-list rewrite and archive.ubuntu.com traffic. The earlier pending labels are historical. #279 identity/reference input text is source-reviewed and provisionally accepted at `11a4813759c1e384fa6b077499c9f06998c66f58` (review 5864859891); smoke/viewport are green and native matrix qualification is pending. #275–#277 and #279 remain unmerged. Current main at 6067c641 is brought into this stack by a normal merge; the combined head a75e3408 passed 45 scoped local CPU checks plus mirror/CI contract checks. Updated-head hosted qualification and maintainer integration remain pending. Next bounded work: remaining identity/comparison-formatting and membership traversal/callable audit, then complete setup, combined accounting and operation/control integration.


**Queue reconciliation — 2026-11-11:** Reviewed #268–#274 are integrated into main at `b05db31c2e591f8fbcd5c14dc7ddff14ef46b2e9`, reviewed tip `ce3642ee463beb76948a2497c9aeb50172c3267d`. Every head in that stack has successful native and smoke workflows; #272 warm also passed. #274 native 43030713058 passed all 12 jobs; inspected regression logs total 115 tests and database reports addressed_reads=24/private_reads=0. Earlier provisional/pending labels below are historical and superseded by this acceptance.

**Earlier review queue (preserved history):** #275 parent-value text, #276 contact/membership/focus text and #277 selection text are source-reviewed and provisionally accepted, but remain unmerged. Inspected native jobs 130564239034 (#275), 130638929459 (#276 retry) and 130648619590 (#277) failed during toolchain installation because azure.archive.ubuntu.com could not resolve, before product compilation. #276 viewport retry job 130638930090 was cancelled during dependency installation with the same DNS failures. Preserve these attempts; complete successful hosted qualification before integration. The next coding slice is definition/occurrence identities and references, per #277 review 5863228338. Other identities/comparison formatting, complete setup, combined accounting and operation/control integration remain open. #75, #111 and #123 remain deliberately held.

**Earlier infrastructure follow-up (now accepted in #278):** normalize the affected native/viewport runner APT sources to the primary Ubuntu archive, preserving authentication, pinned toolchain and gates. Local mirror fixtures, ShellCheck and orchestration contracts passed; hosted qualification remains required. See [hosted CI](../../engine/docs/HOSTED-CI.md#ubuntu-archive-resolution-repair-2026-11-11-awaiting-review). Definition/occurrence identity text remains the next coding slice.

C2b-1 is accepted in #192 under the [#191 plan](../../docs/cumulative-publication-integration-plan.md). The [C2b-2 weighted native probe](../../engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted in #193 with [source-pinned evidence](../../engine/tests/weighted_handoff/README.md). It qualifies held-input numerical propagation, not production hierarchy or inherited response.

The [production hierarchy/response plan](../../docs/production-hierarchy-response-plan.md)
is accepted in #194. It specifies comparison and force ownership, production
reader/history lifetimes, combined integration and coupled recovery. Its C3-1
fixed-tree implementation is accepted in #195; C3-2 coherent refresh is accepted
in #198. B1 contact-safe ticks are accepted in #201; private admission/bootstrap and atomic exchange are accepted through #210/#219.

The [accepted C3-1 implementation](../../engine/docs/LIVE-HIERARCHY.md) now supplies
that opt-in fixed-tree CPU path. Its executable acceptance now follows the
[coherent-dynamics direction](../../docs/coherent-dynamics-gpu-snapshots.md); see the separately pinned
[revised observations and original investigation](../../engine/tests/live_hierarchy/README.md).
The active-capture adapter is accepted. #196 supplies scoped local GTX 1070
evidence; full integrated CUDA qualification remains open.

This file is the maintained status entry point. Code and executable evidence determine
what is built; accepted decisions govern intended behavior. A conflict between
them and this summary is drift to investigate, not permission to silently pick
one. Check [open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls)
and latest maintainer dispositions before implementing. Update this section when
a merge or review changes Built/Held/Active status; other entry points link here.
Keep this table and the immediate task current rather than adding another milestone
row for every PR. Put detailed run/commit evidence in the owning acceptance record.
When the next task changes, reconcile this status and the continuation task
in the same documentation change. Preserve provisional versus merged acceptance.

| State | Current scope and authoritative evidence |
|---|---|
| Accepted data and preparation | Retained [letter/core snapshots](../../data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md), native [warm tools](../../kernels/database/warm/README.md), coordinate-free [v2 recipes](../../kernels/database/warm/STRUCTURAL-RECIPES.md), [projection](../../engine/docs/LETTER-PROJECTION.md) and prepared native backing/updates. Preserve existing stores; population and operational ownership are separate. |
| Accepted CPU numerical path | Cumulative publication, [accepted motion](../../engine/docs/ACCEPTED-MOTION.md), [installed hierarchy](../../engine/docs/LIVE-HIERARCHY.md), coherent [refresh](../../engine/docs/LIVE-REFRESH.md) and [contact-safe ticks](../../engine/docs/CONTACT-SAFE-TICKS.md), accepted through #198/#201. Scope-specific GPU evidence does not qualify the complete path. |
| Accepted private operations | Certified placement (#203), ordinary admission/bootstrap (#210), live focus (#211), exact full/reduced f64 rebase (#212/#213), and source-bound mass/momentum/placement accounting with [atomic exchange](../../engine/docs/ATOMIC-EXCHANGE.md) through #219. These are bounded private primitives; an enduring operational owner/public Add API is not accepted. |
| Latest accepted engine evidence | Integrated through corrected #294, reviewed head `32fe9928`; all 12 native jobs and smoke passed. Inspected 115 regression tests and addressed_reads=24/private_reads=0. |
| Accepted exchange owner | Scopes through #297 integrated: binding setup, accounting inventory and checked initial-map work prepayment. Combined admission remains open; projection-copy audit stays closed. |
| Next implementation | Prepared-invocation and node/list/ambient exports on reviewed #302, trusted Harness ledger, combined admission and operation/control integration. |
| Factoring follow-up | #230 completes the first mechanical kernel extraction under [#229](https://github.com/Human-Cognome-Project/human-cognome-project/issues/229). Keep #229 open; further separation follows the resource/lifetime inventory. Layered onboarding is authorized on 2026-11-08; see the [migration plan](ONBOARDING-PLAN.md). |
| Accepted tooling | [Local evidence runner](../../engine/docs/LOCAL-CI.md) (#204), [parallel hosted CI](../../engine/docs/HOSTED-CI.md) (#206), and optional [offline viewport](../../tools/viewport/README.md). #220's AJV 8.18.0 fix is merged: hosted 15 model tests, reproducible bundle and 125 browser checks passed. Observation is not engine control or proof of natural clustering. |
| Outstanding qualification | #156 owns integrated operation acceptance; #145 owns actual GPU, transfer/pinning and resource qualification. Scoped [GTX 1070 evidence](../../engine/docs/CUDA-LOCAL-VALIDATION.md) is not blanket GPU acceptance. Mixed-precision repair, complete operation/hardware matrix and [Docker release](../../docs/onboarding/AGENT-START.md#intended-local-study-and-release) remain separately scoped. |
| Held/deferred | #111 and #123 remain held; draft #75 remains deferred. They do not block separately accepted core work. Analyst functions are conceptually designed within the overall architecture; their implementation structure and calibration await engine results on real data structures. Dynamic LoD selection and process-failure hosting remain unbuilt. |

Detailed milestone pins and their historical limits are preserved in the
[2026-11-05 acceptance snapshot](../../docs/history/agent-acceptance-2026-11-05.md)
and [earlier history](../../docs/history/acceptance-through-201.md).
Do not copy historical “next” instructions back into this table.

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>

## Active and deferred work

**Immediate work:** remaining prepared-invocation and node/list/ambient exports under #302 review 5880581671, then trusted borrowed accounting. Preserve refusal/retry, reclamation and native authority.

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


