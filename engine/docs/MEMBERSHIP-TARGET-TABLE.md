# Catalogue-owned unique membership targets

Status: **implementation provisionally accepted in #388**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `6da228466327cfaeb9da6c697360352e929e7d30`, tree `379c0d7e84be94b7f0f38b39335e6801e0f37eb4`, implements [#387's explicit table direction](https://github.com/Human-Cognome-Project/human-cognome-project/pull/387#issuecomment-7020608389) from accepted `a912e6a668141e11c48fb3e1d69eacccd8aab981`. At the original source handoff, main `0e3c0fdf` integrated #385/#386 and this contribution was stacked on qualified #387. Main `f228581d` now integrates #387. [#388 review 7022583392](https://github.com/Human-Cognome-Project/human-cognome-project/pull/388#issuecomment-7022583392) accepts final `fa83360e`, including the measured tradeoffs below. Smoke, native build/database and projection-values passed; remaining native jobs run. All merge events remain with the maintainer.

## One owned target per full pin

`MembershipFields` owns a reservation-backed vector of complete `ToolRef` values. Collection compares scope, complete address and revision before reusing an entry. New targets append in first-use order; a checked 32-bit local index replaces the full pin in each private row. Native count limits are checked before narrowing. Numeric occurrence entry/full path, explicit kind and exact whole-mass count remain per row. Existing group traversal, row order, per-occurrence diamond deduplication and aggregate/byte alternatives are preserved. Only immutable target storage is shared.

Both vectors and every nested UUID/address/path use the existing selected domain. Construction, full-pin comparisons, table and row growth, and nested relocation prepayment remain metered. A production target read checks bounds before dereference and charges its scalar validation where a preparation meter exists. Structural plan construction uses its own checked guard so malformed indices return `InvalidInput` and unwind through the existing result boundary. Bind, capture and command retain their full study/recipe, role, revision, count, claim and generation gates.

The table owns its pin buffers independently of the source recipe. It adds no recipe-payload lifetime, raw source loan, global registry, cross-owner cache or native slot authority. Native `RecordHandle` and field-map/publication responsibilities remain unchanged. The public count/index API still exports a fully owning `MembershipContribution`, including study/path and full target revision, after source/table/catalogue/domain death.

## Value and assignment lifetime

Ordinary copies deep-copy both vectors and all nested pin/path data into independent unselected storage. Copy assignment constructs the complete replacement in the destination domain before replacing either correlated table. Any allocation/quota refusal preserves the old header, row vector, target vector and facts. Healthy retry succeeds. Move construction/assignment transfers both vectors and the domain together; reconstruction releases displaced buffers with their allocators intact, following the existing projection/exchange-value pattern. Same/unequal-domain moves and self-assignment preserve exact values.

The census counts row/index capacity, numeric paths and the target table/pins once. Independent backing visitors and graph-address/UUID controls use the same actual ownership surfaces. Shared catalogue/control-block counts do not change. A malformed target index refuses export, legacy/structural bind, plan, capture and command preparation; healthy retries preserve native snapshots.

## Actual factories and copies, including overhead

