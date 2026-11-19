# Membership row studies and the existing catalogue header

Status: **inspection provisionally accepted in #385**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `47c979269acc259d2ec5f351700e2a940b4ffaf9`, tree `d1ddc9e1b6fe058762b1686e1e3659ed0c2f113f`, starts from the maintainer's explicitly accepted combined #384 head `4d4695acbfb212271e8ceb2cd16317a6458ee0a2` ([disposition](https://github.com/Human-Cognome-Project/human-cognome-project/pull/384#issuecomment-7015927137)). That head integrates #383's parent inspection with private indexed parents; main `c961fe86` integrates #382. This slice changes only executable tests. Production, native substrate, fork, workflows and build registration are unchanged. **Runtime saving from this slice is zero.**

The [maintainer's disposition](https://github.com/Human-Cognome-Project/human-cognome-project/pull/385#issuecomment-7016915894) at `fc0e7f65` authorizes private numeric rows and explicit detached public values. The [runtime follow-up](MEMBERSHIP-HEADER-IDENTITY.md) measures the actual migration and its export/copy costs. This inspection record preserves the unchanged runtime and original before measurements at `47c97926`.

## One repeated identity and its owner

`MembershipFields` already owns `study_`, exposed by `study()`, plus its recipe pin and contribution vector. `MembershipFields::collect` validates each projected occurrence's study against this header before emitting any group row, then allocates another study UUID in every `MembershipContribution::occurrence`. A row is emitted per exact occurrence and reached target, so one logical study string is repeated across groups as well as repeated/aggregate/byte alternatives.

The duplicate is solely the study component. Numeric entry/path, explicit membership kind, full target ToolRef and revision, exact whole-mass count, contextual ownership and per-occurrence diamond deduplication are required meanings. Equal study identity does not merge rows or substitute another language's groups for a shared byte definition. The existing header and rows share the catalogue's value/domain lifetime; the header is not a loan from the projection. Ordinary catalogue copies own their headers independently.

Native RecordHandles/context/incarnation identify execution claims, not this logical study UUID. Taichi's structure does not supply the semantic UUID. The candidate reuses existing HCP ownership, as in the separate admission-plan [study-header precedent](PLAN-STUDY-IDENTITY-COST.md), rather than adding a native field, registry, shared pointer, identity cache or owner.

## Actual returned values, including copying and source death

The observer brackets actual public v1 live collection, bounded v2 live collection, bounded v2 all-alternative collection and an ordinary copy of that catalogue. The copy source is prepared before measurement and destroyed along with the original projection/fixture before detached value checks. Expected full occurrence keys, group pins, order and counts are prepared independently from fixture source facts.

| Actual factory | Rows / capacity | Separately requested row-study strings | Existing header-study string | Current vector capacity / UUID-member attribution |
|---|---:|---:|---:|---:|
| Public v1 live | 8 / 8 | 8 requests / 296 bytes | 37 bytes | 1,984 / 384 bytes |
| Bounded v2 live | 8 / 8 | 8 requests / 296 bytes | 37 bytes | 1,984 / 384 bytes |
| Bounded v2 all alternatives | 16 / 16 | 16 requests / 592 bytes | 37 bytes | 3,968 / 768 bytes |
| Ordinary catalogue value copy | 16 / 16 | 16 requests / 592 bytes | 37 bytes | 3,968 / 768 bytes |

Every header/row string pointer resolves to an actual independently observed C++ request with its exact extent and selected allocator. Row pointers differ from the header and from every other row. On the measured ABI a `MembershipContribution` is 248 bytes and its embedded UUID object is 48 bytes. The UUID-member capacity column attributes part of the already counted outer vector request; it is **not another heap allocation**. A later representation's width/padding, public output costs and whole retained/peak changes need actual before/after measurement. No net saving is implemented or inferred by adding these columns.

All original three producer modes retain their existing construction, retained, peak and work reports. The fourth mode measures the real implicit value copy and sweeps every one of its actual allocation failures; it is not a synthetic reconstruction or an alternate operational collector. Collector graphs, target-object pin authority and source-pinned historical costs remain separate.

## Actual adopted owner

The new nonallocating observer visits the real owner's retained all-alternative catalogue after successful adoption, locates header and row UUID allocations inside that constructor's allocation table, and independently finds every complete row occurrence in the owner's projection. Existing parent/index observers, capture facts, work/refusal/retry and map/native-state checks remain active.

| Actual owner | Membership rows / capacity | Row-study requests / bytes | Existing header bytes | UUID-member capacity attribution |
|---|---:|---:|---:|---:|
| One selection | 3 / 4 | 3 / 111 | 37 | 192 |
| Four mixed selections | 12 / 16 | 12 / 444 | 37 | 768 |
| Three equal-sized selections | 6 / 8 | 6 / 222 | 37 | 384 |

These are current attributable allocations, not achieved owner savings. The full owner still releases every request/domain/weak/bootstrap allocation, preserves all current/candidate maps and leaves native snapshots unchanged. The inspection retains no catalogue loan after owner destruction. The complete mixed-owner case additionally retains ordinary exchange, native survivor/retirement, stale/source/claim refusal, exact retry, postclaim unreadiness, atomic map/projection/report publication and following ticks.

## Checked header use, existing rendering and public boundaries

The inspection compares the existing header study plus each numeric `OccurrenceId` against independent full projected keys without constructing another owning `OccurrenceKey`. This comparison allocates nothing. It still charges its operations: 672 units for the eight-row fixtures and 1,312 for the sixteen-row fixtures; the owner fixtures charge 254 / 1,016 / 492 units. These are only the measured header/numeric comparisons, not full collector traversal work or a production setup improvement. Zero/one-short comparison work refuses; exact retry succeeds. Another study with identical entry/path, a different repeated entry, or a different constituent path refuses.

Existing `occurrence_text(study, id)` renders the same complete opaque name as a full row key. Text rendering still owns output strings; it is not claimed allocation-free. The detached result remains exact after all projection/graph/copy-source inputs die. English/French contexts, shared byte definitions, repeated/multibyte paths, shorter group addresses, NULL naming masses, kind/count/revision, deduplication, missing/cyclic/stale pins and all original work/allocation-unwind controls remain tested.

Current production readers are live `bind`/`bind_structure`, structural admission plan construction, private exchange binding and command preparation, retained census and independent backing visitors. Bind already compares the catalogue header against the requested projection; capture also checks catalogue study and recipe pins before matching rows. Plan/command paths must preserve their complete source/owner/generation/recipe gates when coordinated. Header-based comparison never authorizes an unrelated projection or native claim.

The current public row type owns a complete `OccurrenceKey`, and an extracted row copy therefore carries study identity independently. This inspection changes neither that shape nor that contract. A follow-up must explicitly settle the public row migration: catalogue values/copies and bound input/report outputs must retain complete detached identity, and any independently detached row export must materialize the existing full key at that boundary. Numeric rows without their owning header must not silently become complete identities.

## Bounded follow-up proposal

Review reuse of the existing owned catalogue study header with numeric entry/path in retained rows. Prefer existing `warm::OccurrenceId`, `study()` and lossless rendering over a new wrapper/registry. Keep the full header alive in the selected domain and validate it against the active projection/assembly at each owning consumer boundary. Preserve full input study validation before collection, explicit kind/full target pin/count, all alternatives and contextual per-occurrence deduplication. Reject inconsistent legacy duplicates wherever a retained transition still accepts them; add no fallback solely to preserve duplicate storage.

Coordinate collection, public/value outputs, both bind paths, plan generation, binding capture, command preparation and census/visitors. Remove only eliminated UUID construction/relocation/backing charges; retain and price actual traversal/association/header checks. Native maps/claims, publication, generations, Catalogue/mixed-revision policy, broader producer adoption and #324 mathematics remain separate. After representation review, measure actual public/bounded/copy/owner before/after requests, retained bytes, peak and work including row layout/export overhead, with source death, weak/domain release, every allocation/partial unwind boundary, zero/one-short/exact retry and native following ticks.

## Source-pinned local qualification

At the exact clean source: x64 CPU, Clang/LLVM 15 and GCC 11 wrapper with matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled, GPU qualification disabled.

- Quick CI: three native CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: membership value/native CTests plus five native hierarchy case invocations (four cost worker/path modes and complete mixed owner on 2-worker direct).
- GCC 11: the same two membership CTests plus three hierarchy cases (1-worker bulk/2-worker direct costs and complete mixed owner).
- Standalone GCC 11 and GCC 13: all four projection/value CTests each. Each membership value run passes 4,527 checks, including four actual factory/copy observations and every corresponding allocation-refusal point.

There are twenty actual owner UUID observations and sixteen factory/copy observations across these configurations. All four final profiles exit zero with unchanged clean source before/after. The existing source-pinned full owner native controls remain bounded to the cases above; no accepted complete baseline was rerun merely to onboard. Development's initial four zero-allocation failures counted assertion-message logging inside the observer window. Logging now occurs after that window; strict request attribution and the zero-allocation assertion remain. Failed and corrected development logs are preserved and excluded from qualification counts. Existing post-PASS cache-lock warnings are retained without cleanup.

Verified machine-local archive: `/tmp/hcp-membership-study-47c97926-evidence.tar.gz`, SHA-256 `525eff295318e5bf6902cf1014ce47689b056faa1a7381c94fb600edd5cc8656`. It retains all four profiles, exact commands/configurations, source-clean and unchanged-production proofs, recursive submodule/toolchain/binary/runtime identities, raw value/native/cost/work/refusal/lifetime logs, the twenty owner and sixteen factory/copy records with checked cross-configuration agreement, original/corrected development logs and verified profile/bundle manifests. Publishing a digest does not make this raw archive remotely available.

Reproduction on a matched CPU build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build --output-on-failure -R '^membership_fields(_native)?_test$'
engine/build/live_hierarchy_test --threads 1 --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_owner_multiple
```

Existing registered value/native/owner suites execute these controls in hosted CI. GitHub CI is authoritative. Full local regression, no-unroll, database/GPU execution, physical residency, throughput and complete accounting are not established. #384's combined hosted gate and this slice's new review/CI remain separate; all merges stay with the maintainer.
