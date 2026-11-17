# Admission-plan study identity from the owned source header

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Public tested source:
`576153688eeed29183097e930f3fb7d89e8c7a51`.

Implements the removal accepted in
[#355 review 5929836328](https://github.com/Human-Cognome-Project/human-cognome-project/pull/355#pullrequestreview-5929836328),
following the [source-pinned ownership comparison](PLAN-STUDY-IDENTITY-COST.md).
Started from main `a06362be`; dependency merge `52a10bd0` preserves that
maintainer handoff and source-accepted #355 `93dcd711` / #354 `eff64deb` ancestry.
Those PRs remain explicit unmerged dependencies. The merge's runtime/test files
are identical to #355; implementation is a separate commit.

## Representation and validation

`AdmissionOccurrenceCommand::occurrence` now owns only `warm::OccurrenceId`
(numeric entry/path). The enclosing immutable plan's existing
`source().study` owns the study UUID once in the selected reservation domain.
Full ToolRef, command index, node and explicit units remain per row. The header
and rows survive caller assembly/bindings/SNode teardown and move with the same
unique plan lifetime; no pointer into the caller or another owner is retained.

The builder still validates full owner/study/generation/recipe stamps and every
supplied full occurrence/definition key. Before numeric-only emission it also
explicitly compares the projection occurrence's study with the owned header.
Foreign supplied studies therefore cannot alias an equal numeric entry/path.
Separate plans with equal numeric paths retain different complete identities.

The source-copy helper now copies only numeric path/entry. Occurrence-vector
reservation and row-emission work use the actual smaller row size. Source/header,
binding, sorting, topology and contact validation stay charged. Contact lookup
retains its previous full-key scan charge, including the common study's bytes;
the explicit header equality adds a charged comparison per emitted row. Only
legitimate storage/copy charges shrink. Exact/one-short preparation work and
selected-allocation bounds still qualify the actual new producer.

Sources: [plan interface](../src/assembly/structural_admission_plan.h),
[builder, copies and work charges](../src/assembly/structural_admission_plan.cpp),
[unchanged checked association boundary](../src/assembly/prepared_study_owner.cpp).

## Consumers and detached output

The existing occurrence-text helper is exposed as an overload accepting
`const Uuid&` and `const OccurrenceId&`. The existing owning-key overload delegates
to the same implementation, with its validation and spelling preserved. The
prepared viewport adapter renders header plus numeric row directly, avoiding an
owning temporary OccurrenceKey. Existing projection selection/contact rendering
uses the same helper. This introduces no identity table, registry or new owner.

Checked body rows still align with the same actual full native RecordHandles.
Owner/observer assertions compare header study plus numeric identity against
independently supplied complete source keys. Native context/incarnation/address/
record checks, generation/tick stamps, authorization, roles/units, source mass
versus physical mass and publication order retain their existing behavior.

Detached Origin values still own complete occurrence keys and full provenance
pins; encoded report identifiers remain owned text. Public StudyInput/Origin
value semantics, JSON schema and detached lifetime are preserved. Only the
private sealed plan's repeated study member is removed. No native claim array,
candidate/committed state, execution backing or numerical policy is replaced.

Sources: [rendering interface](../src/assembly/letter_projection.h),
[shared rendering implementation](../src/assembly/letter_projection.cpp),
[prepared observer consumer](../../tools/viewport/native/prepared_owner_state.cpp).

## Actual before/after producer measurements

Both compiler pairs use the same aggregate/parts fixture construction as #355.
Fresh UUID values vary between executions; their canonical widths and the
numeric paths, geometry, masses and structural associations are unchanged.
Before measurements execute the actual source-pinned Clang/GCC binaries from
public `52be505c`, verified against their retained CI binary manifests and copied
before the first implementation build. These are historical producer executions,
not reconstructed row estimates. Their complete logs and binaries remain in
the evidence. Fixture preparation and expected assertion values are outside
each producer's observation window; the changed test measures the actual new
`StructuralAdmissionPlan::build` call.

Clang 15 and GCC 11 observe:

| Fixture / actual producer | Rows / capacity | Requests | Cumulative requested bytes | Retained requested bytes | Scoped peak requested bytes |
| --- | --- | --- | --- | --- | --- |
| Aggregate, before | 2 / 2 | 37 | 12,865 | 10,993 | 12,865 |
| Aggregate, after | 2 / 2 | 35 | 12,695 | 10,823 | 12,695 |
| Reduction | | 2 | 170 | 170 | 170 |
| Parts, before | 24 / 24 | 151 | 57,077 | 48,741 | 57,077 |
| Parts, after | 24 / 24 | 127 | 55,037 | 46,701 | 55,037 |
| Reduction | | 24 | 2,040 | 2,040 | 2,040 |

On this Linux x64 ABI the row shrinks from 248 to 200 bytes. Two/24 removed
48-byte UUID members account for 96/1,152 bytes of vector extent; removed nested
strings account for another 74/888 requested bytes. The existing header string
still requests 37 bytes. Vector requests change from 496/5,952 to 400/4,800 bytes.
These categories explain the observed 170/2,040-byte reductions; vector extent
and nested storage must not be counted again on top of those totals. Build work
now reports 69,916/904,070 units for the fixtures, including the explicit new
header comparison and preserved validation charges.

Every observed allocation and selected-domain byte releases after plan teardown.
Counters cover current-thread C++ new/new[] request lifetime and the logical
build window, not arbitrary malloc, allocator rounding, native/device storage,
RSS, process peak or latency. This does not complete B or combined admission
accounting, and it makes no general saving claim for unrelated owners.

## Controls and qualification

The [expanded builder test](../tests/structural_admission_plan_test.cpp) keeps
numeric 2/10 order, aggregate/parts/mixed exposure, full source/definition pins,
geometry, exact versus physical mass, explicit scale/topology, independent
source/domain lifetime and all original refusal/unwind controls. It adds an
explicit foreign occurrence-study refusal, separate-study equal-path controls,
and exact rendering/allocation parity with the existing complete-key overload.
No temporary owning key is constructed in the split rendering window. It also
injects std::bad_alloc at every actual producer request: 35 aggregate and 127
parts failure points, with no partial plan or live allocation after refusal.

[Owner controls](../tests/prepared_study_owner_test.cpp) and
[observer controls](../../tools/viewport/native/prepared_owner_state_test.cpp)
retain full logical/native association, authorization/stale-view, tick/generation,
complete snapshot/readback and detached-lifetime assertions across worker/path
profiles. The only changes to those assertions resolve study through the header.

Personally executed at public `57615368`, matched CPU-only LLVM 15 fork with
exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-plan-study-57615368-quick`:
  PASS. Full native rebuild, actual CPU, three core CTests, 632 independent
  placement interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-plan-study-57615368-checks.sh focused`:
  all 12 CTests passed in 481.58 seconds. Builder: 4,708 checks including all
  162 C++ allocation failures. All four owner profiles, all four observer
  profiles, workbench, origin provenance and complete snapshot passed. Owner
  totals are 268/256/256/256; fewer dynamic allocation-refusal boundaries
  reflect the removed allocations, while the guard assertions remain intact.
  Observer controls retain 61 checks per profile at all three ticks.
- `bash /tmp/hcp-plan-study-57615368-checks.sh gcc`: fresh GCC 11 Release
  wrapper on the matched fork. Five CTests passed in 165.44 seconds: builder,
  one-worker bulk owner/observer, workbench and snapshot. Builder checks and
  every reported producer allocation/work value match Clang.
- `bash /tmp/hcp-plan-study-57615368-checks.sh standalone`: a fresh standalone
  Clang build passed all four projection/membership/structural-recipe/bootstrap
  reservation CTests in 0.18 seconds without Taichi/PostgreSQL linkage.

Both historical before binaries passed their original 4,465 controls and exact
producer measurements in this slice. Their original source/binary manifests,
configuration, copied binaries, hashes and execution logs are retained. The
current implementation remained clean at the same committed source throughout
all runs, with source-after checks, commands, configuration, submodule pins,
logs and binary hashes. All per-file manifests verified. The final notes add
only five Markdown files/updates after the tested source.

Raw evidence: `/tmp/hcp-plan-study-57615368-{quick,focused,gcc,standalone}` and
`/tmp/hcp-plan-study-authority-before`. Archive
`/tmp/hcp-plan-study-57615368.tar.gz` excludes build trees but includes the
historical before binaries. SHA256
`99203d03dbed336c4dce194dd3f184022436863aea1e66579a55f3d4d228c69e`.
This is contributor-local evidence, not independently inspected remote raw
evidence. Full regression/compiler/database matrices, GPU, browser and native
Windows were not run locally for this slice.

Main `1337bbcc` native run 43972037837 now reports success. The #354 reconciled
and #355 native runs remain separate pending gates at inspection; source
acceptance is not integration. No duplicate hosted workflow is dispatched.

Review and authoritative hosted CI remain pending. Native indexed guards,
mixed-revision/catalogue adoption, public producer migration, complete accounting
and #324 mathematics remain separately scoped. No force, scheduling or activity
law follows from this private metadata change.
