# Projected occurrence definition ownership

Status: **contributor inspection awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `3de88e610b5915e446d75114885b26bf4db1cee4`, tree `d0ae70024f99b74b9cd6bb4a17ac9e6c58f2fe49`, follows provisionally accepted #388 `fa83360e085118d91f3cec7bfe9e2c0056ce9382` ([review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/388#issuecomment-7022583392)). Main `f228581d` integrates hosted-qualified #387. This test/documentation contribution is stacked on #388; all merge events remain maintainer work. **Production behavior is unchanged and achieved runtime saving is zero.**

## Existing lifetime and native authority

`LetterProjection` already owns `shared_ptr<const warm::Bundle> recipe_`. `project_letters` copies the input bundle into that immutable payload. Ordinary value copies and explicitly selected-domain copies independently own the entire recipe; only the private owner-spare constructor shares the committed projection's exact recipe/control block. The projection therefore already has strong lifetime ownership for its complete definition pins. This differs from a membership catalogue's reference-only recipe header.

Every `ProjectedOccurrence` still independently owns `definition`, a full `ToolRef` with scope UUID, complete variable address and revision UUID. Each matches a tool in the retained immutable recipe. Committed and spare projections repeat those buffers even while they share that recipe. The observer attributes each actual buffer to the constructor's allocation table and selected domain, checks that it differs from its recipe source and every other row, and checks pairwise independence across committed/spare views. It retains only integer observations after owner teardown.

Occurrence study/entry/full numeric path, active state, v1 slot and source node remain separate row facts. Byte provenance independently owns its logical owner key, character `ToolRef` and both ordinals; complete-character equivalence has another optional full pin. This inspection attributes **only `definition`**, not those provenance pins or membership targets. Equal definitions do not merge occurrences, change mass or collapse participation.

Native `RecordHandle` provides context/incarnation/address/record claim identity. It does not encode the complete HCP definition revision. Native prepared fields, claim/recycle validation, execution state and atomic publication remain under their existing owners. Reusing immutable HCP meaning must preserve the source node/mass association and every native claim/generation gate. See [native instance inspection](NATIVE-INSTANCE-INSPECTION.md), [record map](../src/engine/record_map.h) and [projection interface](../src/assembly/letter_projection.h).

## Actual public factories and copies

The source fixture has five logical selections: two English occurrences with aggregate/parts exposure, a French occurrence, a multibyte character aggregate and repeated-character parts. Independently prepared fixture expectations check all full row keys, definitions, node/activity/slot facts, byte owner/character/ordinals and complete-character equivalence. All fourteen alternative rows remain; eight are active and ten full definitions are distinct.

| Actual operation | Rows / capacity | Definition pin requests / bytes | Inline definition capacity | Whole row-vector bytes | Construction requests / cumulative bytes | Retained / requested peak |
|---|---:|---:|---:|---:|---:|---:|
| Public v1 projection | 14 / 16 | 42 / 1,246 | 2,176 | 10,240 | 1,549 / 131,406 | 67,214 / 70,254 |
| Public v2 projection | 14 / 16 | 42 / 1,246 | 2,176 | 10,240 | 1,529 / 124,448 | 57,010 / 60,326 |
| Explicit selected-domain v2 copy | 14 / 14 | 42 / 1,246 | 1,904 | 8,960 | 604 / 53,798 | 53,798 / 53,798 |
| Ordinary v2 value copy | 14 / 14 | 42 / 1,246 | 1,904 | 8,960 | 604 / 53,798 | 53,798 / 53,798 |

Each actual scope/address/revision request is visited once. On the measured x64 ABI `ToolRef` is 136 bytes inside the unchanged 640-byte public row. Inline capacity is part of the already counted vector allocation, not an extra allocation or additive saving. All pin bytes are already included in construction/retention totals. The recipe already owns matching full pins; the table does not model a new registry, index width, replacement row or its overhead.

## Actual adopted owner

Both real owner views are inspected in the existing adoption window without C++ allocation. They share the exact immutable recipe but have separate row and pin buffers. These per-view results apply equally to committed and spare projections:

| Owner fixture | Rows / active / distinct definitions per view | Pin requests / bytes per view | Both views' pin bytes | Inline definition capacity per view | Whole row-vector bytes per view |
|---|---:|---:|---:|---:|---:|
| One selection | 3 / 1 / 3 | 9 / 267 | 534 | 408 | 1,920 |
| Four mixed selections | 12 / 6 / 3 | 36 / 1,068 | 2,136 | 1,632 | 7,680 |
| Three equal-sized selections | 6 / 3 / 2 | 18 / 534 | 1,068 | 816 | 3,840 |

Whole-owner retained/requested peaks stay 240,310 / 285,465 / 253,834 bytes. Construction stays 1,733 / 2,169 / 1,861 requests, cumulative 290,323 / 342,501 / 305,873 bytes, setup work 380,450 / 629,375 / 437,026 and temporary capture work 49,228 / 144,520 / 74,873. Five capture buffers still request 43,280 bytes; current/candidate map census stays 94,360 and the candidate pair remains 47,180 bytes / 562 requests. These unchanged totals include the attributed definition buffers and all existing recipe/membership/parent ownership; attribution is not net saving.

## Public row value and lookup controls

The current `occurrences()` API lends a const reference to the owning row vector. A caller can copy one `ProjectedOccurrence` and retain every key, revision and provenance fact after the source recipe, projection and selected domain die. The new fixture copies all fourteen values into caller-reserved output. Every mode requests 113 nested allocations / 3,326 bytes; the separately reserved output vector occupies 8,960 bytes. Every one of those 113 allocation sites is refused individually in each mode, checking unchanged source/live/domain accounting, prior output values and a healthy exact retry. Existing projection constructor/copy/quota/assignment unwind controls remain active; the new sweeps cover public row copying, not a new runtime constructor.

