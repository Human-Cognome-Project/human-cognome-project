# Recipe targets in projected parent contributions

Status: **inspection accepted and integrated in #397**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source `f551d14f4dae90f130731a3ab74b5cdccecb89d3`, tree `488fb8bfcef517e51cda3360ec93e2555026c69f`, follows [#396 review 7051718353](https://github.com/Human-Cognome-Project/human-cognome-project/pull/396#issuecomment-7051718353) from accepted `2fc9d157`. Main `5d8c08ce` integrates qualified #395 and has its reviewed `c68d3751` tree. At the original handoff, #396 was an accepted, unmerged dependency. It subsequently passed native/smoke and the maintainer integrated it at `f706f056`. [#397 review 7052622147](https://github.com/Human-Cognome-Project/human-cognome-project/pull/397#issuecomment-7052622147) accepts final `f27db64d`, verifies all nine post-test files are Markdown and authorizes the [selector implementation](PROJECTION-PARENT-TARGET-INDICES.md). #397 and #398 subsequently passed hosted native/smoke and were integrated at `a1ac5250` and `c2b2d4d2`, confirmed in [acceptance 7062570528](https://github.com/Human-Cognome-Project/human-cognome-project/pull/398#issuecomment-7062570528). This contribution changes tests and documentation only. Production representation, behavior and achieved runtime saving at this inspection source are unchanged: **zero saving**.

## Ownership issue and actual buffers

After the [parent study migration](PROJECTION-PARENT-HEADER.md), each private active/spare row retains a complete `FieldKey`: kind plus scoped object address. The existing exact immutable projection recipe already owns that target in the source tool's ordered `Parent::target` full reference. Initial publication, value/domain copying and inactive-pool reservation duplicate the scoped object. Exposure changes move those independent objects between pools. The production census and allocation domain count them. Public active exports copy complete owning field objects separately.

The observer follows each actual parent source to its checked occurrence definition and exact recipe parent. It checks kind, ordinal, count and complete target object before comparing buffer identities. Each retained field scope/address buffer has distinct backing from the recipe target; both copies belong to the expected domain and independently match actual live C++ requests. The recipe target's scope, address and full revision backing are checked in its existing owner too. No pointer into caller input supplies its lifetime. Both adopted views already share one exact owned recipe; no new lender is needed to keep its targets alive.

All public v1/v2 factories and selected/ordinary v2 copies retain fourteen active field objects: 28 requests / 728 nested bytes, comprising fourteen scope strings / 518 bytes and fourteen addresses / 210 bytes. Seven distinct keys have observed representatives totaling 364 bytes. That is an attribution, not an implemented unique-target table. Source occurrences and repeated-target ordinals remain separate edges.

On the recorded x64 ABI, `FieldKey` occupies 96 bytes inside the actual 160-byte private parent row. At capacity sixteen its member attribution is 1,536 bytes within a 2,560-byte row vector; at capacity fourteen it is 1,344 within 2,240. These inline extents are already included in vector backing. They must not be added as separate heap requests.

## Both adopted views and publication

| Actual selections | Active / spare rows per view | Field requests per view | Nested field bytes per view / both views | Inline field capacity per view | Actual parent-vector bytes per view |
|---|---:|---:|---:|---:|---:|
| One | 1 / 4 | 10 | 260 / 520 | 960 | 1,600 |
| Four mixed | 10 / 10 | 40 | 1,040 / 2,080 | 3,840 | 6,400 |
| Three equal-sized | 4 / 5 | 18 | 468 / 936 | 1,728 | 2,880 |

Each pool reserves the complete edge count (five / twenty / nine). Only constructed rows own nested field backing. Distinct target counts in each initial active/spare pool are 1 / 4, 5 / 5 and 3 / 3 respectively; these per-pool counts must not be added as though cross-pool targets were distinct.

All thirty-two mixed-owner publication rounds inspect both current/spare views with allocation forbidden. Sampled rounds 0, 1 and 31 preserve the target/pin relation as active/inactive partitions change; the first current view changes ten / ten to thirteen / seven with the same total twenty field objects. Source paths, field kinds, repeated ordinals/counts, full handles, survivor values, generation/readiness, reserved map/coordinator backing and following ticks retain their existing controls. Target observation does not modify native or owner state.

## Checked selector experiment and tradeoff

A test-only candidate row shape keeps full numeric source, field kind, ordinal and count, plus two independent 32-bit selectors: occurrence row and position in that source tool's parent array. It measures 72 bytes including eight selector bytes, compared with the current 160-byte row. It is never used as production storage. Capacity-based selector attribution would be 80 / 320 / 144 bytes per native view across both pools; those bytes are already inside the proposed row shape, not additional table backing.

The resolver takes the owning projection, pool and checked row index plus those selectors. It independently compares the retained full source to the selected occurrence, verifies active/inactive role, uses the real production source/definition/provenance resolver, bounds the parent selector and checks independently retained ordinal/count and field kind. It then borrows the existing recipe's full target reference synchronously. It does not derive a selector from entry or ordinal, accept a detached numeric row, scan tool/selection tables on reads, substitute a native claim for logical identity, or establish a new revision policy.

| Actual fixture / per-view inventory | Stored-object comparison work | Indexed source/parent guard work | Selection-construction witness work |
|---|---:|---:|---:|
| Fourteen-row factory/copy | 1,442 | 5,268 | 13,450 |
| One native selection | 515 | 1,792 | 4,171 |
| Four mixed | 2,060 | 7,168 | 17,830 |
| Three equal-sized | 927 | 2,850 | 6,870 |

