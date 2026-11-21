# Recipe-backed projected parent targets

Status: **accepted and integrated in #398**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `c63bf01d084cb9946738f2bf54e7d19fea65bf10`, tree `ba6dcf1078dc305eb95eea2d98a79f35097e7d46`, implements [#397 review 7052622147](https://github.com/Human-Cognome-Project/human-cognome-project/pull/397#issuecomment-7052622147) from accepted `f27db64d`. Initial main `5d8c08ce` integrates qualified #395 and has its reviewed `c68d3751` tree. At the original handoff #396/#397 were provisional dependencies. [Maintainer acceptance 7062570528](https://github.com/Human-Cognome-Project/human-cognome-project/pull/398#issuecomment-7062570528) confirms #396–#398 passed hosted native/smoke and were integrated parent-first at `f706f056`, `a1ac5250` and `c2b2d4d2`. Each integration tree matches its qualified head; main has final `0ddb8b35`'s tree and all nine post-test changes are Markdown. Local CPU evidence qualifies only the recorded scope; authoritative GitHub CI remains separate.

## Independent selectors and complete output

Each private active/spare parent row retains full uint64 entry and numeric path, independent occurrence/recipe-parent selectors, field kind, ordinal and exact count. The actual row is 72 bytes, down from 160, including both 32-bit selectors. Initial publication and inactive reservation take known vector positions, checking selector width before narrowing. The parent selector is independent of the retained ordinal; the occurrence selector is independent of the retained source. No new recipe lender, target table, registry, reader object or native authority is retained.

The existing exact immutable recipe owns complete target references and their scope/address/revision backing. Contextual reads select the owning projection, pool and checked index. `resolve_parent_source` checks source-selector bounds, complete numeric source, field kind, the real production source/definition/provenance resolver, parent bounds and independently retained ordinal/count. The steady reader separately requires the indexed occurrence's current active/inactive role to match the selected pool. `parent_matches` validates complete study/entry/path association and that steady parent meaning; a foreign study or wrong valid selector cannot acquire context.

`parent_count()` / `parent(index)` remain active-only. Public extraction resolves before copying output and uses named staging to reconstruct a complete independently owning key and field object. Its 208-byte value, source order, kind, ordinal/count and detached scope/address/path allocators remain. Empty paths do not retain the selected domain. The inactive pool stays private. Native claims/incarnation/generation and revision policy keep their existing authorities.

Ordinary copies own independent recipes; selected copies construct source paths in their destination domain and preserve correlated selectors. Moves transfer header/recipe/rows together. The existing private owner-spare path shares exactly one immutable recipe. Census and independent visitors count actual narrower row capacity and source paths, retaining the existing recipe target pins once. Private field scope/address requests and inline field-object extent are removed from rows, rather than discounted against independently owning public output.

## Correct roles through preparation and retry

`prepare_exposure` first validates committed parent rows as steady active/spare pools. Candidate rows are checked for immutable source/selector/kind/ordinal/count meaning without requiring their current vector to express a final pool role. A work-refused attempt may legitimately leave a partially reordered candidate with both roles in the active vector. No public reader can request this role-independent check; it remains a private preparation boundary.

After setting target activity, preparation combines both candidate pools using the existing prepaid whole transfer. During search it first matches a row's full source and ordinal to the active occurrence being built, then applies the steady active reader to that matched row and compares the full target pin. Unrelated inactive rows are not incorrectly rejected while sharing the intermediate vector. Existing row objects and selectors move/swap together. The remainder transfers back to the private inactive pool with the complete move prepaid before mutation.

This preserves exact work-refusal retry while enforcing final roles. No full tool/selection-table replay is added to frozen production reads; source and parent lookup use the existing retained indices. The stronger independent source/ordinal search remains test-only. Numerical input and native execution state are unchanged by the representation.

## Actual factories and copies

| Actual operation | Requests before → after | Cumulative bytes before → after | Retained bytes before → after | Peak bytes before → after |
|---|---:|---:|---:|---:|
| Public v1 projection | 1,470 → 1,442 | 119,231 → 115,775 | 59,607 → 57,471 | 62,759 → 60,623 |
| Public v2 projection | 1,450 → 1,422 | 112,273 → 108,817 | 49,403 → 47,267 | 52,831 → 50,695 |
| Selected-domain v2 copy | 525 → 497 | 46,815 → 44,855 | 46,815 → 44,855 | 46,815 → 44,855 |
| Ordinary v2 copy | 525 → 497 | 46,815 → 44,855 | 46,815 → 44,855 | 46,815 → 44,855 |

The fourteen-row fixture removes 28 private field requests / 728 nested bytes. Actual parent vectors fall 2,560→1,152 bytes at capacity sixteen and 2,240→1,008 at capacity fourteen. Selector capacity of 128 / 112 bytes is already inside those buffers. Net retained/peak reductions are 2,136 bytes for factories and 1,960 for copies; public cumulative reduction is 3,456 because vector relocation also narrows. Existing header and recipe backing remain charged. These requested/capacity figures are not RSS or allocator metadata.

## Both actual owner views and work

Actual whole-owner constructor totals include both current/candidate views, both parent pools, selected maps and existing recipe/header backing:

| Selections | Requests before → after | Cumulative bytes before → after | Retained = peak before → after | Setup work before → after |
|---|---:|---:|---:|---:|
| One | 1,694 → 1,674 | 286,361 → 283,905 | 236,492 → 234,212 | 350,250 → 348,815 |
| Four mixed | 2,013 → 1,933 | 326,077 → 315,197 | 270,193 → 261,073 | 456,509 → 450,499 |
| Three equal-target | 1,786 → 1,750 | 298,049 → 293,241 | 246,538 → 242,434 | 379,948 → 377,266 |

