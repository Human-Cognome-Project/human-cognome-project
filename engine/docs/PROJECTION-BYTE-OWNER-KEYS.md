# Projected byte-owner keys and contextual ownership

Status: **inspection provisionally accepted in #393**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `502f555d26a3eafd87b94a84d0fb678cd38216d8`, tree `17d0078b43f4123524bf57ebea7fc0fde0d283c9`, follows accepted #392 `51912690660f02db38a69c80b12666994e1aca03` ([review 7047017029](https://github.com/Human-Cognome-Project/human-cognome-project/pull/392#issuecomment-7047017029)). At its original handoff, main `9d24287a` integrated qualified #390/#391 and its tree equaled reviewed #391. The inspection was stacked on the explicitly reviewed #392 head. Only seven test files change before the documentation handoff. **Production layout, behavior and achieved runtime saving are unchanged: zero saving.** [#393 review 7047998750](https://github.com/Human-Cognome-Project/human-cognome-project/pull/393#issuecomment-7047998750) accepts final `521afbda`, verifies all ten post-test files are Markdown and authorizes the [independent owner-index implementation](PROJECTION-BYTE-OWNER-INDICES.md). #392 passed hosted qualification and is integrated at `8627c262`; remaining #393 native qualification is pending. Original attribution and zero saving below apply to this inspection source. All integration remains maintainer work.

## Actual ownership and consumer

