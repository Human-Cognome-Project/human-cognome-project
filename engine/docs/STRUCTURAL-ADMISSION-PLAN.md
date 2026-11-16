# Private structural admission plan builder

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Governing proposal:
[prepared-admission seam](PREPARED-ADMISSION-SEAM.md), source-accepted in
[#347 review 5920157778](https://github.com/Human-Cognome-Project/human-cognome-project/pull/347#pullrequestreview-5920157778).
Starts from main `6641e032b6017bb5b402b480f91f1e8a895892c6`, with #347 at
`13d08f37ffa6ca1be0cdba46e5aa84a5831c2992` as an explicit merge dependency.
#347's native run 43864232274 has passed; its source acceptance is not a claim
that it is integrated. Tested implementation:
`7bb1e670dd006cf7d7feb17c2c03382273e2619c`; Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53` with exact initialized submodules.

## Implemented boundary

[`detail::StructuralAdmissionPlan::build`](../src/assembly/structural_admission_plan.cpp)
borrows one immutable `ActiveAssembly` and trusted preparing-owner
[`AdmissionBindings`](../src/assembly/structural_admission_plan.h). A narrow friend
declaration permits direct inspection of the snapshot's pinned projection,
current live memberships and focus override. This avoids an intermediate
unbounded copy and independently assembled source/focus catalogues. The helper
does not change the public assembly/session interface or call native admission.

The preparing owner supplies matching owner/study/logical-generation/recipe
stamps, real SNode layout with explicit role/units, occurrence-to-definition and
native kind/address associations, physical mass, opaque complete geometry
identity/revision, explicit comparison identities/tree/precision and ordered
focus. These are trusted setup inputs, not a new external request API. The
builder validates their associations with the same pinned prospective source;
it cannot certify an installed native kind/geometry registry or execution backing.

| Input responsibility | Validation and emitted values |
| --- | --- |
| Selected occurrences and exact source | Only the snapshot's active aggregate/parts exposure becomes bodies. Numeric source entry/path order determines particle commands and placement ranks, independently of descriptor order or lexical text. Distinct repeated occurrences remain distinct. Original uint64 mass is `exact_mass`; ordered parent counts/ordinals and deduplicated per-occurrence memberships become exact participation. Shares are derived from those counts. Caller-supplied physical floating mass remains separate. |
| Layout and complete intrinsic geometry | Actual root/child SNode relationships and pinned recipe roles are checked. Explicit units must agree with PrimaryStep ×64 and SecondaryCompression/NamingEquivalent ×1, without reading depth as scale. Original double geometry is validated through the shared `intrinsic_enclosing_radius` helper, then converted by the explicit operative units. No f32 PreparedStudy values, world starts or velocities enter this path. Geometry identity/revision must be nonzero, complete and consistent across repeated pinned definitions. The initial subset supports the zero root frame; nonzero rebase/restart input is unsupported. |
| Fields, comparison and focus | Native declarations use unique valid addresses per record family and valid kind codes. Every emitted field must have positive source and one unique comparison owner in the explicitly supplied complete tree. Children precede parents, levels/precision are supported, exact child coverage sums match the parent for each occurrence, and every body participates in a leaf. Cohorts name this builder's ordered participation commands. No comparison topology is inferred from scale or membership edges. Requested focus must match the snapshot's inherited recipe or explicit override in order; an empty override never restores recipe focus. |
| Source contacts | Every explicit pinned contact is retained using symbolic particle references and final source-ledger positions. Inactive/missing/self/duplicate endpoints reject. This source list does not replace the later native prospective contact union or geometric certification. |

Successful output owns existing `UpdateCommand`, `PlacementIntrinsic`, focus
`UpdateRef`, `LiveRefreshNode`/child/contribution tables and typed
occurrence/definition/field-to-command indexes. Pending references contain no
guessed native handles or physical record slots. Full source stamps and nested
identity payloads attach to the supplied reservation domain. The immutable plan
owns no SNode pointer, source reference, Program, Harness, physical state or warm
lease. `hierarchy()` borrows stable tables for the plan's lifetime; moving its
unique handle does not move those tables. The future execution owner must retain
its own source pins, native bindings and exclusive lifetime boundary.

## Refusal, resources and native reuse

The builder uses existing `PreparationWork`, `ReservationDomain`, reserved
containers and `ReservedUnique`. Work is charged before sized scans, key
comparisons/copies, reservations and emission; checked additions/index conversion
and the existing checked element-work helper guard arithmetic. The supplied
domain must already exist; this helper neither invents a new allocator nor
claims to account for the caller or domain's outer construction.

Logical particle/field/participation/contact/focus/address, hierarchy
node/link/reference/coverage/level and whole `BootstrapCommands` limits are
checked. Whole bootstrap may exceed ordinary Batch. A failed build releases
partial output and scratch, returns typed work/capacity/allocation/numerical or
unsupported-input refusal, and exposes no usable plan. Retry uses the unchanged
snapshot and appropriate admitted resources. The plan does not replace native
preflight of generation, bindings, numerical ranges, placement/cell/contact/work
peaks, response/reference demand, or actual backing. There is no admission-ready
token or complete C+B+H+D accounting claim.

The enclosure formula/validation moves unchanged into
[`intrinsic_geometry.h`](../src/assembly/intrinsic_geometry.h) and is shared by
legacy `study.cpp` and the builder. Native record claiming, generated placement,
hierarchy repair, commit and tick code are unchanged. The new library/test target
is native; standalone projection targets acquire no Taichi/PostgreSQL dependency.

## Qualification and retained failure

Personally executed at public `7bb1e670`, Linux, matched LLVM/Clang 15 CPU fork:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-structural-plan-quick-7bb1e670`:
  PASS. Rebuilt all native targets; actual CPU, one worker, bulk path. Three
  CTests, 632 independent placement-math samples and placement_capacity passed.
- `/tmp/hcp-structural-plan-native-7bb1e670.sh`: eight focused CTests passed,
  31.73 seconds. Includes the new builder, existing active workbench/projection/
  membership/native projection/native membership/structural recipe and viewport
  observer controls. Existing owner/observer suites retain 165/64 checks.
- `/tmp/hcp-structural-plan-focused-7bb1e670.sh`: fresh GCC 11 native-wrapper
  build against the same matched fork passed the builder test; a separate GCC 11
  engine-independent build passed all four standalone projection tests.
- Both compiler executions of the builder passed 4,454 checks: aggregate/parts/
  mixed exposure, numeric entry/path order, repeated exact parent provenance,
  membership diamond deduplication, original double geometry, explicit scale/
  comparison/focus, incompatible/fractional coverage refusal, every reached
  preparation-work and selected-allocation refusal boundary, exact/one-short
  logical limits, source death and final domain release. No native claims or
  retained execution are exercised by this metadata test.

Original public `852b83cfba99f78b377fdfba2188ccd880c458f1` passed quick CI but
**failed** GCC 11's allocation-refusal cleanup control (`every request refusal
fully unwinds`). This result remains failed. The correction constructs each
source-stamp owner independently before nonallocating moves form the aggregate,
avoiding the GCC 11 nested-aggregate cleanup failure already encountered in other
bounded setup code. The same refusal controls pass at the corrected revision;
the original logs and binary are preserved. No quota/control was weakened.

Raw evidence directories are `/tmp/hcp-structural-plan-{quick,focused}-852b83cf`,
`/tmp/hcp-structural-plan-{quick,focused,native,record}-7bb1e670`; exact scripts,
commands, compiler/configuration, source/submodule/binary hashes and logs are
retained. Archive `/tmp/hcp-structural-plan-7bb1e670.tar.gz`, SHA256
`6edbcecda603c7fd9f7ace38ed3859f1d6cc7453e3aeaf6017ca6a7681f68e09`.
The archive excludes build trees and includes the original failed GCC binary.
Post-PASS Taichi cache-lock warnings remain in quick logs. This is local
contributor evidence, not independently inspected execution or remote artifact
availability. Full regression/compiler matrix, database and GPU execution were
not run locally. GitHub CI remains authoritative; its disposition is in the PR.

## Next owner slice

After review, connect this private metadata preparation to the existing empty
Harness and combined bootstrap under one exclusive execution owner. Reserve all
owner/projection/map publication space before claims; consume actual `resolved`
handles and publish maps only after native commit. Do not bootstrap then rely on
fallible `ExchangeOwner::adopt` as healthy publication. Prove preclaim unchanged
state/retry, postclaim unready behavior, nonallocating publication, native
tick/completed observation and Runtime/Harness/borrower teardown order. Retained
C3 capture remains unavailable until that separate integration is implemented
and qualified. No Analyst decision API, new allocator/placement solver/scheduler,
complete accounting envelope or #324 formula is introduced here.
