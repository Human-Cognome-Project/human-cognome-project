# Membership field objects from complete target pins

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`4f27d26ddf3867ac5382600cc5f97ec0b6bd6331`.
The membership implementation is `dcf75953`; two qualification-driven repairs
follow in separate commits (`ae105cfb` and `4f27d26d`).

Starts from fully integrated main `fd0817f6`. Implements the removal accepted in
[#357 review 5931480140](https://github.com/Human-Cognome-Project/human-cognome-project/pull/357#pullrequestreview-5931480140),
following the [source-pinned comparison](MEMBERSHIP-FIELD-KEY-COST.md).
The earlier #355–#357 dependencies are integrated; this contribution has no
unmerged source dependency.

## One object authority and coordinated readers

`MembershipContribution` now retains complete occurrence identity, an explicit
`GroupKind`, the full target ToolRef and exact count. Its field object is the
object already owned in `target`; the independent FieldKey/ObjectKey copy is
removed. The collector still emits membership kind explicitly. Valid group
addresses may be shorter than physical object addresses; two-, three- and
five-element group keys remain supported.

Legacy and structural binding, ExchangeBindings, ExchangeCoordinator and
StructuralAdmissionPlan use the existing kind/object arguments and borrow
`target.object` directly. No owning FieldKey temporary, registry or new owner is
introduced. Field kind stays distinct from sibling-definition and parent-value
kind even when the object key agrees. Invalid relabelled membership kind refuses
at the bind/validated-plan/binding boundaries.

Existing full target revision checks, including memoized graph visits, missing
or cyclic-source refusal, per-occurrence diamond deduplication, repeated uses,
English/French context isolation, multibyte counts and NULL naming anchors are
preserved. The live set and all-alternative catalogue remain distinct. Full
target pins/occurrence keys survive projection/fixture teardown and the existing
shared-catalogue lifetime. Native handles, candidate/committed state, publication,
force/tick policy and public producer adoption remain under their existing owners.

The old row shape is no longer accepted by the C++ interface. No runtime adapter
accepting a second row ObjectKey remains, so no legacy fallback is added.
Separate field declarations/native bindings still require matching kind/object
associations against the pin; their validation lookups remain in place.

Sources: [row layout](../src/assembly/membership_fields.h),
[collector and bind readers](../src/assembly/membership_fields.cpp),
[exchange binding](../src/assembly/exchange_bindings.cpp),
[exchange staging](../src/assembly/exchange_coordinator.cpp),
[prepared plan](../src/assembly/structural_admission_plan.cpp).

## Charges, census and failure cleanup

Collector construction no longer copies the redundant field object. Growth
prepayment uses the actual smaller row and stops charging relocation of that
removed payload. All traversal, pin resolution, cycle/dedup and context
validation remain. The prepared plan retains the prior full object-association
validation charge explicitly, checks membership kind and resolves the complete
pin against its recipe. Exchange binding adds a charged kind check.

The retained [owner census](../src/assembly/owner_storage.cpp) counts occurrence
identity and the complete target pin once per constructed membership row, plus
actual contribution-vector capacity. Explicit kind is already inside the row.
Independent test census visitors and the graph-address witness now expect one
target address and three UUIDs per row (occurrence study, target scope/revision).
Temporary graph keys and source/header pins keep their separate charges.

The collector constructs occurrence study/path as independently owned values
before nonallocating aggregate moves, following the existing GCC 11 unwind
discipline. Work/allocation exact and one-short controls remain. The value test
also injects std::bad_alloc at every actual public/bounded/catalogue request:
209 + 209 + 245 = 663 failure points, each releasing scratch/output and restoring
zero selected-domain live bytes.

## Actual producer before/after

Verified Clang/GCC binaries from public `ec21b153` execute the actual old
collector. They were matched to retained CI binary manifests, copied before the
first implementation build and rerun with full logs. The new witness brackets
the actual changed collector with the same legacy/structural fixture
construction. Fresh UUID values vary, while canonical widths, paths, groups,
counts and context relationships are unchanged. Fixture/expected values and
domain creation stay outside the allocation window.

The public v1 live and bounded v2 live cases agree. Clang 15 and GCC 11 observe:

| Actual producer | Requests | Cumulative requested bytes | Retained requested bytes | Scoped peak requested bytes |
| --- | --- | --- | --- | --- |
| Live, before (8 rows / capacity 8) | 225 | 16,707 | 4,191 | 14,235 |
| Live, after | 209 | 15,007 | 3,107 | 13,151 |
| Reduction | 16 | 1,700 | 1,084 | 1,084 |
| Catalogue, before (16 rows / capacity 16) | 277 | 23,459 | 8,255 | 18,299 |
| Catalogue, after | 245 | 19,971 | 6,087 | 16,131 |
| Reduction | 32 | 3,488 | 2,168 | 2,168 |

The row changes from 336 to 248 bytes on this Linux x64 ABI, retaining a
four-byte kind and its alignment. Vector requests change from 2,688/5,376 to
1,984/3,968 bytes. Removed 88-byte member extent accounts for 704/1,408 retained
bytes and removed nested buffers for 380/760 more. Those components explain
the observed 1,084/2,168 totals; do not add them to those totals again. Cumulative
requests also include replaced vector capacity during growth. Scoped peak is
measured separately; equal retained/peak reductions here are fixture results.

The target objects keep their original 380/760 nested bytes and full revisions.
Valid pin-object reads add no C++ allocation. Bounded live/catalogue collection
reports 55,736/73,209 preparation-work units after legitimate copy/relocation
changes. Row counts, exposure alternatives and whole contributions are unchanged.

Counters observe current-thread C++ new/new[] request lifetime. They do not
measure arbitrary malloc, allocator rounding, RSS, native/device memory, process
peak or latency. Stack result objects, domain creation and owner shared-pointer/
control-block construction are outside the collector window. Neither these
results nor the retained census complete full B/combined admission accounting.

## Controls and qualification

[Value controls](../tests/membership_fields_test.cpp) pass exact detached
context/pin/count and owned-buffer checks for public v1 live, bounded v2 live
and v2 alternative catalogue output. Original graph/revision/context/exposure,
deep traversal, work/allocation bounds and teardown controls remain. New C++
failure injection covers every reached allocation request.

Dynamic work/allocation-refusal iteration counts follow the actual reduced
copies/relocations and the grouped pin-object validation charge. The original
exact/one-short/refusal/unwind assertions still execute; no guard was removed
to obtain the smaller counts.

[Native membership controls](../tests/membership_fields_native_test.cpp) retain
field-kind collisions, mass/centroid/participation checks and add invalid-kind
refusal for both legacy and structural binding. Structural handoff checks
short group names independently against fixture identities. Prepared owner/
observer controls retain full source/native association and detached snapshots.

Exchange-owner lifetime/census controls use independent actual allocation
observation with the smaller row shape, including domain prepayment, catalogue
copy/move/weak lifetime, all failure/retry paths, stable backing and following
ticks. Native claims and transactional state are preserved through those checks.

## GCC failures found during qualification

The first clean `dcf75953` run passed quick CI, all 14 selected Clang CTests,
four Clang exchange CLI cases and both standalone four-test compiler suites.
Its fresh GCC wrapper passed six selected CTests, then the additional
`exchange_owner` CLI crashed during Fixture installation's native LLVM audit,
before membership-owner controls. A line-buffered repeat and signal backtrace
locate the failure in argument shape-index lookup. A minimal GCC 11
AddressSanitizer reproducer confirms stack-use-after-scope in the conditional
`std::initializer_list` range. Commit `ae105cfb` replaces it with named local
arrays containing exactly the original two argument sets; every audit
predicate remains. Corrected Clang/GCC address-sanitizer probes pass. The
environment cannot run LeakSanitizer under tracing; its failed report remains,
and only leak detection is disabled for the corrected stack-lifetime probe.

That repair lets GCC reach the pre-existing explicit projection-copy sweep,
which then refuses with partial allocations retained. An isolated pure C++
probe of the unchanged projection implementation reproduces 37 retained bytes
when the third 37-byte allocation is refused. Commit `4f27d26d` finishes
allocating keys/pins, recipe rows, projected rows and input strings into
independent owners before nonallocating aggregate moves, including spare
parent construction. It preserves allocation order, selected domains, values,
successful request totals and quotas. No cleanup assertion is relaxed.

The [standalone projection test](../tests/letter_projection_test.cpp) now covers
legacy v1 copy, structural v2 copy and v2 copy plus inactive-parent reservation.
It checks every byte/work refusal boundary, exact allowances and every C++
allocation-failure point: 438 + 400 + 421 = 1,259 requests. All 3,988 projection
controls pass under Clang/GCC. Existing native projection lifetime/census and
failure/retry controls remain. Initial development compile/census failures,
the two failed formal GCC profiles and diagnostic reports are retained with
their source context, separately from final clean-source qualification.

Personally executed at clean public `4f27d26d`, matched CPU-only LLVM 15 fork
with exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-membership-pin-4f27d26d-quick`:
  PASS. Native rebuild, actual CPU, three core CTests, 632 independent placement
  interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-membership-pin-4f27d26d-checks.sh focused`:
  15 CTests passed in 470.81 seconds: both value suites, native membership,
  builder, all four prepared-owner and all four observer profiles, workbench,
  origin provenance and complete snapshot. Builder: 4,696 controls; prepared
  owner: 268/256/256/256; observer: 61 per profile. Four exchange invocations
  cover single/multiple owners at one-worker bulk and two-worker direct paths.
  Basic compiler positive/negative and response lifecycle/refresh compiler
  controls also pass.
- `bash /tmp/hcp-membership-pin-4f27d26d-checks.sh gcc`: fresh GCC 11 Release
  wrapper against the same matched Clang-built fork. Seven CTests passed in
  137.55 seconds, followed by the same four exchange invocations, basic compiler
  controls and response lifecycle/refresh compiler controls.
- `bash /tmp/hcp-membership-pin-4f27d26d-checks.sh clang-values` and `... gcc-values`:
  fresh standalone builds each pass all four projection/membership/structural
  recipe/bootstrap reservation tests without Taichi/PostgreSQL linkage.
  Every actual collector and projection-copy measurement agrees across both
  compilers. The two suites cover 1,922 C++ allocation-failure points in total.

All final profiles retain unchanged head/clean-source checks, commands,
configuration, submodule pins, full logs and binary hashes. The historical
before binaries, original failures and successful intermediate Clang profile
are retained separately. Known post-PASS offline-cache lock warnings remain
in the logs. All per-file manifests verify. Final notes change only six
Markdown files/updates after the tested source.

Raw final evidence: `/tmp/hcp-membership-pin-4f27d26d-{quick,focused,gcc,clang-values,gcc-values}`.
The archive also retains the two earlier qualification sources, before binaries,
development failures and isolated diagnostics, excluding build trees:
`/tmp/hcp-membership-pin-4f27d26d.tar.gz`, SHA256
`108d24557f9127ea6d33977347a604e12f80a7d1950a9863b5e2c9424f9e268a`.
These are contributor-local executions, not independently inspected remote
raw evidence. Full hosted regression/compiler/database matrices, GPU, browser
and native Windows were not run locally for this slice.

Integrated base main `fd0817f6` smoke/native runs 44000323894/44000324524 now
report success; they qualify that base. This contribution's review and hosted
qualification remain separate; no duplicate workflow is dispatched.

Review and authoritative hosted CI remain pending. Direct indexed guards,
catalogue/mixed-revision adoption, general public producer migration, full
accounting and #324 mathematics remain separately scoped.
