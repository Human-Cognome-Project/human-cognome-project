# Repeated membership target pins and catalogue ownership

Status: **inspection provisionally accepted in #387**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `01b09c773654c24df2ec1cac03f45ae24e1e1f07`, tree `91161780699e21630e2a960aa4d898e89c7170c3`, follows accepted #386 `1c10f14c60b355677357f10f85d394d50f4a3994` ([disposition](https://github.com/Human-Cognome-Project/human-cognome-project/pull/386#issuecomment-7018419165)). The maintainer integrated combined #383/#384 at main `6dcf3fef` after its native gate passed. At that handoff #385 targeted main and #386/the inspection were stacked; #385/#386 are now integrated at `0e3c0fdf`. No integration merge is created here. This contribution changes only tests and documentation. **Production behavior and achieved runtime saving are unchanged: zero saving.**

The [maintainer's table direction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/387#issuecomment-7020608389) accepts `a912e6a6`; its twelve native and six smoke jobs passed. Main `0e3c0fdf` integrates #385/#386. The [runtime follow-up](MEMBERSHIP-TARGET-TABLE.md) measures actual table/index/header overhead and preserves this record's original before evidence.

## Actual ownership and native boundary

After [#386's migration](MEMBERSHIP-HEADER-IDENTITY.md), `MembershipFields` owns one study header, a recipe `BundleRef` and private numeric occurrence rows. Each row still independently owns a full target `ToolRef`: scope UUID, variable-length address and revision UUID. Repeated occurrences and aggregate/byte alternatives copy those complete pins even when they reach the same target. The immutable projection recipe already owns the exact tools from which collection copied them.

This is not permission to remove target meaning or merge occurrences. Entry/full path, explicit membership kind, exact whole-mass count, per-occurrence diamond deduplication, full target revision and study/recipe association remain required. English/French contexts, shared core bytes and repeated paths retain their existing distinct rows. Equal target pins can supply the same immutable definition while participation and physical occurrence identity remain separate.

The existing catalogue owns only a recipe *reference value*, not a strong recipe payload pin. It must remain usable after projection/fixture/copy-source teardown. A raw pointer or index into the source recipe cannot replace that ownership. The private owner already retains its immutable recipe through projection/declaration pins, but ordinary public collection/copy does not inherit that lifetime. Retaining an entire recipe graph would be a separate lifetime and retained-cost tradeoff.

Native `RecordHandle` owns context/incarnation/address/record claim identity; prepared field headers and the kind/object binding map associate those claims with HCP fields. They do not supply the complete logical target UUID/address/revision. Taichi's native storage and claim authority remain unchanged. The duplication under inspection is immutable HCP pin backing, not a second native execution state. See [native instance evidence](NATIVE-INSTANCE-INSPECTION.md), [record handles](../src/engine/record_map.h), [prepared headers](../src/field/prepared_storage.h), and [exchange associations](../src/assembly/exchange_bindings.h).

## Returned factories and ordinary catalogue copying

The observer visits actual returned row buffers, attributes each scope/address/revision to an independently observed C++ allocation and confirms selected-domain ownership. Every row's three buffers differ from the recipe's and every other row's buffers. Checked study, recipe selection/revision and full-target lookup resolves the exact source tool without constructing a new owning pin or allocating.

Expected full keys, target pins, kind and counts are independently prepared from fixture source facts before collection. Public v1 live, bounded v2 live, bounded all-alternative and ordinary catalogue-copy operations retain their original measurement windows and allocation-failure sweeps. Source/graph/copy inputs then die; full catalogue exports and bound identities retain their existing lifetime controls.

| Actual operation | Rows / capacity | Distinct full targets | Actual pin requests / bytes | One representative owning pin per target: requests / bytes | Repeated beyond those representatives: requests / bytes | Inline pin capacity attribution |
|---|---:|---:|---:|---:|---:|---:|
| Public v1 live collection | 8 / 8 | 5 | 24 / 676 | 15 / 424 | 9 / 252 | 1,088 bytes |
| Bounded v2 live collection | 8 / 8 | 5 | 24 / 676 | 15 / 424 | 9 / 252 | 1,088 bytes |
| Bounded v2 all alternatives | 16 / 16 | 5 | 48 / 1,352 | 15 / 424 | 33 / 928 | 2,176 bytes |
| Ordinary catalogue value copy | 16 / 16 | 5 | 48 / 1,352 | 15 / 424 | 33 / 928 | 2,176 bytes |

The representative column is the sum of actual first-row pin requests for distinct full targets. It is **not an implemented target table**, its allocation count, or a measurement of the source recipe's capacity. The repeated column subtracts those observed representatives; it is a candidate backing reduction before replacement overhead, not net savings.

On this x64 ABI `ToolRef` occupies 136 bytes inside the unchanged 200-byte private row. The inline column attributes part of the already counted row-vector request (1,600 / 3,200 bytes), including unused capacity. It is not another allocation and cannot be added again to whole retained bytes. No new row width, table capacity, index or header cost is inferred. Public exported values remain 248 bytes with independently owned complete keys/pins.

## Actual adopted owner

The same nonallocating observer runs against each real adopted owner's catalogue and exact recipe while the constructor's allocation table is alive. It retains no recipe/catalogue loan after owner destruction. Full maps/native snapshots and complete domain/weak cleanup remain checked.

| Actual owner | Rows / capacity | Distinct full targets | Actual pin requests / bytes | Representative requests / bytes | Repeated beyond representative: requests / bytes | Inline pin capacity attribution |
|---|---:|---:|---:|---:|---:|---:|
| One selection | 3 / 4 | 1 | 9 / 267 | 3 / 89 | 6 / 178 | 544 bytes |
| Four mixed selections | 12 / 16 | 1 | 36 / 1,068 | 3 / 89 | 33 / 979 | 2,176 bytes |
| Three equal-sized selections | 6 / 8 | 1 | 18 / 534 | 3 / 89 | 15 / 445 | 1,088 bytes |

These pins are already included in actual owner totals of 240,856 / 288,444 / 255,191 retained requested bytes. Whole construction requests, cumulative bytes, retained/peak bytes, work, capture buffers, map census and detached-export costs remain identical to the accepted #386 measurements. The source-pinned comparison verifies that equality; it does not relabel attributable pin bytes as a whole-owner saving.

## Checked lookup and retained meaning

The test-only lookup checks complete study plus recipe selection/revision before comparing complete target pins. Foreign study, foreign recipe selection/revision and the same target object with a different target revision refuse. Zero/one-short allowance refuses; exact retry succeeds. The per-row lookup and repeated-target comparison allocate nothing. They charge 38,915 / 82,671 inspection-work units in live/catalogue fixtures and 13,085 / 52,865 / 26,345 in the owner fixtures. These are conservative observer scans, not current production setup work or an implemented lookup improvement.

Public indexed export still materializes a detached full key/target and costs 1,012 / 2,008 nested bytes for eight/sixteen rows, with output vector capacity separate. Ordinary catalogue copies independently own the header, paths and all pins; their actual requests and allocation-refusal coverage are preserved. Full exported values survive catalogue and selected-domain death. The observer does not introduce a retained vector, cache, shared pointer or replacement collector.

## Bounded follow-up proposal

Review one owning full pin per distinct target within the existing catalogue, with a checked numeric target index in each private row. Keep numeric occurrence entry/path, explicit kind and count per row. Deduplicate immutable targets by the **complete ToolRef**, scoped to that catalogue's full study/recipe context. Preserve deterministic row/group order and per-occurrence participation; do not deduplicate rows or conflate field kinds. Reuse existing native C++ vector/allocator/work primitives; introduce no global registry, cross-owner cache, native slot identity substitute or generic wrapper.

An owned catalogue-local table would preserve the current standalone/source-death boundary without retaining an entire source recipe. The private owner's already pinned exact recipe is an alternative source, but any retained index into it needs strong lifetime ownership and explicit detached public/copy semantics. Do not silently make ordinary catalogue copies share source-domain recipe payloads. Review the representation and its lifetime tradeoff before implementation; a new table or recipe-backed mode is not built here.

Coordinate collection, value copying, both bind paths, plan/capture/command reads, exports, census and independent backing visitors. Validate an index before dereference and validate complete study/recipe association at owning boundaries. Preserve full target revision, kind/count, short valid addresses, NULL naming masses, all alternatives, existing native claim/generation guards and publication semantics. Exports and bound/report values must still own complete identities after table/catalogue/domain death.

Measure actual before/after construction, retention and peak with replacement table/header/index extents, capacity growth and any recipe lifetime extension. Include empty and single-row controls, many occurrences sharing one target and multiple distinct targets. Measure ordinary copies and explicit exports separately. Sweep every row/table/pin allocation failure and selected quota boundary; preserve source/copy/move/assignment lifetime, weak release, partial unwind, zero/one-short/exact work retry, allocation-free steady preparation/execution, atomic publication and following ticks. This inspection authorizes no implementation by itself and does not change #324 mathematics, catalogue/mixed-revision policy, broader producer adoption or combined resource admission.

## Source-pinned local qualification

All four final profiles exit zero. Actual CPU/x64, Clang/LLVM 15 and GCC 11 wrappers with matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`; exact recursive submodules, warm/viewport enabled and GPU qualification disabled.

- Quick CI: three CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: membership value/native CTests and five hierarchy invocations (all four worker/path cost modes and complete mixed owner on 2-worker direct).
- GCC 11: both membership CTests and three hierarchy invocations (1-worker bulk / 2-worker direct cost modes and complete mixed owner).
- Standalone GCC 11 and GCC 13: all four projection/value CTests each. Each membership run passes 4,922 checks.

There are sixteen factory/copy and twenty adopted-owner target observations. All final profiles and verified manifests preserve exact source and clean state before/after. Production/fork/build/workflow diffs from accepted #386 are empty. The retained #386 archive supplies identical runtime before evidence; no accepted baseline is rerun. Comparison checks all sixteen factory/copy records, all twenty owner records and sixteen detached export records for unchanged actual metrics. Complete mixed-owner controls preserve refusal/retry, survivors, atomic publication, no-new preparation/execution and following ticks.

Development's overlapping second build was stopped with exit 130 while the first completed. Both logs and an event note are preserved and excluded from qualification counts; final native builds use their build-directory lock. All final qualification results are separate from development. Existing post-PASS cache-lock warnings remain in raw logs without cache deletion. No database, GPU, timing, full unrelated regression or complete accounting acceptance is claimed.

Verified machine-local archive: `/tmp/hcp-membership-target-01b09c77-evidence.tar.gz`, SHA-256 `2fa99abcee86dd7b14c9e9a4a5ea488fa9a1d980564163629ed817e8c436e216`. It retains raw source-pinned profiles, exact commands/configuration, toolchain/submodule/binary/runtime identities, production-unchanged and clean-source proofs, actual pin/copy/export/owner/work/refusal/lifetime logs, comparisons, the accepted runtime archive, development logs and manifests. A digest identifies local evidence; it does not make the raw archive remotely available. New authoritative GitHub CI and representation review remain pending. All merges belong to the maintainer.
