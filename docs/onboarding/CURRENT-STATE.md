# Current implementation and work

Read when selecting work, reviewing acceptance or asking what is built. For the next bounded task, use the [continuation handoff](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). You do not need the conceptual introduction first.

## Current state

Last checked: **2026-11-10**, accepted integrated CPU implementation through #246 (reviewed head `8c0468168e3837a45e0526c51dce55707a2f81c3`). Native and smoke gates passed. Both retained projections' four table stores now have accepted allocation limits. #245's projection setup work gate and #246's membership contribution-table/temporary graph-node allocation limits are accepted together. Declaration source/leaf table limits in #247 are provisionally accepted. Active contribution: nested declaration parent-vector allocation limits. Next: nested key/recipe/input payload, preceding catalogue/declaration construction and graph-temporary allocation/setup limits, then combined caller/borrowed resource accounting, then remaining-operation integration and same-execution controls. C2b-1 is accepted in #192 under the [#191 plan](../../docs/cumulative-publication-integration-plan.md). The [C2b-2 weighted native probe](../../engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted in #193 with [source-pinned evidence](../../engine/tests/weighted_handoff/README.md). It qualifies held-input numerical propagation, not production hierarchy or inherited response.

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
| Latest accepted engine evidence | #246 head `8c0468168e3837a45e0526c51dce55707a2f81c3`: native 42665443526 and smoke 42665443451 passed. Inspected logs cover 115 regression tests and 180 database checks, addressed_reads=24/private_reads=0; all compiler jobs passed. [Acceptance record](../../engine/tests/admission/README.md#maintainer-integrated-acceptance--245246-2026-11-10) retains source/run pins and local/hosted distinctions. |
| Accepted exchange owner | Private persistent owner and preparation gates; input-snapshot admission and actual allocation limits for selected buffers, map outer stores, declaration-table backing and shared declaration/catalogue object/control-block requests, the five unique owner/projection/coordinator/binding objects and both retained projections' four table stores, are integrated through #246. Projection setup work and membership contribution-table/temporary graph-node allocation limits are also accepted. Contribution backing releases at last strong alias; the shared control block stays charged through final weak release. Remaining nested payload and constructor/setup resources stay open. See [resource inventory](../../engine/docs/OWNER-RESOURCES.md). |
| Next implementation | Active contribution: nested declaration parent-vector allocation limits on provisionally accepted #247. Then complete nested key/recipe/input payload, preceding catalogue/declaration construction and graph-temporary allocation/setup limits, then combined caller/borrowed resource accounting, then remaining operations and same-execution controls. See [continuation](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). |
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

**Immediate work:** nested declaration parent-vector allocation limits on provisionally accepted #247, followed by nested key/recipe/input payload, preceding catalogue/declaration construction and graph-temporary allocation/setup limits, then combined caller/borrowed resource accounting, then remaining-operation integration and same-execution controls under the [continuation plan](../core-local-study-continuation-plan.md).

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


