# B3 generated-row staging evidence

Status: contributor-local CPU evidence for a private implementation prerequisite,
not acceptance of complete admission. Tested commit: `68a106af` on accepted main
`d7e2f447`. Later changes in this increment document scope and evidence only.
See [implementation scope](../../docs/NATIVE-ADMISSION.md).

On 2026-11-03, Ubuntu 24.04/WSL2, Clang/LLVM 15, native CPU/x64:

- `live_hierarchy_test --threads 1 --case placement_generated`: passed.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement_generated`: passed.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement_compiler`: passed at 52 and 8 body fraction bits. Five emitted modules per profile were audited, including the initializer; seven unsafe controls were rejected per profile. Raw LLVM/ABI outputs retained.
- `live_hierarchy_test --threads 1 --case placement`: all focused placement cases passed, including existing geometry, boundaries, ownership, rejection, compiler and capacity cases plus generated-row staging.

Generated-row checks compare native scratch readback to the independently
observable certified positions, verify canonical-to-command remapping, zero
non-intrinsic state, clearing after shorter repeated batches, no recurring C++
allocation/native backing growth or compilation, preserved survivors/history and
unchanged earlier public observations on successful and rejected stages.
The emitted initializer writes only its frozen typed scratch binding; native
presence/activity writes are rejected by the audit.

Raw logs, commands, source/submodule pins, CMake configuration, toolchain, binary
and runtime hashes, and emitted modules are retained locally as
`admission-68a106af`. Its `SHA256SUMS` digest is
`adbabccc58d25208eb6402a591c9562b5355d7365a880c19d09c71c2256e10af`.
The bundle is local, not made remotely accessible by this hash. Shared-host
timings are not performance qualification.

Full CPU regression, all compiler/worker modes, PostgreSQL and GPU profiles were
not run for this increment. No first-admission D1 focus, native record claim,
contact publication or operational Add/Remove behavior is claimed.

## Hosted acceptance — 2026-11-03

PR #205 at `a8a14fdee36ca6b70b08d4c9f1d274c4b16493ac` was source-reviewed
without blockers and passed all eight hosted checks. The independently inspected
[native job 125871626394](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41622544766/job/125871626394)
passed 115/115 CPU tests in 4319.55 seconds and 180 integration checks, with
24 addressed reads and zero private reads. Both evidence artifacts uploaded.
This hosted result is separate from the contributor-local evidence above.

