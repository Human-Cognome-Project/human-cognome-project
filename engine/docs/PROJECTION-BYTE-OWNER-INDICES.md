# Recipe-checked projected byte-owner indices

Status: **accepted and integrated in #394**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `ec5ca6875642e1a487ee0360c77582365d5c5cec`, tree `ba7c028a966446b4ecd22a2a4e6c05ffd01a55a7`, implements [#393 review 7047998750](https://github.com/Human-Cognome-Project/human-cognome-project/pull/393#issuecomment-7047998750) from accepted `521afbda`. At its original handoff, main `8627c262` integrated #392 and #393 remained a provisional dependency. Subsequently, main `dffbed2f` integrated qualified #393. [#394 review 7049930164](https://github.com/Human-Cognome-Project/human-cognome-project/pull/394#issuecomment-7049930164) accepts final `1f71eb76` and verifies that all nine post-test files are Markdown. Native [44803914461](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44803914461) and smoke [44803914617](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44803914617) subsequently passed and the maintainer integrated #394 at `66b7ef3a`. Local CPU evidence and original measurements below retain their exact source and scope.

## Independent owner and bounded contextual reads

Private byte provenance replaces its complete owner key with a retained 32-bit aggregate-owner row index. The index is independent of the byte entry. Temporary normalization metadata assigns each sorted projection row an index after checking the width bound; the validated selection supplies the aggregate index for each byte. Only the index, complete character pin and character/byte ordinals enter the private row. Existing recipe/source indices, activity, node/slot and complete-character pins remain.

The measured private byte payload is 152 bytes (formerly 240), optional payload 160 (formerly 248), and private occurrence 376 (formerly 464). The four-byte index plus alignment is already inside that width. Public `ByteProvenance` and 640-byte `ProjectedOccurrence` remain complete owning values. Parent contribution keys retain their full identities.

Every contextual definition read validates its row against the original indexed v1/v2 recipe source, including bounds, full numeric path, node, canonical scoped definition and positive mass. For bytes it also resolves the retained owner against that owner's independent recipe source, requires an aggregate with empty path and the same full entry, checks both ordinals against the byte path, and compares the complete character pin/revision to the aggregate's ordered parent. Wrong in-range owners sharing the same character refuse. Inactive aggregate owners remain valid logical owners.

Construction already validates exact UTF-8 character-to-byte construction and declared selection paths against the immutable complete recipe. The contextual resolver reuses those sealed invariants after independent source and parent-pin checks. It does not rescan tool/selection tables on frozen paths or substitute the inspection's stronger replay for production validation. No registry, new recipe lender or native identity authority is introduced. Native claims/incarnation/generation, atomic publication and subsequent ticks keep their existing authority.

`ProjectionAccess::byte_owner_index(projection,index)` performs that complete read before returning the owner index. All readers remain coupled to the owning projection and checked indices. Complete external study/entry/path association still uses the owning header; a foreign full key cannot silently acquire its namespace.

Membership collection now follows the retained owner index directly. Its temporary entry-to-root map and its comparator are removed, since the source/provenance resolver supplies the checked association. Contextual groups still come from the owning letter, preserving English/French contexts for a shared core byte. The map removal changes temporary construction requests and work, not membership rows, unique target pins or detached membership output.

## Copy, export and refusal boundaries

Ordinary/selected copies own independent headers, rows and complete recipes. Only the existing private owner-spare path shares its exact immutable recipe. Staged destination-domain assignment and moves transfer the correlated header/recipe/rows/indices. Census and independent backing visitors remove only the private owner UUID/path buffers and count actual narrower row/index capacity; complete character/other pins and parent keys retain their domain and ownership.

`occurrence(index)` resolves the complete source/owner association before copying. Named owners stage independent source/definition/provenance values for supported GCC 11 unwind behavior, then reconstruct the public owner key from the existing study header and checked aggregate row. All public study/revision/character/ordinal values, including the empty owner-path allocator, detach through source/projection/domain death.