Net retained/peak reductions are 2,280 / 9,120 / 4,104 bytes. They reconcile exactly with removed nested field backing of 520 / 2,080 / 936 bytes plus 88 bytes per actually retained parent-row capacity across both views and pools. All selectors are already charged inside the new rows. Temporary capture remains five requests / 43,280 bytes; selected map census remains 94,360 bytes and candidate map backing 562 requests / 47,180 bytes. Constructor work decreases overall because narrower rows and removed field copying outweigh added guards in these fixtures. No timing claim follows.

For the fourteen-parent projection, the actual steady source/parent guard costs 5,282 units, including fourteen final-role checks, versus 5,268 for the old inspection-only candidate guard. Complete target comparison costs 1,442 in both records. The independent test replay now costs 6,740 versus the old 13,450 selection experiment; these perform different duties. The legacy `numeric_comparison_work` counter grows 1,242→6,524 because matching now calls the real source/parent guard. Strong provenance replay grows 8,182→14,472 because its independently found source/ordinal is checked against the actual resolved target. All these observers allocate zero times. These counters describe bounded metered work, not elapsed time.

Full source/parent guards and committed/candidate prevalidation have different duties from old direct field reads. Preparation uses actual 72-byte copy/move/swap charges and removes the old field-object copy work; added source, target and role validation is measured explicitly. No work-meter reduction is presented as a timing speedup, and no CPU result qualifies GPU behavior.

## Detached values, malformed indices and frozen operation

Thirty-four active parent exports remain 132 requests / 3,266 nested bytes and 7,072 caller-reserved row bytes. The reserved v2 lifetime control additionally copies twenty-four private inactive values: all fifty-eight remain 214 requests / 5,482 nested bytes and 12,064 row bytes. Only the active subset uses the public API. Existing fourteen occurrence exports retain 113 requests / 3,326 nested bytes and complete 640-byte values. All individual output allocation refusals unwind and retry with complete source/field/ordinal/count and no retained source/domain loan.

Wrong in-range occurrence selectors with identical full target, ordinal/count and role reject their independently retained source. Wrong valid repeated-parent positions with identical full target/count reject the independent ordinal. Actual production readers and public export reject malformed source/parent bounds. Kind, count, entry, every path component and final pool role are faulted/restored separately. Zero/one-short work rejects; exact-work retry succeeds with all C++ allocations refused. Max entry, ordinal ten, repeated/multibyte paths, ordinary/selected copies, moves and domain/weak release remain covered.

Both real owner views and both parent pools exercise invalid selectors during staging. Each refusal preserves committed/native values, full handles, pending/readiness state and selected backing, then retries frozen before normal publication. Mixed-owner rounds check exactly one matching contribution per expected source/kind/target/ordinal/count, inspect both views after publication with allocation forbidden, and execute the following tick. Full constructor and staging work-boundary sweeps retain legitimate partial candidate partitions and prove exact retry. Broader revision/admission policy, #324 mathematics and other pin ownership remain separate.

## Source-pinned qualification

Canonical source `c63bf01d`, tree `ba6dcf10`, is pinned before/after every final profile with a clean worktree. LLVM/Clang 15 and GCC 11 wrappers use the matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53` and runtime-bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`; warm integration and viewport are enabled, CUDA qualification disabled. Commands, configuration, submodules, binary hashes and raw output are retained. The existing `ticache.lock` warnings occur after passing native results and remain in the logs; no persistent cache was cleaned.

The local scope passed `run_local_ci.sh quick` (three CTests plus placement arithmetic/capacity controls), sixteen Clang native CTests, thirteen GCC native CTests, and standalone GCC 11/13 builds with four CTests each. Each standalone compiler reports 12,708 projection and 5,813 membership checks. Native hierarchy coverage passed thirteen Clang invocations and eight GCC invocations: full single/mixed/equal owners in Clang one-thread bulk and two-thread direct plus GCC two-thread direct; bindings/commit/fields in two-thread direct; and adoption cost in all four Clang declaration/thread modes and GCC one-thread bulk/two-thread direct. GPU execution, the full hosted matrix and unrelated policy/mathematics are not qualified by these CPU checks.

The comparison validates 798 before/after record pairs, including 21 whole-owner totals, 152 target observations and sixteen selector-case records. It preserves 1,808 occurrence-export and 2,768 parent-export allocation refusals. All 4,728 selector faults and thirty native both-view/pool selector refusals pass. Storage and metered-work deltas are explicitly attributed; semantic, capture/map, recipe pin, public output, lifecycle and unchanged-native records compare equal. There are 1,788 attributed numeric field changes and 365 unchanged records. Legacy comparison/replay counter names retain their original provenance and do not imply identical duties after production guards are added.

Machine-local evidence: `/tmp/hcp-projection-parent-indices-c63bf01d-evidence.tar.gz`, SHA-256 `ccfe01169630cbd590d8ee3f22b38f734af001a4e242f7334142bff252457d1c`. The archive embeds the accepted `f551d14f` before bundle, SHA-256 `2b61ef6cda487690e49382d4de64a6c1e9820258eb74f7619a54c40a69f57220`; its nine-file Markdown-only child `f27db64d` has identical non-Markdown source. It retains commands, all four profiles, raw comparisons/deltas, source/unchanged-boundary proofs and manifests. The before bundle and every profile were verified; the final bundle was extracted afresh and its root/profile manifests verified. This local hash identifies retained evidence; it does not make the raw archive remotely available.

The next bounded [submission-lifetime inspection](PREPARED-SUBMISSION-LIFETIMES.md) follows that acceptance from integrated main. Original runtime/work/output measurements remain pinned above. Character/complete-character pins, broader revision/admission policy, #324 and GPU qualification remain separate.
