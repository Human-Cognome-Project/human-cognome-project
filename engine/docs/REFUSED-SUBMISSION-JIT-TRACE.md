# Real submission refusal, CPU registration and JIT presence

Inspection/disposition under #343, following [#414 review 7103232403](https://github.com/Human-Cognome-Project/human-cognome-project/pull/414#issuecomment-7103232403).
Existing observations suffice to resume a behaviour-preserving responsibility
split. They do not supply complete native resource accounting for an admission
or retirement decision. The smallest proposed next change is the private request
preflight extraction below; no production change is made by this inspection.

## Source and dependency

Tested `3468a26a20ad681b775e9fe2e6b0457ff09626a9`, tree
`4a8dfafb778d3c4ee228ab1e23e15677a469291a`, changes only
`engine/tests/compiler_lifetime_trace.h`. Native fork subtree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648` and all HCP production are exactly
reviewed #414 `e0fa10bb`. Qualification began on that explicitly reviewed
dependency while its native hosted gate was pending. Both hosted workflows then
passed; the maintainer integrated its exact tree at `c5770a2e` and reconciled the
handoff in current main `d78c6d86`. The tested observer is carried onto that main
with executable-source equivalence checked: only seven Markdown files differ
from the tested tree. No baseline rerun is needed for those documentation changes.
All integration remains maintainer-only.

The [#414 implementation](CPU-JIT-ID-ASSOCIATION.md) retains its own qualification,
actual 88-byte row and +64-byte capacity-eight cost. This observer changes no
production storage, admission policy, registry, numerical formula or lifetime.
Later handoff changes are Markdown only.

## One real request through the existing owners

The selected witness is `prepared_submission_inspection`, using its existing
shared v2/f64 fixture, one-worker BulkCopy and two-worker CpuDirect. It accesses
no database or retained private input. The fixture supplies the study, focus,
bindings and budgets explicitly; it does not emulate an implemented Analyst.

| Boundary | Existing responsibility and observed result |
|---|---|
| Request | `PreparedStudySubmission` carries the pinned source, typed bindings, explicit prepared capacity/reservation domain, host path and plan/placement/hierarchy work. The test sets `plan_work=0` for a real plan refusal. |
| Session checks | `ActiveSession::submit_prepared` checks authorization, supported v2/f64/x64 input, duplicate study, source/binding identity, layout/frame and scratch capacity before native construction. Early scratch/duplicate refusals preserve the observed state. |
| Candidate construction | The existing staging map reserves publication storage. Its `Execution` owns the source SNode tree, Harness and prepared owner with the accepted drain/dependent/tree order. Harness setup compiles and prepares actual kernels before the plan refusal. |
| Compiler and CPU | HCP `CompiledKernel` uses the native compilation manager. First registration clones IR into the native JIT, stores an append-only CPU attempt ID and receives its separate native ID. Rows retain record metadata and a confirmed wrapper after native registration succeeds. |
| Refusal | `PreparedStudyOwner::prepare` returns `WorkExhausted`; the session clears staging and returns no handle. Selected reservation live bytes return to zero and candidate roots retire. Compiler cache, launcher rows and live-session dylibs remain owned by Runtime/Program. |
| Retry/publication | The fixture retries the same source with its normal work bounds. After its intervening refusal/constructor controls, the successful request prepares a new candidate, performs one native combined bootstrap and publishes the pre-reserved session map node. No second scheduler or JIT registry is involved. |
| Observation and ticks | Existing viewport observations compare the sibling's exact claims, generation, completed tick and numerical bits. Successful candidate/sibling ticks advance to one/two completed ticks. Observation and dispatch use the existing engine authority. |

Source: [session](../src/assembly/active_session.cpp),
[request/result](../src/assembly/prepared_submission.h),
[prepared owner](../src/assembly/prepared_study_owner.cpp),
[Runtime wrapper](../src/engine/runtime.cpp),
[CPU launcher](../taichi/taichi/runtime/cpu/kernel_launcher.cpp) and
[CPU JIT](../taichi/taichi/runtime/cpu/jit_cpu.cpp).

## Actual retained state and identity

The added observer calls the production attempt/presence APIs at every existing
compiler phase. It matches each row's module to an independently captured factory
return, and its record metadata to the existing compiled-data object observer.
It checks the factory's output native ID against the row, then queries with zero
work and the authoritative bound returned by that live session. Every prepaid
issued query confirms a retained dylib; every short query stays incomplete.
Individual rows are retained in raw logs, without pointer-address identities.

This test scans the rows and prepays each native lookup separately. At 725 rows
and 726 issued IDs it charges 526,350 lookup entries for the phase, in addition
to the row/count observation work. This conservative sum is not a measured number
of native comparisons or a new production batch query.

| Prepared boundary | Live definitions | Live cached data | CPU attempts / rows / confirmed wrappers / present dylibs | Issued native IDs |
|---|---:|---:|---:|---:|
| Warm sibling | 90 | 88 | 86 | 87 |
| First real plan refusal | 90 | 168 | 164 | 165 |
| Repeated plan refusal | 90 | 248 | 242 | 243 |
| Placement refusal | 90 | 328 | 320 | 321 |
| Successful retry published, after existing extra controls | 180 | 749 | 725 | 726 |
| Following candidate/sibling ticks | 180 | 749 | 725 | 726 |
| Before Program finalization, executions gone | 0 | 749 | 725 | 726 |
| After Program finalization, session still live | 0 | 0 | 725 | 726 |

The first refused candidate contributes CPU attempt IDs **86–163**, native IDs
**87–164**, eighty cached objects and seventy-eight confirmed wrappers/present
dylibs. These are successful CPU registrations from a refused *study publication*;
the rows' native `failed` flags are false. Calling them failed JIT creations
would confuse two different boundaries. The initial runtime consumes native ID 0
and is excluded from the CPU kernel row set. The observed `native_id=attempt+1`
is specific to this no-native-failure history, not an identity conversion rule.

All seventy-eight new rows have zero client borrows after refusal; the full row
set has 84 unborrowed records, including six from sibling setup. They remain
registered and retained. Zero borrows alone does not make them eligible for the
existing unregistered-record removal operation or establish recovery credit.

The successful retry is not immediately after the first refusal. The unchanged
fixture also runs repeated plan/placement refusal, three constructor/launch faults
and their corrected plan refusals. Its complete phase/row logs reconcile those
intervening registrations; the final 725 total is not attributed to one retry.
The earlier refused interval remains retained with the same record, native ID
and confirmed wrapper while the new candidate and sibling dispatch successfully.

The legacy control explains the other case: real negative/nonfinite-parameter
refusal adds cached data but keeps twelve rows/dylibs. A successful retry reaches
eighteen registered rows before its first tick and twenty-four after following
ticks. Cache retention and native registration therefore cannot be inferred from
the public failure result alone. [Original lifetime evidence](COMPILER-JIT-LIFETIMES.md)
preserves its source; the new logs add real #414 association queries.

At finalization the row's metadata becomes expired while its confirmed wrapper
and queried dylib remain present through the original live owner. No backend
query occurs after Runtime destruction; the existing release observer checks all
captured objects die at the ordinary owner boundaries. `complete=false` remains
set in every attempt/count/presence result. A present dylib is neither a complete
ORC census nor proof of safe selective unloading.

## Owner decision and smallest justified next change

The actual owner must decide whether it may publish this candidate and preserve
the sibling after refusal. The existing result/status, source/root retirement,
record/row association and following-dispatch observations suffice to inspect
that boundary. Native retention does not imply a poisoned Runtime or permission
to publish a refused candidate. No new owner decision needs a JIT registry.

The remaining unknown that changes a different decision is the **complete live
and peak compiler/ORC resource charge** when admitting another owner into a total
resource envelope. Dylib presence and wrapper counts do not measure transferred
IR, executable allocations, dependencies, overlapping peaks or reclaimable bytes.
Zero selected reservation bytes after refusal does not mean zero retained native
resources. These unknowns still block complete B-based admission or recovery
credit; they do not block a mechanical separation with identical behaviour.

Proposed next PR: extract only the existing **binding/layout/frame/scratch request
preflight** in `ActiveSession::submit_prepared` into one private assembly helper,
called at the same point after owner/capability/duplicate checks and before staging
or native allocation. Keep the current request/result types, check order,
Unsupported/InvalidInput/CapacityExhausted outcomes and scratch demand. Leave
Execution ownership, native construction, prepared-owner/bootstrap, map transfer,
drain and observation where they are. This gives request validation a bounded
responsibility without adding a public Analyst interface or moving JIT policy
into control adapters. It is a proposal for review, not implemented authorization.

Its exit evidence should preserve early refusal's zero native changes, late
refusal/retry/native histories, source/claim/generation identity, combined commit,
sibling bits and following ticks on the existing bulk/direct CPU profiles. Keep
standalone projection free of native/viewport dependencies. A broader extraction,
native cleanup, admission gate or numerical correction would need its own scope.

## Qualification and handoff

Clean-source quick CI and four Clang/four GCC 11 CTests pass against the matching
Clang/LLVM 15 CPU SDK: real prepared and legacy submission at one-worker bulk and
two-worker direct. The original compiler/count/borrow/refusal/dispatch histories
compare exactly against the manifest-verified #414 bundle. The added coverage is
8 real histories, 132 phase samples and 24024 row-to-dylib observations. Minimal warm/viewport-OFF cannot run these owner fixtures; GPU and
concurrent use are not qualified by this inspection.

Archive `/tmp/hcp-submission-jit-3468a26a-evidence.tar.gz`, SHA-256 `0182525b4e37f316f7ce01d6647683abc671a500b11be00e2ab127769e7a182c`,
retains commands, source/tree/submodules, matching builds/configuration/binary
hashes, raw per-row tests, development evidence and the verified #414 comparison
bundle. It is machine-local evidence, not remotely published raw data. The
maintainer did not independently inspect that prior local bundle. New review and
authoritative GitHub CI remain separate; parent #414 passed both gates and is
integrated. No retirement, admission credit, new production registry or
numerical change is delivered. #75/#111/#123 stay held; #324 remains separate.
