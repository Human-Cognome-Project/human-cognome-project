# Private exchange parents from the pinned owner recipe

Maintainer update (2026-11-19): [comment 7015927137](https://github.com/Human-Cognome-Project/human-cognome-project/pull/384#issuecomment-7015927137) provisionally accepts this implementation and continuation from `4d4695ac`, integrating #383's independent parent observation with this runtime. #382 is now main `c961fe86`; combined #384 smoke passed and authoritative native CI remains pending. The earlier source/evidence and integration notes below describe the original handoff.

Status: **contributor runtime implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). The maintainer [accepted #383 and specified this representation](https://github.com/Human-Cognome-Project/human-cognome-project/pull/383#issuecomment-7014581181): one strong exact recipe pin per independently retainable private declaration and a validated recipe-local index per source, with detached public/value paths and actual net measurement. This implementation is stacked on provisionally accepted #382 at `e7915da7e1ba38f5e396532b31be688fb1ab7aed`; maintainer integration is still separate. Tested source `45ff81f37a59a09270ccc686b5668ec9c67a2efe`, tree `6a0593a3dc3183352bc5a50b13673554af57dffd`. The [inspection record](https://github.com/Human-Cognome-Project/human-cognome-project/blob/f5409f40de9e58b96ec2b457bd2624b6df67d3ef/engine/docs/EXCHANGE-PARENT-OWNERSHIP.md) retains its source-specific zero-saving baseline.

## Ownership and read boundary

Only `ExchangeOwner` passes its existing detached `shared_ptr<const warm::Bundle>` to private declaration collection. Construction requires pointer identity with the projection's actual immutable recipe; equal nominal selection/revision pins do not authorize another Bundle. It validates the complete occurrence study/entry/path and definition ToolRef against the projection and recipe during metered collection, then stores that recipe's tool index in a private source member. No native record, caller-owned pointer, global registry or cross-owner/revision deduplication substitutes for these associations.

Each retained private declaration owns one strong reference to that exact recipe. Each source keeps its full occurrence, definition pin, exact integer mass and node but reserves no separate parent backing. Its private index is meaningful only inside that declaration's pinned revision. The existing contract resolves ordered parents directly from the recipe, checking bounds and the complete definition pin; binding capture and command preparation charge these checks to their existing meters. Both explicit operation directions still read the matching outgoing/incoming source side. No parent order, optional stored-mass provenance, repeated contribution, membership, mask, native claim or publication behavior is replaced.

An independently retained private declaration now keeps the complete immutable recipe alive after owner teardown. This is an explicit lifetime extension; the owner-live byte savings below do not establish an after-owner-death memory reduction. A detached public/value copy retains its own selected parent facts and releases that private recipe association.

Public construction and value/explicit-domain copies keep independent parent values. Copying a private declaration materializes those values into the destination allocator and drops private recipe sharing. Copy assignment preserves the destination domain; moves transfer ownership. Private declaration matching resolves each side through its own declaration context. Raw caller-array matching requires owned parent descriptors and refuses a detached private index; a coincident index or matching nominal pin is insufficient evidence. This prevents interpreting an extracted private source against an unrelated recipe. The existing value-taking standalone bindings retain owned declarations and maps.

The census validates the owner recipe/index association, counts the source index and embedded shared pointer through actual object/vector sizes, and counts recipe parent backing once with the shared projection recipe. It does not count every declaration alias as another recipe/control block. Independent backing visitors continue to count only actual owned buffers. Taichi/RecordMap/Harness remain execution/structural authorities; semantic definition pins, provenance and composition remain HCP meanings. Runtime substrate, numerical fields, fork, workflows, database and build registration are unchanged.

## Actual net requests, retained bytes, peak and work

The before observations reuse the retained actual #382 CPU measurements at `3d9424e478f3768b180ab70b6462f55f480c2f85`, whose runtime, tests, CMake and fork are byte-identical to base `e7915da7`. Exact source-equivalence proof and original configurations, binary/runtime hashes and verified raw logs accompany the comparison. An accepted baseline was not rerun to onboard. After measurements execute the real modified owner constructor. Clang/GCC × 1-worker bulk/2-worker direct × three fixtures give **12 matching before/after comparisons**.

| Fixture | Construction requests before → after | Cumulative requested bytes before → after | Retained / whole requested peak before → after | Whole setup work before → after |
|---|---:|---:|---:|---:|
| One selection | 1,759 → 1,741 | 292,961 → 291,724 | 242,404 → 241,159 | 381,949 → 381,522 |
| Four mixed selections | 2,285 → 2,213 | 354,400 → 349,452 | 294,636 → 289,656 | 639,948 → 638,592 |
| Three equal-sized selections | 1,914 → 1,881 | 311,341 → 309,124 | 258,014 → 255,797 | 439,801 → 440,245 |

Net retained/requested-peak reductions are **1,245 / 4,980 / 2,217 bytes**; actual cumulative reductions are **1,237 / 4,948 / 2,217 bytes**, with 18 / 72 / 33 fewer requests. The inspection's gross parent backing was 1,285 / 5,140 / 2,313 bytes. On the measured ABI, a source index adds eight bytes per source-capacity slot and the strong pointer adds sixteen bytes per declaration/coallocation: net retained overhead is 40 / 160 / 96 bytes. Source-vector relocation adds eight / 32 / zero extra released setup bytes, accounting for the cumulative/retained delta. These are measured whole-owner results, not just candidate subtraction.

Setup work falls by 427 / 1,356 units in one/four selections and **rises by 444 units in the equal-sized three-selection control**. Complete indexed definition validation adds work even though parent allocation/copying disappears; no uniform work improvement is claimed. Fresh capture work correspondingly rises by 176 / 1,056 / 528 units. Existing complete source/handle/mass/participation validation remains charged; no unlimited private allowance or validation shortcut is introduced.

Temporary capture still makes five requests / 43,280 bytes and releases before spare/coordinator preparation. Maximum whole-owner requested bytes while it is live falls 81,850→80,605 / 112,619→107,639 / 92,073→89,856. Selected-domain peaks fall by the same retained reductions; current/candidate map census remains 94,360 bytes and independently located candidate backing remains 47,180 bytes / 562 requests. These overlaps are separate measurements and must not be added as independent savings.

The representation also increases detached public value extents: `sizeof(LetterExchangeContract)` is 296→312 bytes. The repeated fixture retains 5,040→5,080 requested bytes; the multibyte fixture retains 6,840→6,912 for public/bounded collection and 5,976→6,024 for its exact reverse copy. Public request counts and parent backing are unchanged. Cumulative collection requested bytes rise by 64 / 128 due to source-vector growth; these public value costs are not private savings. Metered public collection work is 21,129→21,615 / 28,733→29,340, including larger rows and complete occurrence checks. The earlier inspection's pure-producer implementation is byte-identical to the base contract code, and its original logs are retained for this comparison.

All byte figures observe calling-thread C++ allocator requests, not malloc/driver metadata, complete stack frames, RSS, physical residency or throughput. Requested peak reductions apply to these fixtures; full machine/accounting and performance qualification remain separate.

## Lifetime, refusal and publication qualification

The native private-parent control verifies exact pin identity and **one strong reference per declaration**, every source index and parent-vector address, zero duplicate parent capacity, complete comparison against a detached public declaration, and survival after caller projection death. Public copy, copy assignment and explicit-domain copy materialize independent parents. A private value moves its pin and value assignment drops it. Raw descriptor matching cannot use an unpinned private index; changed parent facts refuse.

Out-of-range and wrong in-range indices, changed definition revision and a different Bundle with identical outer pins refuse. Zero/one-short private construction work leaves no partial selected backing or extra strong reference; exact retry preserves all facts. Exposure staging leaves the recipe unchanged. An independently retained private declaration survives owner teardown with exactly one strong recipe reference, then releases recipe payload; weak declaration/recipe control storage releases the selected domain/bootstrap correctly. Detached copies remain valid after all those sources die. The control observes complete C++ request release and unchanged native snapshots.

All three full owners retain underlying allocation-failure/partial-construction sweeps, byte/allocation-work/setup-work boundaries, exact/one-short retry, source/claim/role and strong/weak/domain/bootstrap checks. They also retain mixed/equal alternatives, forward/back exchange, survivor/retirement, ordinary refusal/retry, postclaim unreadiness, atomic map/projection/report publication and following ticks. Standalone binding/commit/field lifecycle and pure public value refusal controls remain. No failed execution is repaired or resumed.

Initial source `f47d3923` passed quick/standalone/GCC qualification but Clang's first full owner failed the new strict `private value construction/assignment leaked selected backing` control. Memberwise equal-domain string move assignment could strand reservation accounting, the same issue already handled by `LetterProjection`. The separate fix destroys displaced declaration buffers with their allocators intact, then invokes its noexcept move constructor. Copy assignment stages a detached destination-domain value before this replacement. The failing profile and all original logs are preserved; no quota or cleanup assertion was relaxed. Final qualification below reruns the corrected source.

## Source-pinned local validation

At the exact clean source: x64 CPU, Clang/LLVM 15 and GCC 11 wrapper, matched fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled and GPU qualification disabled.

- Quick CI passes three native CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang passes **13 native case invocations**: four worker/path cost probes, all three full owners on 1-worker bulk/2-worker direct, and the three standalone binding/commit/field cases. Its structural recipe suite passes 3,987 checks.
- GCC 11 passes **eight native case invocations**: two cost probes, all three full owners and the three standalone cases on 2-worker direct; its recipe suite passes 3,987 checks.
- Standalone GCC 11 and GCC 13 each pass all four independent projection/value CTests, including 3,987 recipe checks.

The after cases contain 27 real production construction/capture observations and nine complete private recipe-parent lifetime controls, including 54 explicit new index/pin/work refusals and 27 detached copies. The existing 54 pre-stage adoption inventory controls, full capture/reference facts and allocation/work/unwind/publication tests remain. All four final profiles exit zero with unchanged clean source before/after. Later documentation changes no tested code. Development value/build logs are unpinned and excluded from these counts. Post-PASS cache-lock warnings are retained without cleanup.

Verified machine-local archive: `/tmp/hcp-owner-parents-45ff81f3-evidence.tar.gz`, SHA-256 `1788a9e2b7df1513989fd0bee05cfc09157e0a8e04e7a5f2ac64225ed0f77d54`. It retains all four final profiles, the initial failed Clang and successful candidate profiles, reused #382 baseline observations and independent public-producer baselines, exact source-equivalence/native-boundary proofs, commands/configurations, recursive submodules, toolchain/binary/runtime hashes, raw cost/work/refusal/lifetime/publication logs, all twelve checked comparisons, nine extracted private-parent controls, development logs and verified profile/bundle manifests. Publishing the digest does not make the raw evidence remotely available.

Reproduction on a matched native CPU build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
engine/build/structural_recipe_test
engine/build/live_hierarchy_test --threads 1 --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_owner_multiple
```

Existing registered owner and standalone cases execute these controls in hosted CI. GitHub CI is authoritative; full local regression, no-unroll, retained database, GPU/performance and complete accounting are not claimed. Source-pinned local qualification permits only this reviewed continuation; all merge and integration events remain with the maintainer.

For maintainer integration with independent #383, preserve its historical/public producer controls and adapt its private-parent observer to the indexed declaration context (`ExchangeOwnerAccess::contract_parents`). Its source-specific assumption that every private source owns copied parent backing is intentionally superseded here; restoring copies to satisfy that old assertion would undo this change. This branch does not include or duplicate #383's pending test commit. The combined integrated tree needs its own authoritative CI.
