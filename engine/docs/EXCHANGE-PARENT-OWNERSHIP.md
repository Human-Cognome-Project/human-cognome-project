# Exchange parents already retained by the owner recipe

Status: **contributor inspection and proposal awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Source `50ec736d3fbf8073eb9f25a3ab131e28bfded4bb`, tree `ea796f8d33888bbb081cd1a2317590e5fccbf83a`, starts independently from current main `952e1651c69801a0ddfb2dac219bc6ac36946b25`. Production sources, native substrate, fork, workflows and build registration are byte-identical to that parent. **Runtime saving is zero.** The separately provisionally accepted adoption-local validator in [#382](https://github.com/Human-Cognome-Project/human-cognome-project/pull/382#issuecomment-7013865241) is not a source dependency or included implementation.

## Concrete duplicate and authority

`LetterExchangeContract::collect` resolves each aggregate/byte source to its complete definition in the validated `LetterProjection::recipe().tools`, then `copy_parents` deep-copies that tool's ordered `warm::Parent` rows. Every `ExchangeSource::parents` owns a new vector plus target scope/address/revision backing. The source descriptor still needs its distinct study/entry/path, complete definition pin, exact mass and node. The two alternatives and repeated uses remain distinct; this inspection proposes no occurrence or semantic deduplication.

At private owner adoption, `ExchangeOwner` already owns a detached immutable recipe. Committed and spare projections share precisely that owner-local recipe, while declarations retain another copy of each selected source's parents. Binding capture (`exchange_bindings.cpp`) and command preparation (`exchange_coordinator.cpp`) read these copied parents. `owner_storage.cpp`, contract copy/relocation/matching and test census visitors account for them. These are the coordinated consumers of a possible later removal.

Native `field::detail::RelationshipHeader` retains two full record handles, a native ordinal and share; prepared source ledgers retain exact mass/count. They do not encode a definition's complete semantic ToolRef, stored-mass provenance or recipe revision. Taichi/RecordMap/Harness remain structural and execution authorities; **HCP must retain these meanings in its pinned recipe**. Native handles or live edge rows cannot replace the recipe parents. The opportunity is to reuse existing immutable HCP source ownership, without adding a parallel structural registry.

## Actual requests attributed

The fixed-storage allocation observer identifies the real producer/owner requests by pointer, checks exact requested extents and selected domains, rejects duplicate attribution within a declaration, and proves the copied buffers differ from recipe backing. Vector capacity and nested scope/address/revision backing are reported separately. No estimated subtraction, `sizeof`-only saving or replacement runtime path supplies these numbers.

| Actual adopted owner | Sources across both alternatives | Ordered parent rows | Parent vector bytes | Nested pin bytes | Combined parent requests / bytes |
|---|---:|---:|---:|---:|---:|
| One selection | 3 | 5 | 840 | 445 | 18 / 1,285 |
| Four mixed selections | 12 | 20 | 3,360 | 1,780 | 72 / 5,140 |
| Three equal-sized selections | 6 | 9 | 1,512 | 801 | 33 / 2,313 |

The public producer, bounded selected-domain collection and independent reverse-copy control also attribute complete parent backing:

| Producer fixture, each of three factory modes | Sources | Parents | Vector bytes | Nested pin bytes | Combined requests / bytes |
|---|---:|---:|---:|---:|---:|
| Repeated multibyte construction | 5 | 11 | 1,848 | 979 | 38 / 2,827 |
| Other multibyte construction | 6 | 13 | 2,184 | 1,157 | 45 / 3,341 |

All factory modes and tested compiler/worker paths agree. The public factories are measured as independent ownership controls, not proposed zero-copy runtime implementations. Outer source/declaration/vector headers, recipe ownership, an eventual pin or index, and any revised coallocation are outside the candidate byte totals. These are current attributable requests, **not achieved whole-owner savings or a prediction of net retained/peak reduction**. Actual runtime changes would require before/after measurement.

## Existing const access, work and lifetime evidence

The inspection-only helper resolves a source through existing projection/recipe const accessors. It checks study, recipe selection/revision, full occurrence, definition revision, exact mass and node before returning the recipe's existing tool. It then compares every parent target pin, ordinal, optional stored mass and contribution in order. No replacement array, retained wrapper, global lookup table or native state is constructed.

All successful lookup/comparison traversals allocate nothing. Linear checked resolution still has real logical work: 9,431 / 42,242 / 15,534 units for the one/four/three owner fixtures; 17,059 / 18,446 for the two public fixtures. These units measure the inspection's resolution alone, not a new production setup or operational allowance. Zero/one-short lookup work refuses and exact retry succeeds. A runtime design must preserve and price validation; this witness does not justify adding repeated linear scans to ordinary exchange or claim a setup-work improvement.

Six source mutations refuse foreign study, entry, path, definition revision, mass or node. Five controls retain the same lookup association while changing parent revision, order, optional stored mass, contribution or length; comparison must reject them. Thus a matching outer pin alone does not excuse inconsistent inner meaning. Existing descriptor permutation/bijection, stale/foreign/duplicate, both-direction, repeated-path and exact-budget/allocation-unwind controls remain in the value suite.

Native owner observation covers actual declarations and both alternatives. The separate recipe-lifetime witness destroys the caller projection before lookup, confirms the owner recipe is detached, stages the opposite exposure without changing immutable parent backing, then destroys the owner while retaining an explicit existing recipe shared pointer. Parent addresses remain owned by that pin. Releasing the strong pin retires recipe payload; the weak/control-block and selected-domain/bootstrap release checks follow. No borrowed pointer is used after its pin expires. Existing whole-owner tests preserve native state, generation, claims, captured facts, maps and publication behavior, including subsequent ticks.

Public declarations and explicit domain copies still outlive every source independently, with complete parent matching and stale-revision refusal. That detached ownership is an existing requirement, not evidence that a public borrowed reference would be safe.

## Bounded follow-up proposal

Review a private owner path that reads ordered parents from the **same owner-owned immutable recipe** used to validate each exact source. Prefer existing const recipe/tool access and an explicit lifetime association. Preserve independently owned public/value-taking declarations and copies; do not silently reinterpret their arrays or authorize borrowing caller storage. Do not share by nominal pin across owners or revisions.

Any storage change must coordinate declaration construction/copy/matching, binding capture, command preparation and census visitors. Retain full identity, exact integer mass, stored-mass provenance, repeated ordinals, both alternatives, current exposure/direction, generation/readiness, native claims and transaction/publication state. Remove only actual duplicate parent construction/retention charges, retaining all source/association validation. No generic range wrapper, registry, cache, owner or cross-generation sharing is proposed. Review chooses the explicit private representation before implementation.

Qualify actual before/after whole requests, retained bytes, peak and new ownership/index overhead; include complete source death, owner/pin/weak/domain/bootstrap lifetime and allocation-failure/partial-unwind sweeps. Preserve exact/one-short setup/work/bytes, healthy retry, standalone value semantics, mixed/equal alternatives, forward/back publication and following ticks across compiler/worker paths. Native authority, catalogue/mixed-revision policy, broader producer adoption, full accounting, GPU and #324 mathematics remain separate.

## Source-pinned validation

At the exact clean source, x64 CPU execution uses Clang/LLVM 15 and GCC 11 wrappers with matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, recursive submodule pins and GPU qualification disabled. No production path changes.

- Quick CI: three native CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: 4,283 structural recipe checks and seven native case invocations: four adoption probes, two recipe-lifetime modes and a complete mixed-owner 2-worker direct control.
- GCC 11 wrapper: 4,283 recipe checks and five native case invocations: two adoption probes, two recipe-lifetime modes and the complete mixed-owner direct control.
- Standalone GCC 11 and GCC 13: all four independent projection/value CTests each, including 4,283 recipe checks. Raw producer observations are retained.

There are 20 real owner-parent attribution observations and 14 recipe-parent lifetime observations across these native cases. All final profiles exit zero with unchanged clean source before/after. Existing post-PASS native cache-lock warnings are retained; no caches or retained data are reset. Later documentation changes no tested code. GitHub CI is authoritative; full local regression, the no-unroll matrix, database/GPU execution, physical residency, throughput and complete accounting are not established.

Verified machine-local archive: `/tmp/hcp-parent-inspection-50ec736d-evidence.tar.gz`, SHA-256 `0664f1fff36ea234356f4e00f42e2de7dc7927ab279740f58ac8015548b76db1`. It retains all four profiles, exact commands/configurations, source-clean and unchanged-production proofs, recursive submodule/toolchain/binary/runtime identities, raw value/native/refusal/lifetime logs, all 34 extracted observations and their cross-configuration verification, development logs and verified per-profile/bundle manifests. Development is unpinned and excluded from qualification counts. Publishing a digest does not make this raw archive remotely available.

Reproduction with the matched CPU build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
engine/build/structural_recipe_test
engine/build/live_hierarchy_test --threads 1 --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_recipe_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_owner_multiple
```

The retained script records all worker/path and standalone compiler commands, clean source checks and verified manifests. Existing registered owner cases and the standalone projection suite execute the added controls in hosted CI. All merges remain with the maintainer.