Before measurements are reused from [#387's inspection](MEMBERSHIP-TARGET-PINS.md), source `01b09c773654c24df2ec1cac03f45ae24e1e1f07`. Its runtime, tests, build registration and fork match accepted base `a912e6a6`; the empty source diff is retained. No accepted broad baseline is rerun. All values below are actual C++ requests in the same construction/copy windows, with exports separate.

| Actual operation | Requests before → after | Cumulative requested bytes before → after | Retained bytes before → after | Requested peak before → after | Preparation work before → after |
|---|---:|---:|---:|---:|---:|
| Public v1 live, 8 rows / 5 targets | 201 → 196 | 13,991 → 13,739 | 2,427 → 2,175 | 12,471 → 12,763 | Public path unmetered |
| Bounded v2 live, 8 rows / 5 targets | 201 → 196 | 13,991 → 13,739 | 2,427 → 2,175 | 12,471 → 12,763 | 54,860 → 57,272 |
| Bounded v2 catalogue, 16 rows / 5 targets | 229 → 200 | 17,891 → 14,787 | 4,727 → 2,711 | 14,771 → 13,283 | 71,373 → 74,851 |
| Ordinary catalogue copy, 16 rows / 5 targets | 60 → 28 | 4,727 → 2,303 | 4,727 → 2,303 | 4,727 → 2,303 | Value copy unmetered |

Live retained storage falls 252 bytes, but its requested peak **rises 292 bytes** during overlapping growth, and bounded setup work rises 2,412 units. The all-alternative catalogue retains 2,016 fewer bytes, peaks 1,488 lower and charges 3,478 more work units. Ordinary copying saves 2,424 retained/peak bytes because its copied target vector has exact size/capacity five instead of the collected capacity eight. These are scoped allocation observations, not timing, allocator metadata or process RSS.

On the measured x64 ABI, rows shrink 200→64 bytes, including a four-byte target index. Row-vector requests become 512 / 1,024 bytes. The separate target vector requests 1,088 bytes (capacity eight) in collected fixtures or 680 bytes (capacity five) in an ordinary catalogue copy. Its fifteen nested requests retain 424 bytes, replacing 676 / 1,352 per-row pin bytes. The existing study/recipe headers still retain 111 bytes total. The catalogue object grows **192→232 bytes** for the extra vector header. Pure fixture objects live on the stack, so that 40-byte extent is reported separately from C++ request totals; the actual adopted-owner measurements include its coallocation cost. Index capacity attribution is already inside row-vector bytes and is never added twice.

## Empty, single and repeated controls

These newly required public/selected fixtures were absent from the accepted baseline. The identical public/count/value-only control source (SHA-256 `200c4a1ea78a25a3f2db6c27e052806ffe42a22ce929779162fbc814d443521b`) was compiled with GCC 11 and GCC 13 against a clean isolated `a912e6a6` checkout, then against this implementation in all four value configurations. This runs only the new controls against old code, not the accepted full suite. Both old compiler results agree, including every actual allocation refusal and exact work retry.

| Control (public / bounded selected agree on bytes) | Requests before → after | Cumulative bytes before → after | Retained before → after | Peak before → after | Bounded work before → after |
|---|---:|---:|---:|---:|---:|
| Empty membership set | 101 → 101 | 5,988 → 5,988 | 111 → 111 | 5,988 → 5,988 | 21,798 → 21,798 |
| One row / one target | 115 → 116 | 6,921 → 6,921 | 394 → 394 | 6,881 → 6,881 | 24,363 → 24,364 |
| Twelve rows / one repeated target | 163 → 131 | 14,362 → 9,369 | 4,307 → 1,354 | 12,673 → 8,881 | 59,973 → 57,584 |

The extra catalogue header still costs 40 object bytes in all three cases. An empty table allocates no target buffer. The single row adds one allocation and one work unit, with unchanged requested bytes/peak. Repeated targets save 2,953 retained bytes and 3,792 peak bytes. Every participation row remains separate, with full detached study/target identity and exact count after all source/catalogue/domain owners die.

## Actual adopted owner

| Owner fixture | Construction requests before → after | Cumulative bytes before → after | Retained / requested peak before → after | Shared setup work before → after | Temporary capture work before → after |
|---|---:|---:|---:|---:|---:|
| One selection | 1,738 → 1,733 | 291,277 → 290,323 | 240,856 → 240,310 | 380,946 → 380,450 | 49,225 → 49,228 |
| Four mixed selections | 2,201 → 2,169 | 347,520 → 342,501 | 288,444 → 285,465 | 631,716 → 629,375 | 144,448 → 144,520 |
| Three equal-sized selections | 1,875 → 1,861 | 308,182 → 305,873 | 255,191 → 253,834 | 438,145 → 437,026 | 74,855 → 74,873 |

Net retained/peak savings are 546 / 2,979 / 1,357 bytes; cumulative savings are 954 / 5,019 / 2,309. Setup work falls 496 / 2,341 / 1,119 units, while temporary capture adds 3 / 72 / 18 units for checked index access. Each fixture owns one target-table slot requesting 136 bytes and three nested buffers / 89 bytes. The former per-row pins requested 267 / 1,068 / 534 nested bytes. Other released setup requests fall 7,141→6,733 / 15,796→13,756 / 9,711→8,759 bytes; temporary whole-construction overlap peaks fall 80,302→79,756 / 106,427→103,448 / 89,250→87,893. These scoped overlaps are separate from whole retained peaks.

The five temporary capture buffers remain 43,280 bytes. Current/candidate map census remains 94,360 bytes and the candidate pair remains 47,180 bytes / 562 requests. Complete target pins, native claims, immutable recipes and survivor state remain exact. Full retained/peak results include the larger catalogue header, target capacity and narrower row/index storage; they are not sums of the inspection's attribution columns.

## Detached output and refusal evidence

Explicit public exports retain the same 248-byte value shape and unchanged nested costs: 37 requests / 1,012 bytes for eight values, 72 / 2,008 for sixteen. Caller output vector capacity is separately reported (1,984 / 3,968 bytes), reserved before export measurement. All 872 export allocation refusals across the four configurations preserve the source catalogue and previously exported values. Ordinary catalogue copying is the separate table-owning operation above.

Each final value run includes 696 allocation refusals across the six new empty/single/repeated factory controls, plus the original collector/copy/export and every selected allocator/preparation-work boundary. Copy assignment independently checks all 28 actual allocation sites and 56 byte/work quota refusals, original backing identity/facts, exact allowance and healthy retry. Complete scope/address/revision equality is exercised directly in private retention: different revision, scope or address gets a distinct entry; repeated complete pins reuse their index.

## Source-pinned qualification

All four final profiles exit zero with unchanged clean source before/after. CPU/x64, Clang/LLVM 15 and GCC 11 wrappers with matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled, CUDA and GPU qualification disabled.

- Standard quick CI: three CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: nine selected CTests (plan, four worker/path prepared-owner modes, workbench, structural recipe and both membership suites) and thirteen hierarchy invocations (complete one/four/three owners on 1-worker bulk and 2-worker direct; standalone binding/commit/field controls; all four adoption-cost modes).
- GCC 11: seven selected CTests, with prepared owners on 1-worker bulk / 2-worker direct, and eight hierarchy invocations (three complete owners, three standalone cases and two cost modes).
- Standalone GCC 11/13: all four projection/value CTests each. Every membership run passes 5,737 checks.

Comparison checks sixteen existing factory/copy before/after records, twenty-four new small-factory records and eighteen owner cost-probe records. All twenty-seven whole-owner observations agree by fixture; sixteen explicit export records remain unchanged. Full foreign-study/recipe controls, invalid-index refusal/healthy retry, zero/one-short/exact work, partial allocation/quota unwind, source/copy/move/assignment/domain/weak lifetime, native atomic publication and following ticks remain active. Successful frozen preparation/execution preserves its no-C++-new guarantees.

Development's initial new-fixture compile caught allocator-vector/address-element type mismatches. The invalid-index plan control initially exposed an escaping warm exception; the plan now validates its index through the existing `InvalidInput` refusal boundary before access. Failed and corrected development logs are retained and excluded from final qualification counts. Existing post-PASS offline-cache lock warnings are preserved without cache deletion. No database, GPU, timing, full unrelated regression or complete resource/admission qualification is claimed.

Verified machine-local archive: `/tmp/hcp-membership-table-6da22846-evidence.tar.gz`, SHA-256 `bd6ba15c13b88de332f0af19cf333a128d4d62fc30142b1354b8a0c3355f9b81`. It retains all final profiles, source/toolchain/configuration/submodule/binary/runtime identities, exact commands, clean-source and baseline-equivalence proofs, accepted inspection archive, the isolated old-code new-control driver/build/results, comparisons, actual allocation/work/refusal/lifetime/publication logs, development failures and verified manifests. A digest identifies local evidence; it does not make it remotely available. Implementation review is provisionally accepted in #388; remaining authoritative GitHub qualification and maintainer integration stay pending. All merges remain maintainer work.

#324 mathematics, catalogue/mixed-revision policy, broader producer adoption and complete resource admission remain separate work.
