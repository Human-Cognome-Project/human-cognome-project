# Catalogue-owned membership study identity

Status: **runtime provisionally accepted in #386**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `e7733a8ce3dd2321eeec2f76a4a7003d30bceed4`, tree `e0547e4ed495288a569a0858e82b600a75085d4b`, follows the [#385 direction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/385#issuecomment-7016915894) from accepted `fc0e7f65feb10aad06f5a33c46b9d400db4ec3c5`. That source includes the maintainer's combined #384 recipe-parent implementation and reconciled observer. At the source handoff main was `c961fe86`. Combined #384 now passed native CI and landed at `6dcf3fef`; #386's remaining authoritative checks and integration stay separate. All merge events belong to the maintainer.

The [maintainer accepted continuation](https://github.com/Human-Cognome-Project/human-cognome-project/pull/386#issuecomment-7018419165) from `1c10f14c`. The next [target-pin inspection](MEMBERSHIP-TARGET-PINS.md) measures the remaining per-row complete pins without changing this runtime.

## Retained rows and detached public values

`MembershipFields` retains its existing owning study UUID once. Private rows hold `warm::OccurrenceId` (entry and complete path), explicit membership kind, full target `ToolRef` including revision, and exact whole-mass count. Collection still validates each complete source study before emission and deduplicates reached targets per occurrence. Numeric equality never merges distinct studies, repeated entries, constituent paths, language contexts or exposure alternatives.

Public `MembershipContribution` keeps its full owning `OccurrenceKey` and unchanged value shape. `contribution_count()` reports the number of rows; `contribution(index)` explicitly materializes one detached value. It rejects an invalid index before indexing. Named intermediate owners preserve allocation unwind on GCC 11. The public value's study, path, target scope/address and revision use independent unselected storage, so it survives catalogue and selected-domain death.

The old reference accessor `contributions()` is removed. Repository callers deliberately use count/index extraction, or an explicit test-owned snapshot when they need a range. No temporary vector is hidden behind the old accessor and no duplicate export vector is retained. Private production bind, plan, capture, command and census readers use catalogue-context rows without exporting a full key for every scan.

Both bind paths compare the complete catalogue study with the projection and render full opaque names with `occurrence_text(study, id)`. Structural plan preparation checks the full catalogue study on association. Exchange capture retains its complete study/recipe validation; command preparation additionally validates the catalogue against the bound contract's owning study before matching numeric rows. Existing source, owner, generation, kind, recipe/revision, native claim, count and publication gates remain. The census counts one header study and only numeric paths in rows; independent backing visitors use the same ownership boundary.

## Actual factories and ordinary catalogue copying

The before evidence is reused from [#385's inspection](MEMBERSHIP-STUDY-IDENTITY.md), tested source `47c979269acc259d2ec5f351700e2a940b4ffaf9`. Its runtime, tests, build registration and fork are byte-identical to the accepted implementation base `fc0e7f65`; the retained empty diff proves that equivalence. No accepted baseline was rerun.

All figures below are actual C++ allocation requests. Retained and peak refer to the collector/copy observation window; they are not RSS or allocator metadata. Export costs are outside these windows.

| Actual operation | Rows / capacity | Requests before → after | Cumulative bytes before → after | Retained bytes before → after | Peak bytes before → after | Preparation work before → after |
|---|---:|---:|---:|---:|---:|---:|
| Public v1 live collection | 8 / 8 | 209 → 201 | 15,007 → 13,991 | 3,107 → 2,427 | 13,151 → 12,471 | Unmetered public path |
| Bounded v2 live collection | 8 / 8 | 209 → 201 | 15,007 → 13,991 | 3,107 → 2,427 | 13,151 → 12,471 | 55,736 → 54,860 |
| Bounded v2 all alternatives | 16 / 16 | 245 → 229 | 19,971 → 17,891 | 6,087 → 4,727 | 16,131 → 14,771 | 73,209 → 71,373 |
| Ordinary catalogue value copy | 16 / 16 | 76 → 60 | 6,087 → 4,727 | 6,087 → 4,727 | 6,087 → 4,727 | Unmetered value copy |

On the measured x64 ABI the private row is 200 bytes, down from 248; the public value remains 248. Actual vector requests fall 1,984→1,600 / 3,968→3,200 bytes. The eliminated study strings are 296 / 592 bytes, while the existing owned header still requests 37 bytes. Net retained/peak reductions are 680 / 1,360 bytes after actual layout. Complete target-object backing remains 380 / 760 bytes. Cumulative savings also include narrower intermediate vector requests; they are not retained savings. Removed work is only eliminated study construction/relocation and narrower row relocation; actual association and traversal stay charged.

## Explicit detached export cost

| Exported values | Actual nested requests | Requested / retained nested bytes | Separately reported output vector capacity | Injected allocation refusals |
|---|---:|---:|---:|---:|
| Eight live rows (each live mode) | 37 | 1,012 / 1,012 | 1,984 bytes | 37 |
| Sixteen catalogue/copy rows (each mode) | 72 | 2,008 / 2,008 | 3,968 bytes | 72 |

The caller's output vector is reserved before the export measurement. Its capacity is reported separately and excluded from nested request totals. Each explicit value export owns a complete study key and target pin. It costs allocations where borrowing an old reference did not; consumers needing detached values must account for that output cost. Internal owner scans never invoke this exporter. A normal catalogue copy instead owns the narrower rows plus one independent study header, as measured in the copy row above.

Every observed allocation site in every per-index export refuses without changing retained catalogue/domain backing or already exported values. Healthy retry matches independent source keys, full pins, kind and counts. Public values remain exact after projection, graph, copy source, catalogue and selected domain die; no allocator keeps that domain alive. Catalogue copy/collector refusal sweeps still cover every actual construction site.

## Actual adopted owner

| Fixture | Whole construction requests before → after | Cumulative bytes before → after | Retained / requested peak bytes before → after | Shared setup work before → after | Temporary capture work before → after |
|---|---:|---:|---:|---:|---:|
| One selection | 1,741 → 1,738 | 291,724 → 291,277 | 241,159 → 240,856 | 381,522 → 380,946 | 49,441 → 49,225 |
| Four mixed selections | 2,213 → 2,201 | 349,452 → 347,520 | 289,656 → 288,444 | 638,592 → 631,716 | 149,632 → 144,448 |
| Three equal-sized selections | 1,881 → 1,875 | 309,124 → 308,182 | 255,797 → 255,191 | 440,245 → 438,145 | 76,151 → 74,855 |

Net retained/peak reductions are 303 / 1,212 / 606 bytes, including both removed string requests and actual row-vector layout. Cumulative reductions are 447 / 1,932 / 942 bytes. Other released setup requests fall 7,285→7,141 / 16,516→15,796 / 10,047→9,711 bytes. Temporary whole-construction overlap peaks fall 80,605→80,302 / 107,639→106,427 / 89,856→89,250; these are scoped overlap observations, not whole retained peaks. The existing 37-byte header remains; all 3 / 12 / 6 repeated row-study requests are gone.

The five capture buffers still request 43,280 bytes total. Current/candidate map census remains 94,360 bytes and the retained candidate pair remains 47,180 bytes / 562 requests. Capture and command association pay the complete header check at their boundary and numeric comparisons per row; eliminated repeated study comparisons are no longer charged. Native state, maps, source captures, target pins, counts and immutable recipes are unchanged by adoption. Full owner retained/requested peaks include actual object layout; they are not obtained by summing candidate string and inline attribution. Full resource/admission accounting, allocator metadata and process peaks remain outside this scoped qualification.

## Source-pinned local qualification

All four profiles exit zero with unchanged clean source before/after. x64 CPU, Clang/LLVM 15 and GCC 11 wrappers, matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled, CUDA and GPU qualification disabled. Before/after native runtime bitcode has the same SHA-256, `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

- Quick CI: three native CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: nine selected CTests, including all four worker/path prepared-owner modes, workbench, structural recipe/plan and membership value/native physics. Thirteen hierarchy invocations cover complete one/four/three owners on 1-worker bulk and 2-worker direct, standalone binding/commit/field cases on 2-worker direct and all four adoption-cost modes.
- GCC 11: seven selected CTests, with prepared owners on 1-worker bulk / 2-worker direct. Eight hierarchy invocations cover all three complete owners, standalone binding/commit/field cases on 2-worker direct and adoption-cost probes on 1-worker bulk / 2-worker direct.
- Standalone GCC 11 and GCC 13: all four projection/value CTests per compiler. Each membership run passes 4,886 checks, including actual live/catalogue/copy construction and every corresponding allocation-refusal point.

The comparison script checks sixteen actual factory/copy before/after observations and eighteen actual native-owner before/after observations. All twenty-seven owner observations agree by fixture across final modes, compilers and full/probe routes. Sixteen detached export observations agree, including all 872 export allocation refusals across the four value configurations. Full keys, short target addresses, revisions, kind/count, contextual diamond deduplication, repeated/multibyte paths and all exposure alternatives remain exact.

Negative controls change only the catalogue header while preserving numeric entry/path: legacy/structural bind, structural plan, capture and command association reject the foreign study. Healthy retries succeed. Public value mutation cannot change the catalogue. Bound inputs preserve owning opaque names after local catalogue/projection destruction. Existing native reports, owner/source/domain/weak lifetime, complete allocation/quota unwind, zero/one-short/exact work retry, stale claims, refusal/postclaim unreadiness, atomic map/projection/report publication and following ticks remain active in the selected cases. Retained census and independent backing visitors agree; successful preparation/execution still performs no C++ `new` where required.

Development's first compile failed because an independent census visitor still used the old full-key path spelling. It was migrated to the numeric path; the failed and corrected logs are retained and excluded from qualification counts. Existing post-PASS offline-cache lock warnings are preserved without deleting caches. No database, GPU, timing, full unrelated regression or complete accounting/admission acceptance is claimed.

Verified machine-local archive: `/tmp/hcp-membership-header-e7733a8c-evidence.tar.gz`, SHA-256 `8935accf419ce762e81bf1f4a5f5d25f3d92404c9ed23dddda6d20bd17d50729`. It retains all four final profiles, exact commands/configurations, toolchain/binary/runtime/source/submodule identities, clean-source proofs, baseline-equivalence and unchanged-boundary diffs, actual export/copy/owner/work/refusal/lifetime/publication logs, comparisons, original before archive and verification, development failures and verified manifests. A published digest identifies local evidence; it does not make the raw archive remotely available. New authoritative GitHub CI and runtime review remain pending; all merges stay with the maintainer.

After review, reconcile hosted findings and integrate the accepted stack through the maintainer. Further #343 work should return to its remaining inspected ownership/native-reuse candidates; this migration does not change #324 mathematics, catalogue revision policy, native maps/claims or combined resource admission.
