# Current implementation and work

Read when selecting work, reviewing acceptance or asking what is built. For the next bounded task, use the [continuation handoff](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). You do not need the conceptual introduction first.

## Current state

**Maintainer acceptance — 2026-11-12:** #285 and #286 are integrated into main at `087b6aff14d454c9ebafcda28c9e9243f596e4d2`, reviewed tip `49643f2a22bbb0076584e01d990a32ff7fe0acda`. Exact-head #286 native 43130677310 and smoke 43130677322 passed; #285 native 43121173964 and smoke 43121173792 also passed. All 12 #286 native jobs succeeded. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 130894721312 reports addressed_reads=24/private_reads=0. At that integration the active review queue was clear; #75, #111 and #123 remain deliberately held. Earlier provisional/pending labels below are historical and superseded by this acceptance. No GPU or performance qualification is implied.

**Integration / active publication contribution (2026-11-12):** #287/#288 are green
and integrated through #288 at main `b234514d`, per #291 review 5875329810; their
older pending labels are historical. #291 `f340c720` has no blocking finding and
permits continuation. Main is included by normal merge `7cb31173` (no tree change).
Occurrence publication now constructs its owned key in the local record, consumes
text/parent/activity reads, then moves the finished record into output. Source-death
checks verify every parent association. Both builds and 47 scoped CPU checks passed
without warnings; see admission for qualification. #289–#291
remain unmerged and this contribution needs hosted checks/review. #291 smoke
43228339158 passed; native 43228339116 was running at review.

The bounded initial-projection copy audit ends here: public recipe, identity,
provenance, input and parent storage remains owned. Next: MembershipFields::collect
recursive callable/traversal costs, then declaration/binding setup budgets, combined
resource accounting and operation/control integration. This does not complete the
owner setup gate.

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
| Latest accepted engine evidence | Integrated through #286 at `087b6aff`; reviewed tip `49643f2a`. Native 43130677310 and smoke 43130677322 passed. Inspected 115 regression tests and addressed_reads=24/private_reads=0. |
| Accepted exchange owner | Recorded scopes through #286 integrated, including retained input text, comparison/validation and adoption-copy reductions, plus initial projection expansion/rendering/provenance/tool-index improvements. Complete setup and combined resources remain open. |
| Next implementation | Publication-copy boundary on provisionally accepted #291, including main b234514d through #288. Next MembershipFields::collect callable/recursion, declaration/binding setup budgets and combined resources/operations. #289–#291 remain unmerged; this contribution needs hosted qualification and review. |
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

**Immediate work:** initial occurrence-index and publication/setup audit from integrated main through #286. Preserve independent outputs, source lifetimes and refusal/retry guarantees.

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