Fourteen public exports remain 113 requests / 3,326 nested bytes, with 8,960 caller-reserved output bytes and 640-byte values. All 1,808 individual export allocation refusals and exact retries remain active across final configurations. Setup/retention savings do not discount those output costs.

## Actual public factory and copy costs

The source-pinned [accepted inspection](PROJECTION-BYTE-OWNER-KEYS.md) supplies before evidence; it is not rerun merely to establish a baseline.

| Actual operation | Requests before → after | Cumulative bytes before → after | Retained bytes before → after | Peak bytes before → after |
|---|---:|---:|---:|---:|
| Public v1 projection | 1,493 → 1,484 | 124,298 → 121,237 | 62,634 → 60,893 | 65,786 → 64,045 |
| Public v2 projection | 1,473 → 1,464 | 117,340 → 114,279 | 52,430 → 50,689 | 55,858 → 54,117 |
| Selected-domain v2 copy | 548 → 539 | 49,570 → 48,005 | 49,570 → 48,005 | 49,570 → 48,005 |
| Ordinary v2 copy | 548 → 539 | 49,570 → 48,005 | 49,570 → 48,005 | 49,570 → 48,005 |

All four retain fourteen rows/nine byte alternatives. Owner-key requests fall nine / 333 bytes to zero. Row-vector requests fall 7,424→6,016 at capacity sixteen and 6,496→5,264 at capacity fourteen, including owner-index capacity of 64 / 56 bytes. Retained/peak reductions are 1,741 bytes for the public factories and 1,565 for copies. Cumulative public reduction is 3,061 because narrower vector relocation is also observed. The existing study header remains 37 bytes; no inline extent is counted twice.

Actual contextual definition-census work rises 26,414→41,588 as every byte read checks its independent owner source and full parent pin. The direct nine-byte owner resolver charges 3,618 units; the stronger construction/provenance replay charges 21,866 and performs different duties. Legacy study-observer comparison counters now include actual byte-owner resolution; they are not evidence of retained UUID comparisons. These are metering units, not timing claims.

The live/catalogue membership factories eliminate three temporary map requests / 144 cumulative and peak bytes in their three-selection fixture: live requests 196→193, cumulative bytes 13,739→13,595 and peak 12,763→12,619; catalogue requests 200→197, cumulative 14,787→14,643 and peak 13,283→13,139. Retained membership rows/targets stay 2,175 / 2,711 bytes. Selected live/catalogue preparation work rises 57,717→58,913 and 75,724→77,520. Ordinary catalogue copying remains 28 requests / 2,303 bytes. This distinguishes temporary map savings from retained projection savings and stronger source/provenance checks.

## Both actual adopted views and whole operation

| Selections | Requests before → after | Cumulative bytes before → after | Retained / peak before → after | Net retained / peak reduction |
|---|---:|---:|---:|---:|
| One | 1,709 → 1,704 | 288,511 → 287,787 | 238,498 → 237,822 | 676 bytes |
| Four mixed | 2,073 → 2,053 | 335,253 → 332,357 | 278,217 → 275,513 | 2,704 |
| Three equal-sized | 1,813 → 1,804 | 302,249 → 300,827 | 250,210 → 248,932 | 1,278 |

| Selections | Owner setup work before → after | Temporary capture work before → after | Capture-overlap whole requested peak before → after |
|---|---:|---:|---:|
| One | 347,435 → 351,112 | 45,659 → 45,659 | 78,850 → 78,512 bytes |
| Four mixed | 443,099 → 460,029 | 75,888 → 80,384 | 99,824 → 98,472 |
| Three equal-sized | 375,879 → 381,526 | 53,888 → 54,731 | 86,081 → 85,442 |

Owner keys make no private UUID/path request. Each one/four/three-selection view retains 12 / 48 / 24 owner-index capacity bytes within row vectors of 1,128 / 4,512 / 2,256 bytes. Net per-view reductions are 338 / 1,352 / 639 bytes; both-view reductions are 676 / 2,704 / 1,278, including actual row/index/reader overhead. No reader object or additional control block is retained.

