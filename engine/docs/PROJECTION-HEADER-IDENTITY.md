# Projection-owned study identity and contextual row readers

Status: **contributor implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `68062bdea3ea348390a8f910c977e6d6e8e662b0`, tree `cda2652fba5b6400bd320b20b06a5d6b8cd6d583`, follows [#391 review 7045231362](https://github.com/Human-Cognome-Project/human-cognome-project/pull/391#issuecomment-7045231362) at `26dec2a0`. Main remains `646df179`; accepted #390/#391 are unmerged dependencies. Local CPU evidence qualifies only the recorded scope. Authoritative GitHub CI and maintainer integration remain separate.

#390's native/smoke/offline-viewport workflows passed. #391's [native 44774748429](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44774748429) and smoke passed. Their qualification does not qualify this new runtime; its own authoritative CI remains required.

## Owned context and independent source validation

Each private row now stores numeric occurrence entry/full path under its projection's existing owned study header. The unchanged definition/source indices, node/slot/activity and full byte-owner/character/ordinal/complete-character provenance remain in the row. The actual row width is 464 bytes, formerly 512. This removes the 48-byte inline UUID member and its independent 37-byte string request from each row. Existing header requests remain 37 bytes per projection. Byte-owner and parent contribution keys retain their full study namespaces and independent buffers.

`LetterProjection::resolve_definition(index)` selects a checked row in that projection. `ProjectionAccess::row`, `resolve`, `tool`, `definition`, `matches` and `find` accept the owning projection and checked index or a complete external key; none accepts a detached numeric row as contextual input. The const table view exposes numeric/provenance payload only. There is no retained reader object, new registry, shared UUID allocation or additional lifetime pin. Temporary ordered/owner lookups use indices with the same x64 extent as their former pointers.

The resolver independently compares retained entry/full path to the indexed v1/v2 recipe source, validates source/table bounds, node and source object against the canonical complete definition, and requires positive exact mass. The immutable owned recipe still supplies the full revision, validated at projection construction. Full external-key association compares the study header before entry/path. Binding capture checks that header once before its numeric scan, then resolves the selected source and preserves full recipe/revision, activity, exact mass and native context/incarnation/address/claim/generation guards. Structural plan refusal remains its structured `InvalidInput` result.

Exposure preparation validates both study/recipe headers and every row in both committed/spare views before changing exposure. Complete canonical pins and independent numeric identities must agree. Parent lookup still compares full study plus source; it never fabricates a temporary owning key on frozen paths. Native admission/publication/ticks remain the sole execution authority, with the same physical/model behavior.

Ordinary/public and selected-domain copies retain independent headers, mutable tables and complete recipes. Only the private owner spare shares its existing exact immutable recipe. Staged copy assignment preserves the destination domain; moves transfer the correlated value. Public `occurrence_count()` / `occurrence(index)` still export complete independent 640-byte owning values, including full key, full revision and all provenance, valid after source/projection/domain death. Origins likewise materialize full identities. Split opaque rendering uses the owning header and numeric source without a temporary key allocation.

## Actual factory and copy costs

All four operations retain fourteen rows. Each selected/ordinary value copy owns its own complete recipe and study header.

| Actual operation | Requests before → after | Cumulative bytes before → after | Retained bytes before → after | Peak bytes before → after |
|---|---:|---:|---:|---:|
| Public v1 projection | 1,507 → 1,493 | 126,304 → 124,298 | 63,920 → 62,634 | 67,072 → 65,786 |
| Public v2 projection | 1,487 → 1,473 | 119,346 → 117,340 | 53,716 → 52,430 | 57,144 → 55,858 |
| Selected-domain v2 copy | 562 → 548 | 50,760 → 49,570 | 50,760 → 49,570 | 50,760 → 49,570 |
| Ordinary v2 value copy | 562 → 548 | 50,760 → 49,570 | 50,760 → 49,570 | 50,760 → 49,570 |

Private row-vector requests are 7,424 bytes at capacity 16 and 6,496 at capacity 14. Existing definition/source indices occupy 128 / 112 bytes within those buffers. The row width accounts for the removed UUID without claiming its inline capacity twice. Public construction's cumulative reduction also includes narrower vector relocation; it differs from retained/peak reduction. Byte-owner UUID buffers remain nine requests / 333 bytes in the fourteen-row fixtures. Full contextual definition census work falls from 33,714 to 26,414; the fourteen source guards fall from 2,636 to 1,614. These are explicit metering units, not timing/performance claims.

Fourteen detached public exports remain 113 requests / 3,326 nested bytes, with 8,960 caller-reserved output bytes and 640-byte public rows. All 1,808 allocation-refusal/exact-retry witnesses remain active over the four final configurations. Export costs are separate from private retained savings.

## Both real owner views and whole adoption

| Selections | Requests before → after | Cumulative bytes before → after | Retained / peak bytes before → after | Net retained / peak reduction |
|---|---:|---:|---:|---:|
| One | 1,715 → 1,709 | 289,021 → 288,511 | 239,008 → 238,498 | 510 bytes |
| Four mixed | 2,097 → 2,073 | 337,293 → 335,253 | 280,257 → 278,217 | 2,040 bytes |
| Three equal-sized | 1,825 → 1,813 | 303,269 → 302,249 | 251,230 → 250,210 | 1,020 bytes |

Each committed/spare view retains its own 37-byte header and unchanged byte-owner stamps. One/four/three fixtures eliminate 3 / 12 / 6 row study requests per view and reduce row vectors to 1,392 / 5,568 / 2,784 bytes. Net per-view reductions are 255 / 1,020 / 510 bytes; both-view totals are 510 / 2,040 / 1,020, including actual row/index/reader overhead. No new reader storage is retained.

| Selections | Whole owner setup work before → after | Temporary capture work before → after | Capture-overlap whole requested peak before → after |
|---|---:|---:|---:|
| One | 349,577 → 347,435 | 45,731 → 45,659 | 79,105 → 78,850 bytes |
| Four mixed | 464,059 → 443,099 | 88,848 → 75,888 | 100,844 → 99,824 |
| Three equal-sized | 382,040 → 375,879 | 56,048 → 53,888 | 86,591 → 86,081 |

Temporary capture remains five requests / 43,280 bytes, map census 94,360 bytes, candidate maps 47,180 bytes / 562 requests, and release before spare setup. Canonical/source reads, split key copying and header-first scanning have their actual metering charges; no historical redundant comparison charge is retained just to reproduce an old number. Native snapshots, candidate mappings, atomic publication, subsequent ticks and final domain/weak release remain checked.

## Qualification and refusal witnesses

Final source/tree and clean status match before/after every profile. Matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules and runtime bitcode `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are pinned. CPU/x64, Clang/LLVM 15 and GCC 11 native wrappers, warm/viewport enabled, CUDA/GPU qualification disabled.

- Quick CI: three CTests, 632 independent placement arithmetic samples and the placement-capacity witness.
- Clang: sixteen CTests covering projection, membership, structural admission, prepared owners on all four worker/host-path modes, workbench/origins and viewport consumers; thirteen hierarchy invocations.
- GCC 11: thirteen corresponding CTests on the selected modes and eight hierarchy invocations.
- Standalone GCC 11/13: all four value/projection CTests each; 5,463 projection and 5,791 membership checks per compiler.

The comparison checks 360 source-pinned before/after record pairs, including twenty whole-owner totals, forty study-owner views, sixteen factory/copy observations and 1,808 unchanged export allocation refusals. Ninety-eight complete records remain identical; 922 numeric fields change within the explicitly allowed storage/work categories. The retained pair files identify every change. All six native cost probes and both full mixed-owner observations agree on the measured fixture totals. Full owner cases cover one-worker bulk and two-worker direct on Clang, and two-worker direct on GCC; cost probes additionally cover all four Clang worker/path modes and two GCC modes.

Both v1/v2 key fixtures exercise uint64 maximum entries, path ordinal 10, complete opaque text, foreign studies with identical numeric keys, changed entry/path, malformed row indices, zero/one-short work and exact allocation-free retry through the production reader. Fifteen successful matches charge 1,318 units; fifteen full lookups charge 2,879. There is no retained reader state. A compile-time control rules out the former raw-row contextual overload.

Actual membership collection refuses a foreign byte-owner study and fully unwinds. Structural admission refuses foreign-study body bindings with identical numeric projection keys and malformed definition/source/node/entry/path facts, then retries successfully. Real owner preparation exercises seven faults in both views, including wrong in-range definition/source indices and independent full entry/path checks. Actual binding capture refuses a foreign-study mapping, invalidates its authority and retries frozen without allocation. Both committed/spare key readers refuse that same foreign context. Existing ordinary/selected copy/move/same- and unequal-domain assignment, quotas/work, full allocation sweeps, weak/source/domain lifetime, frozen publication and following native ticks remain exercised.

Development logs retain an ambiguous fixture-name compile failure and the v2 test's inactive focal-field refusal. Qualification fixes name the intended membership fixture and its active byte focus; production validation is unchanged. They are excluded from final pass counts. Existing post-PASS Taichi cache-lock warnings are retained; caches and persistent data were not cleared.

Verified archive: `/tmp/hcp-projection-keys-68062bde-evidence.tar.gz`, SHA-256 `946680c4831465924956b23a085b7e95f01e9a9b621201ca1e2804fdebc50a8a`. It retains all four final profiles, exact commands/configurations/toolchains/submodules, binary/runtime identities, raw outputs, source-before/after and clean-status proofs, all comparison pairs, development failures and the original accepted inspection archive. Fresh extraction verifies the root and every nested manifest. Comparison and qualification scripts are included.

The before archive is contributor-retained evidence, not a new baseline run. Tested `6a208a14` and accepted `26dec2a0` differ only in documentation. Its runtime is identical to accepted #390's tested `223e874a`; source-equivalence patches and nested manifests are retained. Machine-local hashes identify inputs/artifacts without making raw evidence remotely accessible.

Next: review this bounded runtime migration and reconcile authoritative checks, parent-first on main through the maintainer. After scoped acceptance, select a separately bounded remaining #343 responsibility from actual ownership/native-cost evidence. #324, broader revision/admission policy and full GPU qualification remain separate.