Merged as `ef0ede690900e6b3cea4bdfce7fc0e33b7a6021b` after CI-only #206.
Acceptance covers the private staging prerequisite only. D1 bootstrap/focus,
prospective contact validation, record claims and coordinated single-publication
admission remain the next B3 work. Integrated main [run 41720357523](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41720357523) passed at the merge pin above: all 115 tests exactly once, all eight compiler configurations and extra no-unroll cases, plus 180 integration checks with zero private reads. [CI acceptance details](../../docs/HOSTED-CI.md#integrated-main-verification) retain job/artifact pins and measured timings.

## Prospective source/hierarchy preflight — 2026-11-03

Tested commit `162dcae43865b5ea3a0daa85bb3e54c8fe09c3a0`, stacked on
provisionally accepted #205 (`a8a14fde`). Ubuntu 24.04/WSL2, Clang/LLVM 15,
native CPU/x64; no GPU execution. Later commit documents evidence only.

- `live_hierarchy_test --threads 1 --case admission_preflight`: passed.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case admission_preflight`: passed.
- `prepared_update_test`: passed, including ordinary rejection/no-op/capacity,
  structural wake/history and late-failure cases.
- `live_hierarchy_test --threads 1 --case refresh_dirty`: passed.

Candidate mass/centroid checks prove the hierarchy consumes native generated
positions and mass rather than fixture host rows. Repeated attempts preserve
backing, survivor velocity, pending work, installed state and the earlier public
placement observation. CascadeWork exhaustion, incomplete hierarchy (the existing
Unsupported result), and injected late preclaim rejection retain healthy retry.
Public fixture refresh still rejects missing fixture starts after private staging.
Successful preflight advances neither generation nor ticks and claims no records.

Raw logs, commands, configuration, source/submodule pins and binary/runtime hashes
are retained locally as `admission-preflight-162dcae4`. Its `SHA256SUMS` digest is
`33fb647fc4a90c3d28cc072241cab98a7842fcef6e76b9bc869b75528f944ffd`.
No full CPU regression, full compiler matrix, PostgreSQL or GPU rerun is claimed.
Prospective contact validation, complete bootstrap and atomic publication remain
unimplemented; this evidence does not establish operational admission readiness.

## Preflight hosted review — 2026-11-04

PR #207 at `bf12964641ba5cd6f8fc2b49156e948827857e43` passed all eight
hosted checks. Independently inspected [native job 126197667121](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41716471199/job/126197667121)
passed 115 CPU suites in 5822.67 seconds and 180 integration checks, with
24 addressed reads and zero private reads. Source review found no blockers
within private source/hierarchy preflight scope. Contributor-local evidence
above remains separate. Integration head `ecd1d6aed6007d6859c4e451d233feccf1627610` passed all 18
hosted checks in [run 41827027659](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41827027659).
Independently inspected shard jobs 126566575256, 126566575205,
126566575355 and 126566575272 cover all 115 test indexes exactly once
(29/29/29/28); database job 126566575257 passed 180 integration checks,
24 addressed reads and zero private reads. All compiler jobs and the aggregate
passed. Merged as `c51cab73ef21752bfa9d849a08b941846881aaeb`.
Complete B3 admission and GPU qualification remain outstanding.

## Prospective contact preflight — 2026-11-04

Contributor-local qualification at `e2720b511b4f266ef47d553d22c4f06e0157a9d3`,
stacked on provisionally accepted #207 integration `ecd1d6ae`. Later changes
document scope/evidence only. Ubuntu 24.04/WSL2, Clang/LLVM 15, CPU/x64:

- `live_hierarchy_test --threads 1 --case placement`: all nine focused cases
  passed, including generated starts, source/hierarchy/contact preflight,
  placement geometry/rejection/capacity/ownership and compiler checks.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case admission_preflight`: passed.
- The same two-worker/direct/no-unroll configuration with `--case admission_contacts`: passed.
- The same configuration with `--case placement_compiler`: passed at 52 and 8
  body fraction bits. Ten emitted kernels were audited per profile, with twelve
  unsafe controls rejected per profile; LLVM and ABI outputs retained.
- `prepared_update_test`: passed.
- `live_hierarchy_test --threads 1 --case refresh_dirty`: passed.

The prospective union includes generated endpoints, preserves the first explicit
orientation, maps duplicate/reversed sources to one pair, and retains tangency
semantics. Sleeping survivors participate in source-empty derived discovery.
Tests reject three required contacts against capacity two and 43 simultaneous
source/tree/contact references against capacity 37. Closed retirement removes
exactly the outgoing occupant and permits healthy retry. Preparation and final
union-order work limits reject unchanged. Repeated preflight retains native
backing/compiler state, observes no recurring C++ allocation, and preserves
survivor/history/pending state and prior public placement observations.

Raw logs, exact commands, source/submodule pins, configuration, toolchain,
binary/runtime hashes and emitted modules are retained locally as
`admission-contacts-e2720b51`, with a Windows workspace archive of the same name.
The `SHA256SUMS` digest is
`a5007f96eab199ecb4e42d58584cd731ed61335aa6e5148380a21ad93eff5ab2`.
This hash identifies a machine-local bundle, not remotely available evidence.

No full CPU regression, remaining compiler matrix, PostgreSQL or GPU run is
claimed. Shared-host timings are not performance qualification. This remains a
private preclaim prerequisite: complete D1 bootstrap/focus and coherent
source/hierarchy/contact/activity publication are not implemented. Hosted CI
and maintainer acceptance remain separate from these contributor-local results.

## Ordinary combined publication — 2026-11-04

Contributor-local CPU/x64 qualification at
`edee7e2ff4a097a797cd5d515d06fc8f229539c6`, stacked on provisionally accepted
#208 with main's #207 acceptance notes integrated. Later changes document scope
and evidence only. Environment: Ubuntu 24.04/WSL2, Clang/LLVM 15.

- `live_hierarchy_test --threads 1 --case placement`: all twelve focused cases passed.
- Two-worker/direct/no-unroll runs passed for `admission_commit`,
  `admission_contacts`, `admission_preflight`, `admission_contact_remap` and
  `admission_commit_failures`.
- `placement_compiler` passed in one-worker/bulk/default and
  two-worker/direct/no-unroll configurations at 52 and 8 body fraction bits.
  Each profile audited twelve kernels and rejected fifteen unsafe controls,
  including omitted contact outputs and a missing publication enable guard.
  LLVM modules and ABI descriptions are retained for both configurations.
- `prepared_update_test` and one-worker `refresh_dirty` regression passed.

Ordinary admission runs twice between ticks at both 70- and 128-row capacities.
It preserves the original survivor's complete numerical payload, initializes
new bodies from native certificates with zero motion, resolves source/contact
endpoints to actual claims, installs the contact view, and advances exactly one
generation without a tick. The following tick observes no further coverage
change. Preclaim rejection retains the entire committed snapshot and permits
retry. Backing/compiler metrics stay fixed and successful prepared operations
observe no recurring C++ allocation.

Capacity cases reject three contacts against two and 49 simultaneous references
against 40 while retaining an old installed union. Closed retirement permits
combined publication and a following tick. Synthetic nonidentity endpoint maps
verify explicit orientation, duplicate mappings, both adjacency directions and
canonical derived-only ordering without changing committed state. Failures after
native claims and after contact publication make the execution unready without
advertising a completed generation or tick.

The first ownership audit at `8941b1f3` rejected the second prepared owner's
publication kernel. Native CHI kernel caching uses names rather than IR hashes:
generic names could reuse another owner's SNode identities or fixed capacities.
The publication owner suffix fixed that audit at `ade8602d`; its remaining run
was deliberately interrupted to isolate all capacity-dependent helpers and the
canonical-order variant as well. Final qualification above passes on the
corrected source. Earlier logs are retained separately from passing evidence.

Bundle `admission-commit-edee7e2f` contains raw logs, commands, source/submodule
pins, configuration, toolchain, binary/runtime hashes and emitted modules. Its
`SHA256SUMS` digest is
`d5169003237d9479b55b56c8a62ceb22fc06bec4d1724eabcf3697ca142bf7b2`.
A matching archive is retained in the Windows workspace. This is machine-local
evidence, not a remotely accessible artifact or independent hosted acceptance.

Full CPU regression, remaining compiler modes, PostgreSQL and GPU were not run.
Shared-host timings are not performance qualification. Complete D1 bootstrap,
prospective initial focus and the operational owner/API remain outstanding;
this contribution publishes ordinary arrivals into an already installed model.

## Private D1 bootstrap and initial focus — 2026-11-04

The continuation of provisionally accepted #209 implements first admission into
an empty, never-populated prepared owner. Source, rooted hierarchy, generated
body starts, contact view and explicit initial focus publish as one unticked
generation. The boundary remains private; representation exchange, an enduring
operational owner/public Add API and GPU qualification are separate work.

Local CPU source pin: `d31a0f1d3e579ff57ae6f0ffa5900268d1149f9f`.

All listed qualification runs passed. Bundle `admission-bootstrap-d31a0f1d`
contains commands, source/submodule pins, configuration, toolchain, binary/runtime
hashes, raw logs and emitted modules. Its `SHA256SUMS` digest is
`d10bc3b797ab681811fd5eb716f4a5f251c0436b024e565781dadd66e58fe75b`.
A matching archive is retained in the Windows workspace. The raw bundle is
machine-local; its digest does not make it remotely accessible or independently
verified. The subsequent documentation commit does not change tested code.

The focused qualification covers the complete thirteen-case placement/admission
suite in one-worker bulk/default mode; two-worker direct/no-unroll bootstrap and
ordinary admission; compiler checks in both configurations at 52 and 8 body
fraction bits; `prepared_update_test`; and `refresh_dirty`. Compiler checks audit
thirteen kernels per precision profile, reject seventeen unsafe variants and
accept legal repeated writes. Raw emitted LLVM and frozen ABI descriptions are
retained. This is contributor-local evidence, not hosted acceptance.

Bootstrap cases exercise three new bodies and three new fields in twelve source
commands against ordinary Batch=2 and BootstrapCommands=16, at 70 and 128 physical
capacities. The first selected field's canonical participant establishes zero,
including when that occurrence comes last in ordinary canonical order. Independent
test-only certificate checks use the changing partial focal centroid. A sole
selected field leaves the target unchanged when unselected bodies arrive;
reversing selection order changes the anchor. Missing, duplicate, wrong-kind,
absent-participant and over-capacity selections reject before claims. Separate
bootstrap command/work limits and incomplete hierarchy reject with healthy retry.

Preflight leaves the empty owner unchanged. Successful admission resolves native
field references, retains fixed backing, observes no recurring C++ allocation,
initializes zero motion/force, advances one generation without a tick, and installs
a contact/hierarchy view consumed by the following tick without a coverage change.
Injected failures after native claims and after contact publication leave execution
unready without publishing a completed generation. A populated owner cannot be
bootstrapped again.

The initial full-precision three-radius-one fixture at `a1e7c784` exhausted its
65,536-cell search reservation before hierarchy validation; that rejected attempt
is not passing evidence. Full-precision coverage now uses an exact-boundary
zero-extent final occurrence, while the 8-bit profile retains three positive
radii. A subsequent fixture at `30cd1b90` correctly failed hierarchy validation
because its two leaves lacked a common root. The final fixture supplies that root
and complete source coverage. Both diagnostic logs are retained separately.

Full CPU regression, other compiler modes, PostgreSQL and GPU were not rerun.
Shared-host timings are not performance qualification. The implementation and
source-pinned evidence support provisional review only; hosted CI and maintainer
acceptance remain independent.

## Integrated hosted acceptance — 2026-11-04

PR #210 at `e359b6e31eb38a3f69512c7ac69f25eb7186c36f` passed all 18 hosted
checks and merged as `3f042d706e73e42f5e312a1362a73043ef504b4e`. It includes
#208's contact prerequisite and #209's ordinary commit plus the kernel-identity
correction; GitHub marked both antecedent PRs merged through this integration.
This does not independently accept the uncorrected #208 source.

Inspected [native run 41851862998](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41851862998):

| Shard | Job | Tests | CTest seconds |
|---|---|---:|---:|
| 1 | 126653824877 | 29 | 1448.04 |
| 2 | 126653824803 | 29 | 1622.28 |
| 3 | 126653824990 | 29 | 1669.57 |
| 4 | 126653825042 | 28 | 1714.07 |

Logs cover every index 1–115 exactly once. Database job 126653824761 passed
180 integration checks with 24 addressed reads and zero private reads.
Downloaded compiler artifacts 13337786195, 13337875523, 13337990147 and
13337621864 cover all eight worker/source/compiler configurations: each
placement audit reports 13 kernels and 17 rejected unsafe controls at both
fraction-bit profiles. All four extra no-unroll placement outputs include
passing bootstrap witnesses and zero case failures.

Source review found no blockers within private ordinary arrival and initial
D1 bootstrap. Contributor-local bundles above were not independently inspected;
the hosted evidence here is separate. This does not qualify representation
exchange, focus replacement on an existing model, operational owner/public
Add/browser controls or GPU execution.

## Private existing-model focus selection — 2026-11-04

The continuation after accepted #210 implements the selection portion of D6:
an ordered set of current participating fields replaces the focus in the same
installed model. Native calculation and guarded publication update the complete
focal tuple and applicable wake bits, followed by one unticked generation.
Coordinate rebasing, representation exchange and operational public controls
remain separate work.

Local CPU code/test source: `28f96082c252b847db6464b67677a4af699cf7ef`.
The following documentation commit changes no tested code.

All qualification runs listed below passed. Bundle `live-focus-28f96082` retains
raw logs, commands, source/submodule pins, configuration, toolchain, binary/runtime
hashes and emitted modules. Its `SHA256SUMS` digest is
`ff9f10222ba34161b2ef5f194fdcaa12f3a8e4cca7a1bcddac2c69ef4f192215`.
A matching archive is retained in the Windows workspace. This is machine-local
evidence; its digest does not establish remote availability or independent review.

Qualification covers the fourteen-case placement/admission suite with one worker
in bulk/default mode, two-worker direct/no-unroll live focus and compiler checks,
`prepared_update_test` and `refresh_dirty`. Both compiler configurations inspect
14 kernels and reject 19 unsafe controls per precision profile (52 and 8 bits),
including omitted focal output and a removed publication guard. Legal repeated
writes remain accepted; emitted LLVM and frozen ABI descriptions are retained.

Focus cases cover ordered weighted centroids, full focal tuple publication,
positive finite participant mass, stale identities/generations, duplicates,
Focus/References/CascadeWork limits, identical-selection NoChange and stable
prepared backing without recurring C++ allocation. Populated disabled publication
preserves all state. Successful selection preserves body payloads, source/contact
indices, pending notifications, unequal dirty hierarchy packets and reader history.
Radial-dependent origin changes wake present bodies; unrelated sleepers and unused
rows remain unchanged. Subsequent placement uses the new focus, and the following
tick consumes the same installed model. Observation validity and prepublication
healthy rejection versus postpublication unready failure are checked explicitly.

The first expanded run at `55b33de0` passed functional cases but failed an
overbroad compiler guard check: it classified compiler/runtime bookkeeping stores
as model publication. Diagnostics at `47124f3f` identify a RuntimeContext stack
closure; a subsequent attempt also encountered runtime locking. The final checker
matches the contact publisher's scope: external model arrays, native selection
writes and activation must be guarded. Owned-writer checks remain active, and
missing-guard/omitted-output unsafe controls still fail. Earlier failed logs are
retained separately and are not passing qualification.

Full CPU regression, remaining compiler configurations, PostgreSQL and GPU were
not rerun. Shared-host timings are not performance qualification. This evidence
supports provisional review; hosted CI and maintainer acceptance are independent.

### PR #211 review correction: retained contact references

[Review 5773022878](https://github.com/Human-Cognome-Project/human-cognome-project/pull/211#pullrequestreview-5773022878)
found that focus selection charged the maximum of source/installed contacts,
although their endpoint arrays are separately resident. The corrected demand is
the source ledger's complete references plus proposed focus, installed hierarchy
and twice the installed union pair count. No additional union is built by focus
selection. Counts remain bounded by the prepared native dimension limits.

The new tight-budget fixture contains one explicit source pair and one separately
installed union pair. It first establishes the union through a stationary tick,
then installs the hierarchy and creates dirty child history. At one reference
below the required peak, focus selection must return typed References exhaustion
with the exact demand and retain the state snapshot and prior geometry observation.
At the exact limit it must publish successfully. The test reference helper now
also counts installed union endpoints.

The first attempt at `8c663874` failed its fixture prerequisite because a source
contact alone does not install a physical union; that log is retained separately
and is not passing evidence. The corrected fixture establishes the union through
the normal tick path before adding hierarchy references.
The intermediate fixture at `fe5cb0cc` then correctly rejected stale hierarchy
source references after that tick advanced the generation. The final fixture
binds those references to the current generation; both failed setup logs are
retained separately.

Corrected code/test pin: `df624c42aee53c17822c403fb0cee2552fd065cb`.
The complete `live_focus` case passed on CPU in one-worker bulk/default and
two-worker direct/no-unroll modes. Both report required=36, capacity=35 rejected
without publication and capacity=36 successful. Native kernels are unchanged;
the full placement/regression and compiler matrices, PostgreSQL and GPU were not
rerun for this host accounting correction. Prior green hosted results apply to
the prior head, not this revision. Subsequent documentation does not alter code.

Bundle `focus-reference-df624c42` retains commands, raw logs, failed setup attempts,
source/submodule pins, build configuration, toolchain and binary/runtime hashes.
Its `SHA256SUMS` digest is
`b10320d394488e9161969fcc7f181de19c3e47c9afca5c60b9077c94ae371d85`.
The archive is retained in the Windows workspace. This contributor-local evidence
is not remotely available or independently verified merely by publishing its hash.

## Private exact coordinate rebase — 2026-11-04

This is a continuation of provisionally accepted #211 at
`153f6124193bfe027253716e0672ec41b755bc9e`. It does not claim that dependency's
pending hosted checks or final merge have completed. The [implementation and
numerical limits](../../docs/COORDINATE-REBASE.md) describe a private full-precision
D6 boundary; reduced profiles, operational controls and GPU remain separate.

Code/test pin: `26e8a74dfbe453dc420ea1ba85c4d609c15aa25e`.

All scoped local CPU checks passed: fifteen combined placement/admission cases
in one-worker bulk/default mode; rebase and compiler checks in two-worker
direct/no-unroll mode; prepared update and dirty hierarchy regressions. Each
compiler configuration covers 16 kernels and 24 rejected unsafe controls at
both 52- and 8-bit body precision. The 8-bit compiler checks do not qualify
reduced-precision rebasing; the operational request is explicitly rejected.

Bundle `coordinate-rebase-26e8a74d` retains commands, source/submodule pins,
configuration, toolchain, binary/runtime hashes, raw logs, emitted LLVM and frozen
ABI metadata. Its `SHA256SUMS` digest is
`02269c8e39af288e02d7ac380b3a19a42b65ec15b0e8a1479fb35855dc555f9a`.
A matching archive is retained in the Windows workspace. These are contributor
results; the local bundle is not remotely available or independently inspected
merely because its digest is published. Subsequent documentation changes no tested
code. Full 115-suite regression, remaining compiler modes, PostgreSQL and GPU were
not rerun; shared-host timings are not performance qualification. Hosted CI and
maintainer acceptance remain independent.

The rebase case uses 70/128 capacities with holes, sleeping bodies, explicit and
installed contacts, dirty hierarchy packets and live velocity. It checks exact
translations around `2^27`, both packet slots and reader histories, native previous
centres, frame/focal controls and inverse translation. A tick in the translated
frame is compared with an otherwise identical unrebased execution. Rejections
cover stale generation, work exhaustion, nonfinite inputs, lost small/large
translations, tiny FTZ-range coordinates in all three retained-history surfaces,
and independently reduced body/field/hierarchy precision. Ordinary rejection
preserves state/readiness/observations; injected late failures refuse resumed use.
Prepared backing remains stable with no observed recurring C++ allocation.

The compiler matrix now includes separate rebase check and publication kernels.
Check ownership permits only the status scalar; publication ownership permits
only its coordinate arrays and requires its enable guard. Both use strict math.
Omitted-output, contracted-arithmetic and missing-publication-guard mutations
are negative controls, while legal repeated writes remain accepted.

Earlier attempts are retained separately, not counted as passing qualification:
`466a06f2` encountered the compiler's nested sparse-traversal restriction;
`084aa195` exposed a test expectation that incorrectly treated previous-centre
coordinates as untranslated motion. Combined check/commit kernels at `e0cf2c63`
and `5a059d82` failed emitted guard audits, including live-coordinate stores
outside an explicit publication guard. The implementation now uses two kernels
so validation cannot write those coordinates. The conservative FTZ residual range
was added without changing the runtime floating-point mode.

The broader run at `21a41427` completed twelve cases, including rebase, before
exhausting the Program's default 1,024 monotonic SNode IDs in later fixtures.
It is retained as an incomplete run. The combined test runner now explicitly
reserves 2,048 IDs without changing production model budgets. The final moving
continuation witness also asserts nonzero displacement at positive dt.

### Maintainer hosted acceptance of #211 — 2026-11-04

Corrected head `153f6124193bfe027253716e0672ec41b755bc9e` is accepted in
[review 5775143515](https://github.com/Human-Cognome-Project/human-cognome-project/pull/211#pullrequestreview-5775143515).
All 18 hosted checks passed. Independently inspected native run 41886914617:
regression jobs 126782664057, 126782664215, 126782664440 and 126782664008 passed
29/29, 29/29, 29/29 and 28/28 tests respectively, collectively indexes 1–115
exactly once. Job 126782663799 passed 180 integration checks with 24 addressed
reads and zero private reads. All four downloaded compiler artifacts cover the
eight worker/source/unroll configurations; placement audits pass 14 kernels and
19 unsafe controls per precision profile. Extra no-unroll placement runs include
the real source/installed-contact reference witness: required 36, rejected at 35,
accepted at 36. Contributor-local evidence above remains separately pinned.

This accepts private existing-model focus selection only. Exact coordinate
rebasing is reviewed separately in #212/#213; representation exchange,
operational owner/public controls and GPU qualification remain outstanding.

### Maintainer hosted acceptance of #212 — 2026-11-04

Head `78b5360211050aa02fb541cbc140eda7cf257ddf` is accepted in
[review 5775169104](https://github.com/Human-Cognome-Project/human-cognome-project/pull/212#pullrequestreview-5775169104).
All 18 hosted checks passed. Independently inspected native run 41894881085:
regression jobs 126804479193, 126804478860, 126804479017 and 126804478903 cover
all 115 test indexes exactly once; database job 126804478949 passes 180 checks
with 24 addressed reads and zero private reads. Downloaded compiler artifacts
13357258083, 13356489852, 13356468362 and 13355869838 pass all eight configurations,
with 16 kernels and 24 unsafe controls per profile. All four extra no-unroll
placement logs pass capacities 70/128, exact 2^27 translation/inverse, both history
slots, unchanged rejection and moving continuation. These are inspected hosted
CPU results, distinct from the contributor-local evidence above.

Only private full-precision exact rebasing is accepted here. #213 reduced-profile
support remains provisional; legacy f32, rounded transforms, D7 exchange,
operational controls and GPU qualification remain separate.