Temporary capture remains five requests / 43,280 bytes, map census 94,360 and candidate maps 47,180 bytes / 562 requests. Work reflects actual stronger contextual checks and removal of the temporary membership owner map; the old replay is not imposed on every lookup. Native snapshots, old/current candidate maps, generation/readiness and following ticks remain checked. No numerical kernel or #324 mathematics changes.

## Source-pinned qualification

All four final profiles exit zero with exact unchanged clean source/tree before and after. CPU/x64; Clang/LLVM 15 and GCC 11 native wrappers, warm/viewport enabled, CUDA/GPU qualification disabled. Matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules and runtime bitcode `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` remain pinned. Full toolchain/configuration, binary identities and direct C++ ABI measurement are retained.

- Quick CI: three CTests, 632 placement arithmetic samples and placement capacity.
- Clang: sixteen projection/membership/admission/owner/workbench/origin/viewport CTests and thirteen hierarchy invocations, including all four worker/path cost modes and full owner cases on one-worker bulk / two-worker direct.
- GCC 11: thirteen corresponding CTests and eight hierarchy invocations, including both selected cost modes and full two-worker direct owner cases.
- Standalone GCC 11/13: all four CTests each; 5,908 projection and 5,813 membership checks per compiler.

The comparison retains 460 before/after record pairs, twenty whole-owner totals and all 1,808 unchanged export allocation-refusal/retry controls. Of those records, 122 remain identical; 1,352 numeric fields change within the explicitly checked storage/work categories. Old full-key witness and new owner-source resolution meters are preserved as different duties, not silently compared as equivalent readers. All six cost probes and both full mixed-owner observations agree on the measured totals. Native fault/retry records and numerical snapshots remain in the raw outputs.

Real consumers reject malformed owner indices, including another valid aggregate sharing byte/character definitions, and byte-as-owner role. Membership binding rejects another study with identical numeric row/owner indices. Complete external owner paths remain checked. Public export refuses wrong owner/source/ordinal facts before copying. Stale full character revision, independent source index, zero/one-short work and exact allocation-free retry are exercised, alongside maximum entry, repeated/multibyte paths, inactive owners and distinct language contexts.

Both real committed/spare views exercise twelve source/provenance faults, including owner bounds/role, character/byte ordinals and full character revision. The mixed-owner case separately substitutes another valid aggregate index in both views, checks unchanged native/owner state and reservation backing, then retries frozen and proceeds through publication/following ticks. Structural admission returns structured `InvalidInput`, unwinds and retries after wrong owner indices. Existing ordinary/selected copy/move/same- and unequal-domain assignment, all allocation/quota/work refusals, source/domain/weak release and native claim/generation controls remain active.

Verified archive: `/tmp/hcp-projection-byte-indices-ec5ca687-evidence.tar.gz`, SHA-256 `71299ed9b7503efb8c30dec85e68e8f3238cf7312bb20fddcfa94d5d6429c272`. It retains all final profiles, raw observations, exact commands/configurations/submodules, source-before/after and clean-status proofs, binary/runtime/ABI identities, every comparison pair, development output and the accepted before archive. Fresh extraction verifies the root and every nested manifest.

The accepted tested before `502f555d` and reviewed #393 `521afbda` differ only in ten Markdown files. Source equivalence, unchanged fork/build/boundary patches, raw before/after pairs and preserved development output are retained. Machine-local hashes identify evidence without making raw artifacts remotely available. Existing post-PASS Taichi cache-lock warnings are preserved; caches, retained stores and Git history were not cleared.

The [parent study inspection](PROJECTION-PARENT-STUDY-IDENTITY.md) follows final `1f71eb76`; #395 accepts the [bounded header implementation](PROJECTION-PARENT-HEADER.md) direction. Its review and CI remain separate. Character/complete-character pins, broader revision/admission policy, #324 and GPU qualification remain separate.
