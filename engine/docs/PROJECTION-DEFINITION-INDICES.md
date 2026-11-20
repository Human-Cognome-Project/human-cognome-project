# Recipe-backed projected definitions and owning public rows

Status: **contributor implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `223e874a3d5a58e346f40daf033f380e6baa86ea`, tree `62fc3f088721a74af564c529a8bb30148a04cabd`, starts from fully integrated main `646df1791287330dab8d5dc3ecc17a4dbca26727`. [#389 review 7023633130](https://github.com/Human-Cognome-Project/human-cognome-project/pull/389#issuecomment-7023633130) approves checked recipe-local definition indices and explicit count/index owning exports. #388/#389 passed both hosted workflows and are integrated. This is the implementation of that bounded direction; all merge events remain maintainer work.

## One immutable recipe, checked private indices

Each private projected row replaces its owning definition `ToolRef` with two checked 32-bit indices: the definition tool and its original recipe occurrence. Construction checks index-width bounds before publication, resolves v1/v2 source occurrence, node and complete definition pin against the projection's already owned immutable recipe, and retains every actor-specific key, activity/slot/node fact, byte-owner/character/ordinals and complete-character pin. No occurrence or participation row is merged. The second index supplies an independent source association: an in-range index for another tool refuses instead of silently changing definition meaning.

Contextual reads check tool/source bounds, study, complete entry/path, source node, complete scoped source object and positive whole mass. The immutable canonical tool supplies the full revision pinned at construction; the validated recipe admits one tool per object. Every copy keeps both indices correlated with that recipe. Ordinary and selected-domain copies still deep-copy the full recipe payload and detach from the source; only the private spare shares its owner's exact payload/control block. Existing copy assignment stages the entire correlated value, and moves transfer recipe/rows/domain together.

Before exposure preparation changes either view, both views' indices, exact keys and full recipe pins validate. Activity remains current owner-local state; immutable initial recipe exposure is not substituted for it. Parent-pool movement, native claim/generation checks, atomic publication and following ticks retain their existing authority. Native `RecordHandle` is not a semantic definition revision. No numerical kernel, #324 mathematics, cache/private boundary or catalogue/mixed-revision policy changes.

## Explicit owning public output

`occurrence_count()` reports the number of alternatives. `occurrence(index)` checks the row and returns a complete owning `ProjectedOccurrence`, detached from every recipe/projection/domain allocation. The former reference-returning `occurrences()` API is removed. No materialized temporary is hidden under it. Internal assembly, membership, exchange, plan and frozen execution callers use checked contextual reads without constructing owning rows; the viewport producer explicitly uses count/index exports during setup. Origins and other owning outputs retain complete keys and full pins.

Private rows are 512 bytes on the measured x64 ABI, down from 640; public values remain 640. The two indices occupy eight bytes inside each row. They add no vector/header/control block or per-row recipe lender. Byte and complete-character provenance still independently own their existing full pins. Census and independent backing visitors remove only per-row definition buffers, count private row/index capacity once and keep the immutable recipe counted under its existing ownership.

## Actual public factories and value copies

The accepted [inspection](PROJECTION-DEFINITION-PINS.md) supplies before evidence at `3de88e61`; its code/tests/build/fork match integrated main. Existing accepted baseline execution is not repeated. The same five-selection fixture retains fourteen rows/eight active alternatives and ten full definitions. Construction, copy, retention, peak and detached output use separate observation windows.

| Actual operation | Requests before → after | Cumulative requested bytes before → after | Retained before → after | Requested peak before → after |
|---|---:|---:|---:|---:|
| Public v1 projection | 1,549 → 1,507 | 131,406 → 126,304 | 67,214 → 63,920 | 70,254 → 67,072 |
| Public v2 projection | 1,529 → 1,487 | 124,448 → 119,346 | 57,010 → 53,716 | 60,326 → 57,144 |
| Selected-domain v2 copy | 604 → 562 | 53,798 → 50,760 | 53,798 → 50,760 | 53,798 → 50,760 |
| Ordinary v2 value copy | 604 → 562 | 53,798 → 50,760 | 53,798 → 50,760 | 53,798 → 50,760 |

All forty-two definition-pin requests / 1,246 nested bytes disappear. Public row-vector capacity sixteen now requests 8,192 bytes rather than 10,240; copied capacity fourteen requests 7,168 rather than 8,960. Index capacity (128 / 112 bytes) is already inside those requests. Net retention falls 3,294 bytes in each public factory and 3,038 in each value copy. Public factory peak falls 3,182 bytes: the temporary borrowed occurrence-source map grows eight bytes per row from its added source index/alignment, so peak differs from retained savings. Actual cumulative reductions are 5,102 / 3,038 bytes. No allocator metadata, process RSS or throughput claim follows.

## Both actual owner views

The actual adoption observer checks both committed and spare vectors against their shared exact recipe, without allocation or a retained loan. The table attributes their row-vector and definition-buffer requests; unchanged key/provenance/parent/input surfaces remain independently visited.

| Fixture | Rows per view | Row-vector plus definition buffers before → after, per view | Net reduction per view | Both-view / whole-owner retained and peak reduction |
|---|---:|---:|---:|---:|
| One selection | 3 | 2,187 → 1,536 | 651 | 1,302 |
| Four mixed selections | 12 | 8,748 → 6,144 | 2,604 | 5,208 |
| Three equal-sized selections | 6 | 4,374 → 3,072 | 1,302 | 2,604 |

Both views' net reduction agrees with the separately observed whole-owner retained/peak difference. The recipe/control-block count is unchanged, and no new recipe lifetime is added. Per-view index extents 24 / 96 / 48 bytes are included in row-vector requests, not added again.

| Whole owner | Requests before → after | Cumulative bytes before → after | Retained / requested peak before → after | Setup work before → after | Temporary capture work before → after |
|---|---:|---:|---:|---:|---:|
| One | 1,733 → 1,715 | 290,323 → 289,021 | 240,310 → 239,008 | 380,450 → 349,577 | 49,228 → 45,731 |
| Four mixed | 2,169 → 2,097 | 342,501 → 337,293 | 285,465 → 280,257 | 629,375 → 464,059 | 144,520 → 88,848 |
| Three equal | 1,861 → 1,825 | 305,873 → 303,269 | 253,834 → 251,230 | 437,026 → 382,040 | 74,873 → 56,048 |

Capture still uses five buffers / 43,280 bytes. Current/candidate map census stays 94,360 bytes and the candidate pair 47,180 / 562 requests. Other released setup requests stay 6,733 / 13,756 / 8,759 bytes. Full recipe/semantic pins, node/mass association and native snapshots remain exact. Indexed exposure/contract reads remove repeated tool scans, but bounded membership collection adds guard work: live collection rises 57,272→58,372 units and the all-alternative catalogue 74,851→77,035. Membership allocation/retention/export costs are unchanged. These are measured work charges, not execution timings.

## Small controls and detached exports

New public-only controls are compiled against the isolated integrated before source with GCC 11/13, using only an adapter from the new count/export spelling to the old vector count/value-copy API. The identical fixture source runs against the implementation in all four final value configurations. Both old compilers agree; no accepted full baseline suite is rerun. Empty recipes preserve their existing refusal with zero retained storage. A single selection has two alternatives; twelve repeated selections retain twenty-four distinct rows while sharing two exact tool indices.

| Control | Retained before → after, v1 / v2 | Requested peak before → after, v1 / v2 | Request reduction |
|---|---:|---:|---:|
| Single, public | 30,218→29,784 / 28,826→28,392 | 31,562→31,144 / 30,476→29,930 | 6 |
| Single, selected copy | 30,234→29,800 / 28,842→28,408 | Same as retained | 6 |
| Twelve repeated, public | 86,672→80,440 / 75,088→68,856 | 90,304→84,264 / 79,584→72,944 | 72 |
| Twelve repeated, selected copy | 76,208→71,000 / 65,840→60,632 | Same as retained | 72 |

Explicit public exports retain their complete independent 640-byte shape. Fourteen values still request 113 nested allocations / 3,326 bytes; caller-reserved output capacity is separately 8,960 bytes. Every one of the 113 sites refuses and retries in each factory/copy mode, preserving source values and prior output, for 1,808 refusal controls across four configurations. Single and repeated outputs separately request 16 / 475 and 192 / 5,700 nested allocations/bytes, plus 1,280 / 15,360 caller vector bytes. Complete key, definition revision, byte owner/character/ordinals and complete-character equivalence survive all source/projection/domain teardown.

## Source-pinned qualification

All four final profiles exit zero at the source above with exact unchanged clean source before/after. CPU/x64; Clang/LLVM 15 and GCC 11 wrappers use matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules, warm/viewport enabled and CUDA/GPU qualification disabled. Both runtime bitcode hashes are `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

- Quick CI: three CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang: sixteen CTests covering structural plan, four prepared-owner worker/path modes, workbench/provenance, both projection and membership suites, structural recipe, viewport snapshot, prepared letter on 1-worker bulk / 2-worker direct and prepared retained profile on 2-worker direct. Thirteen hierarchy invocations cover all three full owner fixtures on 1-worker bulk / 2-worker direct, standalone binding/commit/field controls and all four cost-probe modes.
- GCC 11: thirteen CTests with two prepared-owner modes and one prepared-letter mode; eight hierarchy invocations cover three full owners, three standalone controls and two cost modes.
- Standalone GCC 11/13: all four projection/value CTests each. Every value configuration passes 5,132 projection checks.

Comparison independently checks sixteen primary factory/copy records, thirty-two new successful small cases, eight empty refusals, twenty-seven whole owners against fifty-four separate view observations, sixteen unchanged detached row-export records and fifty-six membership allocation/output records, with work changes separated. The new small fixture include SHA-256 is `7c005a93e29c4777b9964afdb2c43faae1e4647b5454dfa89bbf7b6469aabb4e`; the adapted old-API standalone driver is `fe4d8a86f448e781d1f0f9bad70855a5c6de06e52fe3406e18536b1c701941c1`. Additional complete-owner before records reuse accepted `6da22846`, whose runtime matches the integrated before source; the original archive and equivalence proof are retained.

Malformed definition/source indices include an in-range wrong tool. Public/contextual reads, both owner views and structured plan preparation refuse without publishing partial values/native state; healthy retries succeed. Full foreign study/path/node and revision controls, zero/one-short/exact work, every existing selected copy allocation/quota boundary, staged assignment, move/source/domain/weak lifetime and frozen no-new execution remain active. Full owner controls preserve survivors, atomic publication and following ticks. The new explicit row export sweeps all 113 actual allocation sites per fixture mode; it does not claim a new public-factory allocation sweep.

Development logs retain failed plan/test reader migrations, corrections to empty/focus-less small fixtures, an omitted isolated-baseline test header and the comparison tool's GCC diagnostic correction. These uncommitted attempts are excluded from final source-pinned qualification counts. Existing post-PASS cache-lock warnings are preserved without cache deletion. No GPU, database, compiler-IR matrix, process RSS, throughput, full unrelated regression or complete resource/admission qualification is claimed.

Verified machine-local archive: `/tmp/hcp-projection-indices-223e874a-evidence.tar.gz`, SHA-256 `8569cefee910aaf09097790deef3cc2c1aaa467ea0ed8efecf0c0090f1db82a0`. It retains final raw profiles, exact commands, source/toolchain/configuration/submodule/binary/runtime identities, clean-source and unchanged-boundary proofs, all allocation/work/refusal/lifetime/publication and comparison records, the accepted before archive, isolated old-code controls/source/build metadata, development failures and verified manifests. Extracted archive contents and all nested manifests verify. A digest identifies local evidence; it does not publish the raw bundle. Implementation review and new authoritative GitHub CI remain pending; all merges remain maintainer work.
