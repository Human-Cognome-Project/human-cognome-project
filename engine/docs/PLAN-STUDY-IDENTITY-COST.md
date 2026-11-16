# Admission-plan study identity ownership and cost

Contributor comparison under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Public tested source:
`52be505c5830e74a6ffd8dab10894538671b258e`.

This follows the next-comparison direction in
[#354 review 5929111772](https://github.com/Human-Cognome-Project/human-cognome-project/pull/354#pullrequestreview-5929111772).
Main `1337bbcc` integrates #353. Source-accepted #354 is an explicit unmerged
dependency at reconciled `eff64deb977839d8f9d719c93e6645fc99bfe3e0`, which
preserves #353 ancestry/documentation and #354's exact reviewed runtime/tests.
This comparison changes only the existing builder test and Markdown relative
to that dependency. It does not implement the proposed removal below.

## One concrete repeated association

Each `AdmissionOccurrenceCommand` owns an `OccurrenceKey`, including a complete
study UUID. Every row's study equals the UUID already owned in
`StructuralAdmissionPlan::source().study`. The immutable plan's existing source
header is the candidate authority for this one identity component. Numeric
entry/path and full definition pins remain per occurrence; equal study identity
does not merge repeated bodies, roles, units or native claims.

The builder validates owner/study/generation/recipe stamps against the supplied
ActiveAssembly, validates each supplied full occurrence/definition association,
and sorts by numeric entry/path. `copy_occurrence` then allocates a new UUID
string for every emitted row. `empty_source` has already allocated the header's
owned study string in the same selected reservation domain; source assignment
preserves that destination ownership.

Sources: [plan layout and source accessor](../src/assembly/structural_admission_plan.h),
[source validation and copying](../src/assembly/structural_admission_plan.cpp),
[immutable source identity](../src/assembly/letter_projection.h),
[checked owner associations](../src/assembly/prepared_study_owner.cpp).

The [standalone plan contract](STRUCTURAL-ADMISSION-PLAN.md) requires its values
to survive all borrowed source/layout inputs. Borrowing the ActiveAssembly's UUID
would break that contract. Reusing the already owned plan header preserves it:
the header and occurrence table share the plan's lifetime and selected domain.
Moving the unique plan handle leaves both addresses stable. References expire
when the plan dies; an owner association view still requires current full-handle,
generation/tick and authorization checks before loaning plan data.

Native RecordHandles and publication generations identify execution claims;
they do not encode the logical study UUID. Existing native machinery therefore
does not replace this semantic identity. The existing HCP plan header supplies
it without another native field, registry, shared pointer or lookup owner.

## Actual builder witness

The [existing builder test](../tests/structural_admission_plan_test.cpp) now
brackets the actual `StructuralAdmissionPlan::build` call with the established
fixed-table allocation-lifetime observer. Fixture preparation, independent
expected keys/pins and reservation-domain creation occur before the window.
The two fixtures emit two aggregate rows and 24 parts rows for repeated
instances with numeric entries 2/10 and paths that include 2/10.

The witness attributes actual live UUID-string requests through their buffer
pointers. It verifies equal study values, distinct row/header buffers, selected
domain ownership and the enclosing occurrence-vector allocation. It compares
the existing header plus each row's numeric identity/full ToolRef against
independent preparing inputs without allocating. After moving the plan handle
and destroying the caller assembly, bindings and real SNodes, header/row
addresses and complete identities remain valid. Plan destruction releases all
tracked requests and the domain returns to zero live bytes.

Expected values retained by the test are independent copies for assertions;
production identity is read from the surviving plan, not those copies. No
Runtime, native admission or retained execution is created by this test.
Existing work/allocation refusal, numeric order, source lifetime, geometry,
role/unit, comparison and source-pin controls remain exercised.

## Measured candidate cost

Clang 15 and GCC 11 on this Linux x64 ABI observed the same values:

| Actual builder result | Aggregate | Parts |
| --- | --- | --- |
| Emitted rows / occurrence capacity | 2 / 2 | 24 / 24 |
| Build allocation requests | 37 | 151 |
| Cumulative requested bytes | 12,865 | 57,077 |
| Retained requested bytes | 10,993 | 48,741 |
| Scoped peak requested bytes | 12,865 | 57,077 |
| Row-study nested requests | 2 | 24 |
| Row-study nested requested bytes | 74 | 888 |
| Row-study member extent inside vector | 96 | 1,152 |
| Existing header-study nested requested bytes | 37 | 37 |
| Existing header comparison allocations | 0 | 0 |
| Tracked/domain live bytes after plan destruction | 0 / 0 | 0 / 0 |

A UUID object occupies 48 bytes; an occurrence-command row occupies 248 bytes.
The occurrence-vector requests are 496 and 5,952 bytes respectively. UUID member
extent belongs inside those requests; it is not another allocation. The 74/888
bytes are separately allocated character storage. The header's 37 bytes already
exist and must remain. Member extent plus nested contents describe currently
occupied candidate storage, not measured removal savings. Actual row padding,
capacity, temporary allocations, peak and copy-work effects need a subsequent
before/after implementation measurement.

Global new/new[] counters and current-thread lifetime observation cover this
single-threaded logical producer. Selected-domain retained bytes equal tracked
retained bytes in these fixtures; that does not prove complete accounting for
other owners/families. No allocator rounding, arbitrary malloc, RSS, process
peak, native/device allocation, latency or full B claim follows.

## Bounded removal proposal — pending review

Use the existing owned `plan.source().study` as the sole study component for
every row in that immutable plan. Retain the row's numeric `OccurrenceId`, full
ToolRef, command index, node and explicit units. Update all plan producers and
consumers together. No new identity table, registry, owner or general accounting
layer is needed. The source-header UUID must remain owned by the selected domain.

Keep full source and supplied occurrence-key validation before emitting the
numeric-only row; dropping retained duplication must not weaken refusal of
foreign study input. At consumer boundaries, combine borrowed header study and
row entry/path through existing plan/association accessors. Extend the existing
occurrence-text renderer to accept these two const references directly where
needed, avoiding a temporary owning OccurrenceKey just to render an identifier.
Detached reports still own their complete keys/pins and survive native/source
teardown. Existing generic StudyInput/Origin value semantics are outside this
per-plan representation change.

Implementation acceptance must preserve exact full occurrence identity across
separate studies with identical numeric paths, wrong-study/source refusal,
numeric 2/10 order, aggregate/parts/mixed exposure and full definition revisions.
Retain standalone plan/source/domain lifetime, checked owner authorization and
stale-view behavior, complete snapshots/labels, command/native-claim alignment,
source mass versus physical mass and explicit roles/units. Reuse the existing
owner/observer worker/path controls when implementing; native maps and candidate
transaction state remain required.

Recalculate row reservation sizes and legitimate copy-work charges while
preserving validation work before scans/emission and exact/one-short failure
controls. Keep selected-allocation/std::bad_alloc unwind and the existing GCC
construction discipline. Measure the actual changed producer's requests,
retained storage and peak against the source-pinned baseline; do not infer them
solely from sizeof or this candidate attribution. This is one private plan
identity cleanup, not source adoption, a direct indexed guard protocol, a
catalogue/mixed-revision policy or a #324 numerical change.

## Qualification

Personally executed at public `52be505c`, matched CPU-only LLVM 15 fork with
the exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-study-52be505c-quick`:
  PASS. Full native rebuild, actual CPU, three core CTests, 632 independent
  placement interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-study-52be505c-focused.sh`:
  expanded builder CTest passed in 0.08 seconds, all 4,465 checks including
  both actual ownership/cost fixtures and existing refusal/unwind controls.
- `bash /tmp/hcp-study-52be505c-gcc.sh`: a fresh GCC 11 Release wrapper on
  the matched fork passed the same builder CTest in 0.06 seconds. Every
  reported request/retained/peak/member-size value matches Clang.

Every run required the same clean committed source and preserved source-after
checks, commands, configurations, submodule pins, logs and binary hashes.
Per-file manifests verified. Final notes change only five Markdown files after
the tested source. The comparison's production files remain byte-identical to
its accepted #354 dependency; the earlier merge reconciliation likewise kept
#354 production/tests byte-identical while preserving #353 history/documentation.

Raw directories: `/tmp/hcp-study-52be505c-{quick,focused,gcc}`.
Archive `/tmp/hcp-study-52be505c.tar.gz`, excluding GCC build products;
SHA256 `1d92a1e6c5ddfb2d099b65e0fecfb4413ffdb5dc1310f0d4bd69b757fe581845`.
This is contributor-local evidence, not independently inspected remote raw
evidence. GitHub CI is authoritative. Full regression/compiler/database
matrices, owner/observer runtime matrices, browser, GPU and native Windows
were not rerun locally for this test-only comparison. No duplicate hosted
workflow dispatch is requested.

Review this candidate and its removal boundary before implementation. #354
remains source-accepted, unmerged, with separate hosted qualification; this
comparison's local pass neither integrates that dependency nor completes
broader accounting, direct indexed guards, catalogue/producer work or #324.