These meters perform different duties. Stored comparison checks the complete current field object against the recipe object; production's existing direct field read does not perform that comparison on every access. The candidate validates source and parent indices as well as retained facts. Its guard therefore cannot be described as a measured speedup. The setup witness searches existing rows to establish selector equivalence; a future constructor already visiting the source/parent could publish known positions, but its actual work must be measured. No timing or GPU claim is made.

Controls substitute another valid source having the same full target, ordinal/count and role; the independent full source rejects it. Another valid parent position with identical full target/count but a different repeated ordinal also rejects. Bounds, kind, ordinal, exact count, uint64 entry and each full path component are independently faulted and restored. Zero/one-short work refuses; exact-work guard retry succeeds with all allocations denied. A foreign study with identical numeric source/selectors cannot acquire the owning context. Max entry, ordinal ten, repeated/multibyte paths, ordinary/selected copying and moves remain covered.

Selectors established in an original projection still resolve exact targets after copying/moving to a new owner and destroying the original recipe/projection. Actual public values and independently copied recipe target objects remain complete after all projection/domain owners die. The temporary integer selectors retain no domain, recipe pin or reader object. Existing export allocation-refusal/retry and selected-domain/weak lifetime controls remain active.

## Bounded proposal accepted in #397

Keep logical source, kind, ordinal/count and public owning output separate from definition-owned target payload. A reviewed implementation could retain checked independent occurrence/parent selectors in each private row and use the projection's existing exact recipe for target reads. Preserve full study/entry/path association, source/revision validation, repeated-target ordinals, selector-width admission, both pool roles and move/reorder/copy correlation. Public `parent(index)` must still return a complete independently owning field object; the inactive pool remains private. Native claims/incarnation/generation retain execution authority.

The nested-byte and inline-capacity columns quantify actual existing duplication; the 72-byte shape includes proposed selector overhead. No net retained/peak/runtime saving has been implemented. A separate runtime follow-up must measure actual replacement width, temporary construction, guard work, selected-domain requests/retention/whole-operation peak, both frozen views and detached export costs. It must not charge the old stored-field read and the stronger indexed guard as equivalent duties, or introduce a new global field registry, cache authority, recipe lender, broader revision/admission policy or #324 mathematical change.

## Source-pinned qualification

All four final profiles exit zero with unchanged clean source/tree before and after. CPU/x64; native Clang/LLVM 15 and GCC 11 wrappers, warm/viewport enabled, CUDA/GPU qualification disabled. Matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodules and runtime bitcode `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` remain pinned. Exact configuration/commands, binary/runtime identities and raw observations are retained.

- Quick CI: three CTests, 632 placement arithmetic samples and placement capacity.
- Clang: three projection/structural/native CTests, all four worker/path adoption-cost probes and full thirty-two-round mixed-owner cases on one-worker bulk / two-worker direct.
- GCC 11 native: the same three CTests, two adoption-cost probes and a full two-worker direct mixed-owner case.
- Standalone GCC 11/13: all four CTests each; 12,338 projection and 5,813 membership checks per compiler, plus structural recipe/bootstrap reservation controls.

All 630 before/after record pairs are identical, including twenty-one whole-owner totals, prior parent/header/byte-owner observations, 1,808 occurrence-export and 2,768 parent value-copy allocation refusals. New evidence adds 152 target observations, sixteen selector-case records and 4,000 retained-fact faults across the four final compiler configurations. Three full native runs inspect 192 post-publication view states; thirty-six sampled pool records retain their changing partitions and exact target meaning. Comparison and source/binary identities preserve the distinction between guard, object-comparison and setup duties.

Accepted tested before `22482caa` and reviewed #396 `2fc9d157` differ only in nine Markdown files. Main `5d8c08ce` has zero file differences from reviewed #395 `c68d3751`. Production, fork, kernel, network, build and workflow diffs from the accepted head are empty. Local/canonical tree equality and every source-equivalence proof are retained. The accepted baseline was reused, not rerun merely to establish a baseline.

Verified archive: `/tmp/hcp-projection-parent-target-f551d14f-evidence.tar.gz`, SHA-256 `2b61ef6cda487690e49382d4de64a6c1e9820258eb74f7619a54c40a69f57220`. It retains all final profiles, raw outputs, exact commands/configurations/submodules, clean source-before/after and binary/runtime identities, every comparison pair, new target/selector observations, development output and accepted before evidence. Fresh extraction verifies the root and all profile manifests. The accepted before archive has SHA-256 `2d50df7eea816f43af4a3f27d264c7b7aeaa825c5616ddbe2968ea13e6d5cfcc`; its root/profile manifests are checked too. These machine-local hashes identify contributor evidence without making raw artifacts remotely available.

The standalone development suite passed before committing; it is retained separately from source-pinned qualification. Existing post-PASS Taichi cache-lock warnings remain. No caches, retained stores or Git history were cleared.

The authorized [runtime follow-up](PROJECTION-PARENT-TARGET-INDICES.md) records actual replacement, guard, output and phase-correct preparation costs. That runtime is accepted and integrated in #398. Original inspection attribution and zero saving remain pinned to `f551d14f`. Character/complete-character pins, other field ownership, broader resource/revision/admission policy, #324 and GPU qualification remain separate.