The test-only lookup requires complete study, recipe selection/revision, source entry/full path, source node and complete definition scope/address/revision. It resolves a positive whole-mass tool in the projection's own recipe. Foreign study, missing entry/path, changed node, stale revision, foreign scope/address, another valid tool and foreign recipe selection/revision refuse. Zero/one-short work refuse and exact work retry succeeds. Inspecting all rows and counting distinct full definitions allocates nothing, charging 50,332 observer-work units in each pure mode and 9,089 / 38,561 / 14,377 per owner view. This conservative scan is test evidence, not an implemented runtime algorithm or a throughput claim.

## Bounded proposal for review

Review checked recipe-local definition indices in private projection storage, reusing the projection's **already owned exact immutable recipe**. No new recipe registry, catalogue or lender is needed. Preserve every numeric occurrence/path, activity/slot/node fact and all byte/complete-character provenance. Do not substitute native record claims for full semantic pins or alter participation. Bind, exposure preparation, capture, plan, command, report/census and backing visitors must resolve the complete original pin through the owning projection and check bounds before dereference. Full study/recipe/revision checks remain at owner boundaries.

Settle the public output contract before implementation. The current reference accessor cannot silently become a temporary materialized vector. An explicit count/index export returning a complete owning `ProjectedOccurrence` is a reviewable option, analogous to membership values; private readers can use checked recipe access without exporting. Audit all current callers, preserve explicit detached output and ordinary value-copy/source-death behavior, and do not accidentally retain a source selected domain through a public copy. A numeric index must move/copy with its correlated recipe; public/selected copies still own an independent payload, while only the private owner spare shares the exact existing payload. No new borrowed pointer may escape that lifetime.

The next caller audit starts with [projection construction/copy/exposure/storage](../src/assembly/letter_projection.cpp), [membership collection](../src/assembly/membership_fields.cpp), [workbench binding](../src/assembly/active_workbench.cpp), [exchange contract](../src/assembly/exchange_contract.cpp), [exchange associations](../src/assembly/exchange_bindings.cpp), [structural plan](../src/assembly/structural_admission_plan.cpp), [prepared owner](../src/assembly/prepared_study_owner.cpp) and the [viewport producer](../../tools/viewport/native/prepared_letter_profile.cpp). Tests and independent backing visitors must move with those readers. This is a scoped audit list, not a claim that those APIs have migrated.

Measure actual row/index/header extent, construction/copy retention and peak, capacity growth, work, all new allocation/quota unwind sites and separate detached output costs. Include empty/single/many repeated/multiple distinct definitions, complete revision/scope/address differences and malformed indices. Preserve exact exposure switching, native snapshot/claim association, allocation-free frozen preparation/execution, atomic publication and following ticks. Report any small-case peak/work regression. Existing attribution is not a replacement-cost estimate. This inspection implements no index or public API migration and authorizes neither by itself. #324 mathematics, mixed-revision adoption policy, broader producer adoption and complete resource admission remain separate.

## Source-pinned qualification

All four final profiles exit zero at the exact source above with unchanged clean source before/after. CPU/x64; Clang/LLVM 15 and GCC 11 wrappers use matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled and CUDA/GPU qualification disabled. Both runtime bitcode hashes are `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

- Quick CI: three CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: both projection CTests, all four worker/path adoption-cost modes and complete mixed owner on 2-worker direct (five hierarchy invocations).
- GCC 11: both projection CTests, adoption-cost modes on 1-worker bulk / 2-worker direct and complete mixed owner on 2-worker direct (three invocations).
- Standalone GCC 11/13: all four projection/value CTests each. Every configuration passes 4,965 projection checks.

Final evidence contains sixteen pure definition observations, forty committed/spare owner observations and sixteen public row-copy observations with 1,808 individually refused allocation sites. All four value configurations agree exactly by mode; every owner observation agrees by selection count/view. Existing full projection copy/quota/assignment unwind, weak/domain lifetime, native refusal/retry/publication, survivors, frozen no-new execution and following-tick controls remain active within the selected scope.

Comparison verifies **186 unchanged records**: six existing projection-copy and forty-four membership factory/export/small-case records, plus 136 native allocation/work/map/capture/parent/membership records including twenty whole-owner totals. Accepted runtime source `6da22846` and final #388 `fa83360e` have identical production/tests/build/fork; the empty source diff and accepted raw archive are retained. No accepted broad baseline is rerun. This inspection's production/fork/build/workflow diff from #388 is empty.

The last-added address refusal control initially used an integer where the address vector requires an explicit `AddressElement`. Clang quick build and GCC standalone compilation caught that test type error at `259e7f0a`. Normal child source `3de88e61` corrects it; both failed profiles, source history and earlier development logs remain in evidence, excluded from final counts. Existing post-PASS cache-lock warnings are retained without cache deletion. No GPU, database, throughput, full unrelated regression or complete resource/admission qualification is claimed.

Verified machine-local archive: `/tmp/hcp-projection-definition-3de88e61-evidence.tar.gz`, SHA-256 `85de55d73fa729c3667b5d6e4150c6936d0fc4810fa665efea0f2dfbbce69c1b`. It retains final raw profiles, source/toolchain/configuration/submodule/binary/runtime identities, exact commands, clean-source and production-unchanged proofs, cost/refusal/lifetime/publication logs, all comparisons, the accepted runtime archive, development failures and verified manifests. Extracted archive contents and all nested manifests verify. A digest identifies local evidence; it does not make the raw bundle remotely available. Representation/public-output review and new authoritative GitHub CI remain pending. All merges remain maintainer work.