After [#392's header migration](PROJECTION-HEADER-IDENTITY.md), private rows retain numeric entry/full path under an owned study header, but `ByteProvenance::owner` still independently owns a complete `OccurrenceKey`. Construction copies the study UUID and aggregate entry with an empty owner path; projection/value/domain copies repeat that complete key. The storage census counts its UUID/path backing. Each byte retains its character full pin and both ordinals; complete-character and parent contribution keys are separate surfaces.

The production consumer audit finds membership collection using this owner key to associate a byte with its owning letter's contextual groups. Its complete study/entry/path comparison prevents a shared core byte from acquiring another letter's language memberships. This logical owner may be inactive during Parts exposure. The projection retains all alternatives before native admission; an inactive logical aggregate need not have a live native particle claim. Existing Taichi `RecordHandle` context/incarnation/address/claim identity therefore cannot replace this semantic association. This is a source/interface finding, not new hardware qualification.

Public `occurrence(index)` continues to copy full owning `ProjectedOccurrence` values. The current 640-byte public shape, complete owner namespace, character revision, ordinals and source identity survive source/projection/domain death. The inspection additionally checks the empty exported owner-path allocator detaches, since an empty path must not silently retain a domain. No output becomes a contextual loan.

## Actual factory and copy attribution

The observer visits each actual owner UUID buffer, proves distinct backing from the projection header and other byte owners, checks the selected domain, and matches pointer/request size to the live allocation table. Existing independent backing visitors prove the two owner views do not share those stamps. Only integer observations escape the live allocation window.

| Actual operation | Row count / capacity | Byte rows / active bytes | Distinct aggregate owners | Owner UUID requests / bytes | Existing header bytes | Engaged owner-key inline attribution | Owner-key capacity attribution within whole row vector | Whole row-vector request |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Public v1 projection | 14 / 16 | 9 / 6 | 5 | 9 / 333 | 37 | 864 bytes | 1,536 bytes | 7,424 bytes |
| Public v2 projection | 14 / 16 | 9 / 6 | 5 | 9 / 333 | 37 | 864 | 1,536 | 7,424 |
| Selected-domain v2 copy | 14 / 14 | 9 / 6 | 5 | 9 / 333 | 37 | 864 | 1,344 | 6,496 |
| Ordinary v2 copy | 14 / 14 | 9 / 6 | 5 | 9 / 333 | 37 | 864 | 1,344 | 6,496 |

The measured `OccurrenceKey` width is 96 bytes: a 48-byte UUID member plus eight-byte entry and 40-byte path header. Every byte-owner path is empty with zero capacity, so it makes no path-buffer request. Inline attribution includes optional storage in aggregate/unused row slots and is already inside the unchanged 464-byte private row vector. It is neither an additional allocation nor an additive net saving. The observation chooses no replacement width, index overhead, recipe pin or control block.

All construction/copy requests, cumulative bytes, retained/peak bytes and explicit export costs match accepted #392. Fourteen detached exports remain 113 requests / 3,326 nested bytes; 8,960 output-capacity bytes are caller-reserved separately. All 1,808 export allocation-refusal/exact-retry controls remain active across the final four value configurations.

## Both real adopted views

The same observer runs without allocation inside the actual committed/spare adoption window, while the allocation table and domain remain alive.

| Selections | Rows per view | Byte rows / active bytes | Unique owners | Owner UUID requests / bytes per view | Existing header | Engaged key attribution | Key capacity attribution | Whole row vector |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| One | 3 | 2 / 0 | 1 | 2 / 74 | 37 bytes | 192 bytes | 288 bytes | 1,392 bytes |
| Four mixed | 12 | 8 / 4 | 4 | 8 / 296 | 37 | 768 | 1,152 | 5,568 |
| Three equal-sized | 6 | 3 / 1 | 3 | 3 / 111 | 37 | 288 | 576 | 2,784 |

Both mixed views retain sixteen owner UUID requests / 592 bytes, beside two existing headers / 74 bytes. Their 2,304 owner-key capacity bytes are already part of the 11,136 row-vector bytes. The heap requests and inline extents are distinct candidate costs, not implemented whole-owner savings.

Whole-owner retained/requested peaks stay 238,498 / 278,217 / 250,210 bytes; setup work stays 347,435 / 443,099 / 375,879. Capture stays five requests / 43,280 bytes with work 45,659 / 75,888 / 53,888. Map census remains 94,360 bytes and candidate maps 47,180 bytes / 562 requests. The full mixed-owner control still checks frozen preparation/publication, native snapshots, following ticks and complete domain/weak release.

## Checked association witness and actual refusals

A test-only replay takes the owning projection plus independent byte and aggregate-owner row indices. It validates both through the production source resolver, requires the aggregate role and matching full entry, and checks byte path/ordinals, full character pin, canonical character-to-byte pin and exact declared selection-part association in that projection's immutable recipe. It permits inactive logical aggregate owners. A separate complete-key boundary compares the external study and owner path before replay. No detached numeric row can supply another projection's header, and no replay state is retained.

The existing full-key witness charges 1,618 units in each fourteen-row factory/copy; the conservative index/source/provenance replay charges 19,337. Per native view the corresponding totals are 296 / 5,761, 1,412 / 23,056 and 474 / 3,366. **These totals perform different duties.** The first measures complete-key lookup/equality; the second replays source and provenance checks performed across existing boundaries. They do not compare interchangeable runtime readers or predict future setup/frozen work. Actual production work is unchanged.

Both v1/v2 replay fixtures retain twenty byte alternatives, repeated ASCII and multibyte characters, maximum uint64 entry, character ordinal 10 and inactive bytes. Wrong in-range owner indices, byte-as-owner role, malformed indices, foreign study with identical numeric keys, nonempty external owner path, stale full character revision and malformed indexed byte source refuse. Zero/one-short work refuses; exact retry succeeds with every C++ allocation denied.

The actual membership consumer separately refuses a foreign study, another valid owner entry using the same core byte, and a nonempty owner path. Every failure fully unwinds; healthy retries restore independent English/French contexts. Six such refusals and retries per value configuration provide twenty-four total refusal/retry controls. Replay is an inspection witness; production still uses the original full owner keys and all existing native guards.

## Bounded proposal for review

Review replacing only the private byte-owner full key with an independently retained aggregate-owner row index in the already owned projection. Do not silently derive away the association from the byte entry. Select bounds/width and settle checked contextual reads with the owning projection and both indices. Preserve complete external study/path boundaries; independent recipe source indices, aggregate role/full entry, character/full revision/ordinals, exposure, membership and native generation/claim validation remain required. Construction, copy/assignment/move and both exposure views must keep header/recipe/rows/owner indices correlated.

Keep public byte provenance and occurrence exports complete and independently owned, including empty-path domain detachment after every source dies. Character/complete-character pins, parent contribution keys and other provenance remain outside this first proposal. No new registry, UUID lender, native identity substitution, cross-revision sharing or generalized admission policy is proposed.

A runtime follow-up requires explicit review, then actual row/index/reader/export overhead, whole-owner retained/peak/work and failure behavior with the replacement in place. Exercise wrong in-range owner/source indices, cross-study identical keys, inactive owners, shared bytes in distinct languages, repeated/multibyte paths, source/domain/weak and same/unequal-domain value lifetimes, all allocation/quota/work refusals and exact retry, frozen native publication and following ticks. Do not infer net savings from this attribution. #324 and broader revision/resource/admission work remain separate.

## Source-pinned qualification

All final profiles exit zero with exact unchanged clean source/tree before and after. CPU/x64, Clang/LLVM 15 and GCC 11 native wrappers, warm/viewport enabled and CUDA/GPU qualification disabled. Fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules and runtime bitcode `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` remain pinned; full toolchain/configuration and binary identities are retained.

- Quick CI: three CTests, 632 placement arithmetic samples and the placement-capacity witness.
- Clang: projection/value, projection-native and membership-value CTests; four worker/path cost modes plus the full two-worker direct mixed-owner case (five hierarchy invocations).
- GCC 11: the same three CTests, cost modes on one-worker bulk / two-worker direct and the full two-worker direct mixed owner (three invocations).
- Standalone GCC 11/13: all four CTests each, with 5,795 projection and 5,799 membership checks per compiler.

The source-pinned comparison verifies all 360 pre-existing value/native cost records are identical, including twenty whole-owner totals and all 1,808 export allocation-refusal/retry controls. New observations cover sixteen actual factory/copy windows, forty real byte-owner views, eight relation fixtures and eight actual-consumer fixtures. Existing post-PASS Taichi cache-lock warnings are retained; caches and persistent data were not cleared. Development checks pass and remain separate from final pass counts.

Verified archive: `/tmp/hcp-projection-byte-owner-502f555d-evidence.tar.gz`, SHA-256 `6348aac33a522b95d775b9d7d880b530f849f02f280af5f68d14cb7247950f21`. It contains all final profiles, raw observations, exact commands/configurations/submodules, source-before/after and clean-status proofs, binary/runtime identities, every old/new comparison, development output and the accepted #392 before archive. Fresh extraction verifies the root and all nested manifests.

The accepted before source `68062bde` and reviewed #392 `51912690` differ only in documentation. Empty production-diff and source-equivalence patches are retained. Before evidence is reused, not rerun to onboard. Machine-local hashes identify artifacts without making raw evidence remotely accessible.
