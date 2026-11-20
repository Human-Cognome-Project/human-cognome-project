# Projection-owned parent study identity

Status: **contributor implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `22482caa22f04a80b673f8bc9d2ba4cc95a7ea1d`, tree `f90e030aa47be609f8e2f12dd77701289a7d70aa`, implements [#395 review 7050686178](https://github.com/Human-Cognome-Project/human-cognome-project/pull/395#issuecomment-7050686178) from accepted `c68d3751`. Main `66b7ef3a` integrates qualified #394 and has its reviewed `1f71eb76` tree. The accepted #395 inspection remains an unmerged dependency while its native gate finishes. Local CPU evidence qualifies the scope below; authoritative GitHub CI and maintainer integration remain separate.

## Owned context and complete active output

Private `ProjectionParentContribution` rows retain full numeric entry/path, field kind and complete field object, ordinal and exact count. Their existing projection header owns the study namespace once for both active/spare pools. The actual private row is 160 bytes, down from 208; the public `ParentContribution` remains 208 bytes with a complete independently owning occurrence key. No new recipe lender, index table, reader object, control block or native identity authority is retained.

`parent_count()` reports only active contributions. `parent(index)` checks that bound before copying output, then reconstructs its complete key and field object using named staging for supported GCC 11 unwind. Each extracted value survives source/recipe/projection/domain destruction, including an empty path allocator. The public `parents()` vector-reference accessor is removed; all in-tree consumers use count/index owning extraction. No public inactive-pool accessor is introduced.

Internal reads take the owning projection, active/spare selector and checked index through `ProjectionAccess::parent`. Full-key matching checks the existing header and complete numeric source; its split-key overload takes the same complete UUID/source values without constructing a temporary key. No detached numeric row acquires context. Foreign studies with identical numeric parent identity, changed entry/path and malformed pool/index fail their respective boundaries.

Construction already validates the exact recipe and publishes ordered source/parent fields. Those immutable row values, selected-domain copies and existing header/buffer checks supply the production invariant. Contextual parent reads do not scan source/recipe tables; the inspection's stronger source/role/field/ordinal/count replay remains test-only. Source/definition revision checks and native claim/incarnation/generation keep their separate authorities.

## Pool ownership and publication

Initial publication preserves source and parent order. Ordinary copies detach; explicit selected copies construct each source path and complete field object in the destination domain. Moves carry correlated header, recipe, rows and allocators. Both pool capacities and every constructed field/source buffer remain charged by the production census and independently visited by the allocation observer; only repeated study UUID payload and inline row extent disappear.

`reserve_exposure` still provisions both vectors for the complete edge inventory and creates inactive contributions. `prepare_exposure` still validates the candidate header/recipe/source association, transfers the existing objects, finds each complete source/field/ordinal and reorders active contributions before moving the remainder back to the private pool. Checked contextual indices supply the lookup; comparisons no longer repeat a UUID already owned by that projection. Move/swap/copy charges use the actual 160-byte row and do not charge removed study copies. The complete transfer is prepaid before moving, preserving exact retry after work refusal.

Publication swaps the same committed/spare projection pointers. Row order, active/inactive role, full source, field kind/object, repeated-target ordinal and count remain checked against prepared native participation. Neither numerical state nor typed parent metadata becomes a second native execution authority.

## Actual factories and copies

| Actual operation | Requests before → after | Cumulative bytes before → after | Retained bytes before → after | Peak bytes before → after |
|---|---:|---:|---:|---:|
| Public v1 projection | 1,484 → 1,470 | 121,237 → 119,231 | 60,893 → 59,607 | 64,045 → 62,759 |
| Public v2 projection | 1,464 → 1,450 | 114,279 → 112,273 | 50,689 → 49,403 | 54,117 → 52,831 |
| Selected-domain v2 copy | 539 → 525 | 48,005 → 46,815 | 48,005 → 46,815 | 48,005 → 46,815 |
| Ordinary v2 copy | 539 → 525 | 48,005 → 46,815 | 48,005 → 46,815 | 48,005 → 46,815 |

Each fixture retains fourteen active parent rows. Their study strings fall fourteen requests / 518 bytes to zero. Parent row buffers fall 3,328→2,560 bytes at capacity sixteen, or 2,912→2,240 at capacity fourteen. Net retained/peak reductions are 1,286 bytes for public factories and 1,190 for copies. Public cumulative reduction is 2,006 because narrower vector relocations also contribute. The existing header's 37 bytes remain charged. These are actual requests and retained capacity, not RSS or allocator metadata.

The fourteen-row contextual key comparison remains 1,242 metered units. The stronger independent provenance replay changes 9,204→8,182 after removing per-row study-stamp comparisons; it performs different duties and is not a lookup-speed claim. The actual contextual occurrence-definition census remains 41,588 units. No whole-operation timing or GPU qualification is inferred.

## Both actual adopted views

| Selections | Requests before → after | Cumulative bytes before → after | Retained / peak before → after | Net retained / peak reduction |
|---|---:|---:|---:|---:|
| One | 1,704 → 1,694 | 287,787 → 286,361 | 237,822 → 236,492 | 1,330 bytes |
| Four mixed | 2,053 → 2,013 | 332,357 → 326,077 | 275,513 → 270,193 | 5,320 |
| Three equal-sized | 1,804 → 1,786 | 300,827 → 298,049 | 248,932 → 246,538 | 2,394 |

Each one/four/three-selection view retains the same five / twenty / nine parent edges across both pools, with each pool reserving that complete inventory. Parent-vector backing per view falls 2,080→1,600 / 8,320→6,400 / 3,744→2,880 bytes. Removing the original 185 / 740 / 333 study bytes gives actual per-view reductions of 665 / 2,660 / 1,197 bytes. Cumulative constructor reductions are larger because an initial active vector is relocated into its full reservation: released setup backing falls another 96 / 960 / 384 bytes per owner.

Both active and inactive study backing are zero. Existing headers remain 37 bytes per view. All parent field scope/address and source-path backing remain independently owned; the actual row vectors contain no study UUID member. Observed savings include actual replacement/pool overhead and exactly match removed study backing plus the measured row-capacity reduction across both views.

| Selections | Owner setup work before → after | Temporary capture work before → after | Capture-overlap whole requested peak before → after |
|---|---:|---:|---:|
| One | 351,112 → 350,250 | 45,659 → 45,659 | 78,512 → 78,427 bytes |
| Four mixed | 460,029 → 456,509 | 80,384 → 80,384 | 98,472 → 97,622 |
| Three equal-sized | 381,526 → 379,948 | 54,731 → 54,731 | 85,442 → 85,102 |

The temporary adoption capture remains five requests / 43,280 bytes, map census 94,360 and candidate maps 47,180 bytes / 562 requests. Capture work and map storage remain unchanged; native snapshots and map contents still match. Setup work falls with removed copies and narrower accounting; exposure preparation also stops repeating study comparisons and charges actual move widths. Those metering reductions preserve work refusal/retry and are not timing measurements.

## Detached output and controls

Thirty-four active parent exports retain 132 requests / 3,266 nested bytes and 7,072 caller-reserved row bytes. The reserved-pool v2 lifetime control also makes twenty-four test-only inactive copies: all fifty-eight values retain 214 requests / 5,482 nested bytes and 12,064 row bytes, unchanged from the accepted inspection's complete copies. Only the active subset uses the new public API; the spare-copy control creates no public inactive export. Existing fourteen occurrence exports remain 113 requests / 3,326 nested bytes and complete 640-byte values. Setup/retention savings do not discount any output costs.

All individual active export and private lifetime-copy allocation refusals unwind, preserve selected storage and retry with exact source/field/ordinal/count. Bounds checks keep the spare pool private. Full-key and split-key production matching preserve study/entry/path, reject zero/one-short work and retry with exact work while every allocation is denied. Maximum uint64 entry, ordinal ten, repeated targets at distinct ordinals, multibyte paths, ordinary/selected copies, moves and domain/weak release remain covered. Existing assignment and constructor failure controls remain active.

Both real committed/spare views inspect parent meaning and independent backing after every mixed-owner publication with allocation forbidden. Native publication, following ticks, survivor values, full handles, generation/readiness and reservation/map/coordinator backing retain their existing controls. The full owner failure sweeps preserve rejected/abandoned intent and retry without new backing. No mathematical kernel, #324 behavior, broader revision/admission policy or character-pin ownership changes.

## Source-pinned qualification

All four final profiles exit zero on unchanged clean source before and after. CPU/x64; native Clang/LLVM 15 and GCC 11 wrappers, warm/viewport enabled, CUDA/GPU qualification disabled. Matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules and runtime bitcode `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` remain pinned. Exact commands/configurations, binary/runtime identities and raw outputs are retained.

- Quick CI: three CTests, 632 placement arithmetic samples and placement capacity.
- Clang: sixteen projection/membership/admission/prepared-owner/workbench/origin/viewport CTests and thirteen hierarchy invocations, including full one/four/three owner cases on one-worker bulk / two-worker direct and all four worker/path cost probes.
- GCC 11 native: thirteen corresponding CTests and eight hierarchy invocations, including full two-worker direct owner cases and both selected cost probes.
- Standalone GCC 11/13: all four CTests each; 8,770 projection and 5,813 membership checks per compiler, alongside the structural recipe and bootstrap reservation controls.

The sealed comparison retains 630 before/after record pairs, including twenty-one whole-owner totals, 184 parent observations and thirty-six sampled publication pool records. It checks 1,220 changed numeric fields within explicit storage/work categories; 365 complete records remain identical. Every observed whole-owner retained/peak reduction exactly matches removed parent study backing plus the actual row-capacity reduction across both views. All 1,808 existing occurrence-export and 2,768 parent value-copy allocation refusals retain their counts and exact retry controls. Numeric key work, capture storage/work, maps, detached output costs and structural/native meaning remain stable.

The accepted tested before `a9011a88` and reviewed #395 `c68d3751` differ only in nine Markdown files. That source equivalence, initial main `66b7ef3a`'s file equality with reviewed #394 `1f71eb76`, unchanged fork/build/boundary patches and local/canonical tree equality are retained. The accepted baseline was reused, not rerun merely to onboard.

Verified archive: `/tmp/hcp-projection-parent-header-22482caa-evidence.tar.gz`, SHA-256 `2d50df7eea816f43af4a3f27d264c7b7aeaa825c5616ddbe2968ea13e6d5cfcc`. It retains all final profiles, exact commands/configurations/submodules, raw outputs, clean source-before/after proofs, binary/runtime identities, every comparison pair, development output and the accepted before archive. Fresh extraction verifies the root and every profile manifest. The accepted before archive has SHA-256 `2b2105ad0a2eef13d649001203ce65c5978903bc1f0116813ff09de4174f554e`; its root/profile manifests are checked too. Machine-local hashes identify contributor evidence without making raw artifacts remotely available.

The standalone development suite passed before the final source pin; its output is retained separately from qualification. Final profiles cover the canonical source including removal of the unused full-key copy/census helpers. Existing post-PASS Taichi cache-lock warnings remain. Caches, retained stores and Git history were not cleared.

Next: review this runtime and reconcile authoritative GitHub CI before maintainer integration. After scoped acceptance, choose one further bounded #343 inspection from actual ownership/native-overlap evidence. Full character/complete-character pins, field-target ownership, broader revision/admission policy, #324 and GPU qualification remain separate.
