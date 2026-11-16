# Checked prepared associations and consuming observation

**Subsequent implementation — 2026-11-16:** #352 is integrated in main
`5f45f566`. The later [Origin pin-authority removal](ORIGIN-PIN-AUTHORITY.md)
at `417aed19` removes a duplicate key inside detached origin rows and preserves
the checked association/observer contract. Its smaller metadata changes the
retained-versus-consuming comparison to 31 requests/3,287 requested bytes in
both local compiler builds. The 39/3,847 measurements below remain historical
evidence for their exact `33cffcb3` source. The later removal awaits its own
review and hosted qualification.

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Base: integrated main
`677d151f17080eefb4c3ad9e1e5ccb0cdaf7b831`; public tested implementation:
`33cffcb358d6741529c6782f5f88bc9a5b7c0e8f`. Implements the first bounded
consumer slice accepted in [#351 review 5925874274](https://github.com/Human-Cognome-Project/human-cognome-project/pull/351#pullrequestreview-5925874274),
following the [private binding contract](PRIVATE-INSTANCE-BINDING.md).

## Boundary and behavior

`PreparedStudyOwner::associations(owner, handle)` authorizes the current
published owner and returns a small borrowed `AssociationView`. It captures the
owner's native generation and completed tick. `read()` revalidates that stamp,
Harness readiness and every full body/field handle through existing `ready()`
before loaning source, sealed plan, committed handle arrays and pointer-free
reporting scales. Acquisition and reading require exclusive access.

The source/plan remain the existing immutable owned objects. No extra source
snapshot, shared pointer/control block, key registry, current-claim table or
candidate buffer is introduced. Checked body/field ordinals expose references
to the existing typed source row and actual native claim. Body rows also expose
the exact source mass and explicit role/unit association; they never infer scale
from SNode depth or substitute source mass for physical mass.

The stamped view can be retained only while the owner/borrowed execution lives.
Rows/references returned by `read()` are loans for the current exclusive
operation and expire before mutation. To check again after an operation, call
`read()` on the retained stamp; do not reuse a previously obtained row loan.
A quiet completed tick invalidates the tick stamp even when native generation
does not change. A contact-changing tick invalidates the generation stamp.
Reacquisition exposes the same pinned source with current native claims; the
old view cannot relabel itself as new. Caller source temporaries can die while
the owner retains its existing pin. The view does not extend owner/Runtime
lifetime and must end before their teardown.

The prepared viewport adapter is the one new consumer. It reads checked
associations and constructs its detached report metadata from them, preserving
authorization, exact pins, numeric occurrence paths, typed fields, role/units
and full native handles. The adapter still delegates physical readback and
optional combined intent to the existing native completed observer. Current
FixedStudy/letter capture is unchanged.

## Removed observation copy

Previously, the adapter built a detached ActiveReport, moved it into a temporary
HarnessBinding, then the observer deep-copied that mapping into CompletedState.
The temporary binding was immediately destroyed. That last full metadata copy
and the copy of its body-handle vector were unnecessary for this consumer.

The observer now has a consuming HarnessBinding overload. One private template
shares all readiness, generation, coverage, full-handle, comparison, intent and
final completed-boundary validation between retained and consuming overloads.
The retained const-reference API still copies its metadata. The consuming path
moves the mapping after initial validation and moves body records after their
last use. Native values are read/copied identically; it does not move authoritative
Harness storage or change execution policy. On failure, no completed report is
returned; native observation remains read-only.

The checked view itself adds validation rather than eliminating an old source
copy: previous owner accessors already returned references. The measured saving
below comes specifically from consuming the temporary detached observer metadata.
Required final report copies/formatting and native readbacks remain.

Sources: [owner interface](../src/assembly/prepared_study_owner.h),
[checked view](../src/assembly/prepared_study_owner.cpp),
[prepared consumer](../../tools/viewport/native/prepared_owner_state.cpp),
[shared completed observer](../../tools/viewport/native/completed_state.cpp).

## Equivalence and refusal controls

Existing owner tests now exercise both aggregate and parts sources with two
instances, workers 1/2 and bulk/direct paths. They check:

- Pointer identity with existing source/plan/claim arrays and unchanged source
  shared-owner count; view acquisition, `read` and valid ordinal access allocate
  no C++ storage.
- Exact body definition/occurrence and typed field associations against the
  independently supplied admission bindings, plus exact source mass and explicit
  scale parent/role/units. Out-of-range body/field ordinals refuse.
- Unpublished, failed-publication, unauthorized and foreign logical handles
  remain unavailable. Test-only committed-header fault injection changes each
  context/incarnation/address/record component for bodies and fields; `read`
  refuses before returning rows. Headers are restored before further native work.
  This is a guard fault control, not a new native mutation route or direct indexed
  release/reuse qualification.
- Healthy operation refusal, scratch reuse and bootstrap no-op preserve a
  current view. Quiet and contact-changing ticks reject old stamps; new stamps
  borrow the original source after caller pin release. All views/loans end
  before owner teardown.

Observer tests compare retained and consuming bindings at ticks 0/1/2. They use
real completed metadata, clear numerical staging vectors before binding, and
read physical state again through the native observer. They compare complete
snapshot JSON exactly, exact detached occurrence/ToolRef pins, numerical copy
bytes and requested readback bytes. Both overloads reject the same stale
generation. Existing optional-intent/refusal, tick/frame, handle and legacy
snapshot controls remain exercised.

Tests: [owner](../tests/prepared_study_owner_test.cpp),
[observer and transfer measurements](../../tools/viewport/native/prepared_owner_state_test.cpp).

## Measurements and qualification

On this Linux x64 ABI, the stamped view is 24 bytes and its five-reference row
loan is 40 bytes. Both compilers observed zero C++ allocations for acquisition,
read and valid row access, and no increase in the existing source shared-owner
count. These are stack/borrowed objects, not additional retained heap storage or
a new control block. Owner setup/claim reservation shape remains unchanged.

For the two-instance parts fixture with four origin rows, the compiler runs
measure the following completed-observer transfer (optional intent requested):

| Wrapper compiler / path | C++ allocation calls | C++ requested bytes |
| --- | --- | --- |
| Clang 15 retained const binding | 158 | 9,197 |
| Clang 15 consumed temporary binding | 119 | 5,350 |
| GCC 11 retained const binding | 164 | 9,677 |
| GCC 11 consumed temporary binding | 125 | 5,830 |
| Avoided requests in either comparison | 39 | 3,847 |

Totals are compiler/build specific; savings compare retained and consuming
paths within the same build, not across toolchains.

The counters cover new/new[] requests across the measured observer call after
fixture preparation. They exclude the fixture's premeasurement binding copy,
arbitrary malloc, native allocation/reservation, physical residency and process
peak. This is a bounded fixture result, not a general latency or total-memory
claim. Snapshot values and requested native readback/numerical copy bytes are
equal. Native arrays, compiled kernels and all persistent owner maps remain
owned exactly as before.

Personally executed at public `33cffcb3`, matching CPU-only LLVM/Clang 15 fork
tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53` and exact recursive submodules:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-association-33cffcb3-quick`:
  PASS. Full native rebuild, actual CPU, three CTests, 632 independent placement
  interval samples and placement_capacity passed.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-association-33cffcb3-matrix.sh`:
  11 CTests passed in 457.10 seconds. Includes all four owner and all four
  observer worker/path profiles, builder (4,455 controls), existing workbench
  (165) and legacy snapshot (64). Owner controls: 272 one-worker bulk, 260 in
  each other profile. Observer: 61 per profile, all three ticks, identical
  snapshot/readback assertions and consistent 39-request/3,847-byte savings.
- `bash /tmp/hcp-association-33cffcb3-gcc.sh`: fresh GCC 11 Release wrapper on
  the matched LLVM 15 fork. Four CTests passed in 133.12 seconds: builder,
  one-worker bulk owner/observer and legacy snapshot. The same view/loan sizes,
  zero-allocation checks, source-owner count and transfer savings passed.

All runs required a clean committed source, preserved their source-after checks,
commands/configuration, logs and binary hashes. Per-file manifests verified.
The subsequent contribution notes are Markdown-only; implementation/test files
remain identical to the tested source. Raw evidence is retained under the three
`/tmp/hcp-association-33cffcb3-{quick,matrix,gcc}` directories. Archive
`/tmp/hcp-association-33cffcb3.tar.gz` (GCC build products excluded), SHA256
`165b998e1c24d60987e1b9fdc52e8874e44bc05b71f2cd49de982044baa29fba`.

Machine-local evidence remains contributor execution, not remote raw evidence
or maintainer acceptance. GitHub CI is authoritative. Full regression/compiler/
database matrices, browser checks, GPU and native Windows were not run locally
for this slice. No numerical kernel or fork source changed; native backing/
compilation checks in the owner controls remain applicable. No duplicate hosted
run is manually requested.

## Remaining work

Actual claim arrays, sealed plan, owner source pin, candidate/committed maps,
ExchangeOwner behavior, native publication and owner setup budgets are unchanged.
The view owns no retained allocation; its stack objects are not additional
shared pointees. No full B/C/H/D accounting, heap residency or process peak is
qualified. Report copying stays outside native commit/publication.

The next comparison can use this checked immutable boundary to measure specific
remaining duplicate associations before proposing removal. Preserve candidate
transaction state until its separate lifetime is proved unnecessary. Direct
indexed execution still needs authoritative native lifetime/publication guards;
mixed-revision provenance, heterogeneous layouts, producer migration and #324
mathematics remain separate. This change supplies no new force law, raw-coordinate
handle replacement or general live-edit operation.
