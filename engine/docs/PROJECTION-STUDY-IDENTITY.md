# Projected occurrence study identity and header ownership

Status: **inspection provisionally accepted in #391**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `6a208a146d0eb8146d510bdf6b37aaa2e0ca8113`, tree `071f256f7bdfb94dc84d96e04f5c42039f0e8a66`, follows provisionally accepted #390 `db6a4bda8c0d1b482913ddca6fbd7837682c363a` ([review 7044020673](https://github.com/Human-Cognome-Project/human-cognome-project/pull/390#issuecomment-7044020673)). Main remains `646df179`; the original inspection is stacked on the explicitly reviewed head. #390 has since passed its full hosted qualification and remains unmerged. Only tests/documentation change at this inspection source. **Production behavior and achieved runtime saving are unchanged: zero saving.** [#391 review 7045231362](https://github.com/Human-Cognome-Project/human-cognome-project/pull/391#issuecomment-7045231362) accepts final `26dec2a0` and authorizes the [checked header/numeric-row runtime](PROJECTION-HEADER-IDENTITY.md). Original inspection measurements below retain their original source and zero runtime saving. All merges remain maintainer work.

## Existing header and complete keys

After [#390's definition migration](PROJECTION-DEFINITION-INDICES.md), each projection owns one study header and one exact immutable recipe. Private rows retain complete `OccurrenceKey` values plus checked definition/source indices, node/slot/activity and full byte/complete-character provenance. Every row's `key.study` duplicates the already owned projection header. Ordinary and selected-domain projection copies own independent headers, rows and recipes; the private owner spare shares the exact immutable recipe but independently owns its study/header and mutable view.

Study is the logical namespace for entry/full numeric path. Removing that association would conflate identical numeric occurrences in separate studies. Native `RecordHandle` supplies context/incarnation/address/record claim identity; it does not replace logical study identity before native admission or in detached public values after native teardown. Source recipe, definition revision, node/mass and native claim/generation checks remain separate authorities. The [projection resolver and internal readers](../src/assembly/projection_access.h) preserve those checks.

This observer attributes **only row-key study storage**, alongside separately labeled byte-owner study buffers. Byte-owner keys, character/complete-character full pins, parent contribution keys, memberships and native maps retain their existing ownership. No private key layout, reader API or UUID sharing is implemented here.

## Actual public factories and copies

The observer visits the actual constructed header and every row/byte-owner UUID, matches each request to the constructor's allocation table, checks the selected domain and proves distinct backing from the header and every other visited UUID. Existing recipe/source indices still resolve through the production guard. Only integer observations outlive the projection.

| Actual operation | Rows / capacity | Row study requests / bytes | Existing header request | Inline row-study capacity attribution | Whole private row-vector request | Separately retained byte-owner studies |
|---|---:|---:|---:|---:|---:|---:|
| Public v1 projection | 14 / 16 | 14 / 518 | 37 bytes | 768 bytes | 8,192 bytes | 9 requests / 333 bytes |
| Public v2 projection | 14 / 16 | 14 / 518 | 37 bytes | 768 bytes | 8,192 bytes | 9 / 333 |
| Selected-domain v2 copy | 14 / 14 | 14 / 518 | 37 bytes | 672 bytes | 7,168 bytes | 9 / 333 |
| Ordinary v2 value copy | 14 / 14 | 14 / 518 | 37 bytes | 672 bytes | 7,168 bytes | 9 / 333 |

On the measured x64 ABI `Uuid` occupies 48 bytes inside the unchanged 512-byte private row. Inline capacity includes unused vector slots and is part of the already counted row-vector request. It is not an extra allocation or an additive whole-memory saving. The existing 37-byte header is counted separately once per view; its presence means no additional lifetime pin is needed merely to supply the same study value. These observations do not select a replacement row width, reader/token overhead or net after cost.

Actual construction/copy requests, cumulative bytes, retained/peak bytes, selected quotas and explicit export costs remain identical to the reviewed #390 measurements. Public v1/v2 retention is 63,920 / 53,716 bytes; each selected/ordinary v2 copy retains 50,760. The observer changes none of those totals.

## Both actual adopted owner views

Committed and spare headers have equal study values with independent backing; every row/byte-owner UUID is also independent across views. The same no-allocation observer runs in each real adoption window, while its allocation table and domain are alive.

| Owner fixture | Rows per view | Row study requests / bytes per view | Existing header per view | Inline row-study capacity per view | Whole row vector per view | Byte-owner studies per view |
|---|---:|---:|---:|---:|---:|---:|
| One selection | 3 | 3 / 111 | 37 bytes | 144 bytes | 1,536 bytes | 2 / 74 bytes |
| Four mixed selections | 12 | 12 / 444 | 37 | 576 | 6,144 | 8 / 296 |
| Three equal-sized selections | 6 | 6 / 222 | 37 | 288 | 3,072 | 3 / 111 |

Both mixed-owner views therefore retain 24 row-study requests / 888 bytes, alongside their two existing header requests / 74 bytes. The 1,152 inline row-study bytes across their capacities are already inside the 12,288 row-vector bytes. Byte-owner study buffers separately retain 592 bytes across both views; they are not part of a claimed row-key reduction.

Whole-owner retained/requested peaks stay 239,008 / 280,257 / 251,230 bytes, with 1,715 / 2,097 / 1,825 construction requests and 289,021 / 337,293 / 303,269 cumulative bytes. Setup work stays 349,577 / 464,059 / 382,040 and temporary capture work 45,731 / 88,848 / 56,048. Capture buffers remain five requests / 43,280 bytes, map census 94,360 and candidate pair 47,180 bytes / 562 requests. Native snapshots, publication, following ticks and complete weak/domain release remain checked. Attribution is not an implemented whole-owner saving.

## Header/numeric equivalence and detached output

The test-only key matcher compares the complete study header before entry/full numeric path. Its result agrees with the existing owning full key. This is an equality witness, not a replacement for recipe/source/node/definition or native-claim validation. The observer separately calls the actual production source resolver; it introduces no lookup registry, retained cache or admission protocol.

Both v1 and v2 witnesses use real normalized recipes with independent studies, a uint64 maximum entry, numeric entry 2 and twelve repeated characters reaching path ordinal 10. Equal numeric keys in another study refuse; changed entry/path refuse. Zero/one-short work refuse, and exact retry succeeds with the first C++ allocation set to fail, recording zero allocation attempts. Existing split `occurrence_text(header,numeric)` formatting preserves the exact opaque full-key text and the same requested allocation cost. Numeric ordering remains unchanged.

The observation charges 1,837 key/header comparison units in the fourteen-row fixtures and 400 / 1,600 / 711 per owner view, separately from source-validation charges 2,636 and 566 / 2,264 / 1,116. These are conservative test witness costs, not changed production work or a throughput result.

Public `occurrence(index)` still exports a complete independent 640-byte value. Fourteen exports retain their existing 113 requests / 3,326 nested bytes, with caller output capacity 8,960 bytes reserved separately. All 1,808 allocation-refusal/retry controls remain active across the four final value configurations. Key study, full definition revision, byte-owner/character/ordinals and complete-character identity survive source/projection/domain death. The observation does not replace any owning output with a header loan.

## Bounded proposal for review

Review numeric occurrence entry/path under each private projection's existing owned study header, retaining definition/source indices, activity/slot/node and all byte/complete-character provenance. Keep public `ProjectedOccurrence` shape and explicit owning exports intact. Public copying still owns an independent recipe/header; only the private owner spare shares its exact existing immutable recipe. Preserve correlated header/rows/recipe across copy, move, exposure change and publication, with no new registry or lifetime lender.

Settle contextual reader ownership explicitly before removing a row's study stamp. Prefer reads coupled to the owning projection and checked row index; a free numeric row borrowed from another projection must not silently acquire a different header. At full-key boundaries, compare the complete study before numeric path and retain recipe/revision/source/node/mass, role and native claim/generation checks. Preserve full opaque rendering without constructing a fresh owning key on frozen paths. Public snapshots/reports/exports must still materialize complete identities after every owner dies.

Audit [projection construction/copy/exposure/export](../src/assembly/letter_projection.cpp), [contextual access](../src/assembly/projection_access.h), [exchange association](../src/assembly/exchange_bindings.cpp), [contract preparation](../src/assembly/exchange_contract.cpp), [membership collection](../src/assembly/membership_fields.cpp), [structural plan](../src/assembly/structural_admission_plan.cpp), [origins](../src/assembly/active_workbench.cpp), [census](../src/assembly/projection_storage.cpp) and independent backing visitors together. Byte-owner and parent-contribution full keys remain separate surfaces with their current provenance/output contracts; this first proposal targets the row-key study field.

Measure actual row/header/index/reader overhead, construction/retained/peak/work effects in public factories and both owner views, and separate detached output costs. Preserve source/domain/weak and same/unequal-domain assignment/move lifetime, all allocation/quota/work refusals and exact retry, foreign study with identical numeric keys, malformed index association, frozen no-new preparation/execution, atomic publication and following ticks. Include empty refusal, single/repeated and mixed/provenance fixtures, max entry and multi-digit paths. Do not infer net savings from this attribution. No representation change is authorized by the inspection alone; #324, mixed-revision adoption and broader resource/admission policy remain separate.

## Source-pinned qualification

All four final profiles exit zero with exact unchanged clean source before/after. CPU/x64; Clang/LLVM 15 and GCC 11 native wrappers use matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled and CUDA/GPU qualification disabled. Binary/runtime, toolchain and full configuration identities are retained.

- Quick CI: three CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: both projection CTests, all four worker/path cost modes and complete mixed owner on 2-worker direct (five hierarchy invocations).
- GCC 11: both projection CTests, cost modes on 1-worker bulk / 2-worker direct and complete mixed owner on 2-worker direct (three invocations).
- Standalone GCC 11/13: all four projection/value CTests each. Every value configuration passes 5,380 projection checks.

Evidence contains sixteen factory/copy study observations, forty actual owner views and eight full-key witness records. All configurations agree by fixture/mode; native observations agree by selection/view. Existing full identity/revision/provenance, malformed-index, quota/work/source/domain/weak lifetime, 1,808 public export allocation failures/retries, frozen no-new preparation/publication and following-tick controls remain active.

Comparison verifies **304 unchanged existing records**: 128 value and 176 native measurements, including twenty whole-owner totals. Reviewed runtime source `223e874a` and accepted final `db6a4bda` have identical code/tests/build/fork. The empty equivalence diff and original before archive are retained; accepted baseline execution is not repeated. This inspection's production/fork/build/workflow diff from #390 is empty.

The development build, pure tests and native probe passed and are retained separately, excluded from final qualification counts. Prior development failures remain in the accepted archive. Existing post-PASS cache-lock warnings are retained without cache deletion. No GPU, database, compiler-IR matrix, throughput, process RSS, full unrelated regression or complete resource/admission qualification is claimed.

Verified machine-local archive: `/tmp/hcp-projection-study-6a208a14-evidence.tar.gz`, SHA-256 `79797fab6de1f0668d491b106accd7a3515f093a55c9aba4803998afac8b5b09`. It retains final raw profiles, source/toolchain/configuration/submodule/binary/runtime identities, exact commands, clean-source/unchanged-production and before-equivalence proofs, allocation/work/key/refusal/lifetime/publication logs, comparisons, development logs, the accepted before archive and verified manifests. Extracted archive contents and all nested manifests verify. A digest identifies machine-local evidence; it does not publish the raw bundle. Representation/reader-ownership review and new authoritative GitHub CI remain pending. All merges remain maintainer work.
