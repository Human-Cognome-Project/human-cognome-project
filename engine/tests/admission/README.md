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

## Reduced-significand exact rebase — 2026-11-04

This continues provisionally accepted #212 at
`78b5360211050aa02fb541cbc140eda7cf257ddf`. Its pending hosted validation and
maintainer integration remain independent. The private D6 boundary now accepts
0–52 retained fraction bits only when every translated f64 coordinate survives
its destination mask unchanged. It never rounds a rebase to make it fit.
Installed field/history coordinates use the native owner-node mask, including
independent hierarchy precision. Body and unowned-field coordinates use their
respective profiles. Both existing kernels bind the native mask array; the fixed
parameter reservation increases by two, with no new numerical backing.

Code/test pin: `73e22ec7e38ba62c041e90de2687f6f254f16bb7`.

The scoped qualification runs the complete `coordinate_rebase` case on CPU in
one-worker bulk/default and two-worker direct/no-unroll modes. Four reduced
fixtures independently cover 8-bit body, 8-bit field, independently configured
8-bit hierarchy and zero-fraction-bit profiles. Fields occupy nonidentity owner
slots. Each checks exact shift/inverse, unchanged rejection of a subtraction
that is exact in f64 but loses destination bits, stable backing, no observed
recurring C++ allocation and a subsequent positive-dt tick. Separate witnesses
make only previous centres, backup packets or reader history fail the precision
gate. Existing full-precision preservation and failure witnesses remain in the case.

The compiler audit covers 16 kernels and 24 rejected unsafe controls at both
52/8-bit profiles in both modes. It additionally verifies the actual frozen
native mask pointers, type, rank and extent. Prepared update and dirty hierarchy
refresh are the accompanying regressions. Full placement/115-suite regression,
remaining compiler modes, PostgreSQL and GPU are outside this scoped rerun.
Shared-host timings are not performance qualification. Legacy f32 and rounded
rebasing remain unsupported; D7 exchange and operational/browser wiring remain
separate work.

All scoped checks above passed. Bundle `reduced-rebase-73e22ec7` retains the
qualification commands, source/submodule pins, build configuration, toolchain,
binary/runtime hashes, raw logs, emitted LLVM and frozen ABI metadata. Its
`SHA256SUMS` digest is
`4530186856be84a54408d5615b5e819b50b9bc5f3650236cc987b9d34c1522b4`.
A matching archive is retained in the Windows workspace. The raw bundle is local,
not remotely available or independently verified merely by publishing its hash.
Subsequent documentation changes no tested code. Hosted CI and maintainer
acceptance remain independent.

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

## Private D7 outgoing-state prerequisite — 2026-11-04

This continues provisionally accepted #213 at
`65f78f5e9363d851b4ef5f350796b8e781be277d`, with main's authoritative #211/#212
acceptance integrated. The [implementation scope](../../docs/EXCHANGE-STATE.md)
is a native mass-weighted current centre/velocity reduction over validated body
handles. It is scratch-only and does not perform an aggregate/parts exchange.

Code/test pin: `b3626223050ea4be0d9241a679c81c7ae9f0cfb8`.
Scoped qualification covers `exchange_state` at 52/8-bit position precision on
CPU bulk/default with one worker and direct/no-unroll with two workers. Witnesses
include unequal masses, aggregate/set reduction, independent nonbinary velocity
reference, caller-order invariance, sleeping bodies/retained dirty packets,
unrelated occupants, stale/duplicate/empty selection, exact work boundaries,
invalid numerical inputs and retry. Snapshots, prior geometry/placement tokens,
prepared backing and the no-recurring-C++-allocation guard check preservation.

The compiler audit covers 17 kernels and 26 rejected unsafe controls per 52/8-bit
profile in both modes, including the new resident input binding and scratch-only
output. Prepared update is the accompanying regression. Full placement/115-suite
regression, remaining compiler modes, PostgreSQL and GPU are not part of this
rerun. No GPU work occurred. Shared-host timing is not performance evidence.

The initial `581665e7` attempt failed C++ compilation on incompatible conditional
IR pointer types; the common `Stmt` base fixes it. That log is retained separately
and is not passing qualification. Subsequent documentation changes no tested code.
Hosted validation and maintainer acceptance remain independent.

All scoped checks above passed. Bundle `exchange-state-b3626223` retains commands,
source/submodule pins, build configuration, toolchain, binary/runtime hashes,
raw logs, emitted LLVM and frozen ABI metadata. Its `SHA256SUMS` digest is
`8fccb05c3195d5f7f5f5067f68882a3f6010e3552c0485feb5901db8944bacae`.
A matching archive is retained in the Windows workspace. This is contributor-local
evidence; the raw bundle is not remotely available or independently verified
merely by publishing its digest.


## PR #213 hosted continuation correction — 2026-11-04

[Review 5775793038](https://github.com/Human-Cognome-Project/human-cognome-project/pull/213#pullrequestreview-5775793038)
holds #213/#214 integration after hosted two-worker combined placement failures
at `65f78f5e`. The compiler audits themselves passed; the existing full-precision
moving rebase witness failed cross-execution equality before the reduced fixtures.

Diagnostics retain component/slot/value differences and nonzero before/after
body, field, history, packet, selector and activity state. Local original-witness
runs passed all 15 combined cases in two-worker bulk/default, direct/default and
direct/no-unroll, both at #213 diagnostic pin
`019115ac38f6f9beb43e40a9552dba047f804519` and accepted #212 plus diagnostics at
`650dad00fe8e440a2a27d3904f699d14759e5c41`. Standalone two-worker bulk rebase
also passed at each pin. Thus the natural hosted timing was not reproduced
locally; these passes do not dismiss that failure.

The demonstrated test-contract defect is inherited: independent native hierarchy
traversals need not consume the same complete sibling publication generations.
The [corrected witness](../../docs/COORDINATE-REBASE.md#hosted-continuation-correction)
uses exact fixture-specific parent values and requires pending work for lag.
It retains exact body/motion, unaffected payload, prior history and selector
checks, adds per-poll publication/old-slot lifetime checks and requires exact
catch-up without body changes. Negative controls reject missing pending work,
an intermediate parent coordinate and wrong mass. No epsilon, production kernel,
precision gate or scheduling change was introduced.

Explicit parent-first/child-first targeted polls reproduce differing valid
parent values (`1.0625` versus `1.125`) on the accepted #212 engine at diagnostic
test pin `7bac08ce219e4047410170ad5a75156cfc043494`; both converge exactly on
the next poll with history lifetime preserved. This controlled counterexample
is separate from the naturally scheduled runs. It demonstrates why the old
assertion is invalid, but hosted confirmation is still needed to reconcile the
original failure fully.

Corrected #213 code/test pin: `b38ad7e5a5ce05af5fac88160a76f2e3c2ae5da2`.
Dependent #214 merge/test pin: `41fd4df2133c6b607f0b605ee2f47a3bfc5aec22`.
Subsequent changes are documentation only. The initial controlled-witness build
at `312d9cda` failed because a long-double expectation was narrowed into an f64
packet initializer; the expectation parameter now matches the packet type. That
failed build is retained separately and is not passing evidence.

All corrected scoped checks passed: #213's complete 15-case placement suite in
two-worker bulk/default, direct/default and direct/no-unroll, plus a separate
standalone bulk rebase. Each combined compiler audit passes 16 kernels and 24
unsafe controls per 52/8-bit profile. #214 passes the complete 16-case combined
suite in two-worker direct/no-unroll (17 kernels/26 controls per profile), separate
two-worker bulk rebase and one-worker bulk exchange_state. Both capacities retain
the forced-order counterexample and exact catch-up in the corrected rebase case.

Bundle `rebase-continuation-b38ad7e5-41fd4df2` retains all original diagnostic,
controlled-baseline and corrected raw logs, exact pins, commands, binary/runtime
hashes, configuration and toolchain. Its `SHA256SUMS` digest is
`044ff224916315920620264f11f280ba791688571a326c61c4d470f7c899eafd`.
A matching archive is retained in the Windows workspace. Raw evidence remains
machine-local and is not independently verified merely by publishing its hash.

Full 115-suite regression, remaining compiler modes, PostgreSQL and GPU were not
rerun. No GPU work occurred; shared-host timing is not performance qualification.
The source-only test correction and local passes do not replace current-head
hosted reconciliation or maintainer approval for either PR.

## Private replacement staging — 2026-11-04

The [replacement staging prerequisite](../../docs/EXCHANGE-PLACEMENT.md) follows
the source-reviewed provisional #213/#214 corrections. Test/source pin:
`dc620b5fde3007f80cd58b1bb68679e94621b109`, modified Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`. Subsequent changes are documentation
only. Local WSL Ubuntu/LLVM 15 execution reported the actual CPU backend.

Passed:

- `live_hierarchy_test --threads 1 --case exchange_placement`: 52/8-bit
  compression, expansion, independent nearest-free certificates, sleeping
  occupants, native velocity, rejection/retry, ordinary reuse, unchanged state
  and old-plus-new claim overflow.
- `live_hierarchy_test --threads 1 --case placement_compiler`: 18 kernels and
  27 unsafe controls per 52/8-bit profile, with retained emitted LLVM.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all 17 combined cases, including the same compiler audit, ordinary generated
  rows, source/contact preflight, admission/publication failure, bootstrap,
  corrected rebase, outgoing state, replacement staging, live focus and contact
  remapping. Zero case failures.

The matching native build used `cmake --build engine/build --target
live_hierarchy_test -j 2`; runs sourced `engine/build/engine-env.sh` and used
`nice -n 10`. `HCP_B2_IR_DIR` retained compiler modules in each host mode.
The bundle includes exact commands, source/submodule pins, configuration,
toolchain, binary/runtime hashes, raw logs and emitted code:
`/root/hcp-workspace/evidence/exchange-placement-dc620b5f`, with matching Windows
workspace archive `setup-evidence/exchange-placement-dc620b5f.tar.gz`.
Its `SHA256SUMS` digest is
`c7f24d8d50499ab9b86f6b63cf149d3d2fee97bd8ede217da531456cf2792eb1`.
This is contributor-local evidence, not remotely available or independently
verified merely by publishing its hash.

The earlier `b2c3a771` initial replacement case passed. At `56c763a1`, the expanded
placement/motion checks passed but the peak-capacity fixture failed construction:
its field limit was below its retained focus reservation. The correction reduces
only particle capacity. The superseded combined run was explicitly stopped;
its partial log and the fixture failure are retained separately, not counted as
passing evidence. Production code did not change for this fixture correction.

Full 115-suite regression, other compiler configurations, PostgreSQL and GPU were
not rerun. Shared-host timing is not performance evidence. These checks qualify
private scratch staging only, not logical occurrence/source conservation or
atomic exchange publication. Hosted reconciliation of the provisional dependency
stack and review of this increment remain separate acceptance gates.

## Maintainer hosted acceptance of corrected #213/#214 — 2026-11-04

Accepted heads: #213 `aa50a5a79a482a89a5e6a7b62b9bdae3315fba68` and #214
`d81dd279a5b35a70ac436cfb742fdc0af30d54ec`. All current-head checks pass.
Independently inspected runs 41920995451 (#213) and 41920995839 (#214):
each covers all 115 test indexes exactly once and 180 database checks with
24 addressed reads and zero private reads. All four compiler artifacts from
each run were downloaded and inspected, covering eight worker/source/unroll
configurations. #213 reports 16 kernels/24 unsafe controls per profile;
#214 reports 17 kernels/26 controls. All four extra no-unroll placement suites
pass corrected exact legal-parent, pending-work, history-lifetime and forced-order
catch-up witnesses. #214 also passes outgoing-state witnesses at 52/8 bits.

The earlier hosted continuation failure is reconciled by the reviewed test-contract
correction and renewed hosted coverage; its natural timing was not reproduced
locally, and the old failed runs are not relabelled as passes. This acceptance
covers private reduced exact rebasing and outgoing-state reduction only.
#215 replacement staging is separately provisional. Complete D7 source/momentum
accounting, logical alternative validation, prospective hierarchy/contact exchange,
atomic retirement/publication, operational controls and GPU qualification remain
outstanding. Contributor-local evidence above remains separately pinned.

## Maintainer hosted acceptance of #215 — 2026-11-05

Reviewed head `616bcbcd3ae2ed6fe91cb739774d98cf0cd8f3af` is accepted in
[review 5785165393](https://github.com/Human-Cognome-Project/human-cognome-project/pull/215#pullrequestreview-5785165393).
All 18 checks pass. Independently inspected run 41928920745: regression jobs
126923592010, 126923592158, 126923592173 and 126923592137 pass 29+29+29+28
tests, covering indexes 1–115 exactly once. Database job 126923592030 passes
180 checks with 24 addressed reads and zero private reads. Downloaded compiler
artifacts 13373251068, 13372263149, 13371699265 and 13371299838 cover all eight
configurations, with 18 kernels/27 unsafe controls per profile. All four extra
no-unroll placement logs pass replacement witnesses at 52/8 bits and report zero
case failures. Contributor-local evidence above remains separately pinned.

This accepts private scratch replacement staging only. Complete logical/source,
momentum, hierarchy/contact and atomic retirement/publication coordination is
the next D7 work; public operations and GPU qualification remain separate.

## Logical exchange declaration — 2026-11-05

The [prepared declaration](../../docs/EXCHANGE-CONTRACT.md) was tested at
`a26dafd7eb0abbca4582c829a1181f3967f1590b`, based on merged main `e25c91cd`.
Later changes are documentation only. WSL Ubuntu/Clang 15 host structural tests:

- `structural_recipe_test`: 91 checks, including exchange directions, exact
  occurrence/definition/source descriptors, duplicate copies and contributions,
  invalid inputs, comparison work boundary and unchanged projection.
- `letter_projection_test`: 53 checks.
- `membership_fields_test`: 25 checks.
- `letter_projection_native_test`: existing native-layout projection regression.

All four suites passed. Build commands use `cmake --build engine/build --target
structural_recipe_test letter_projection_test membership_fields_test -j 2`, then
the `letter_projection_native_test` target. Verbose CTest selections are
`^(structural_recipe_test|letter_projection_test|membership_fields_test)$` and
`^letter_projection_native_test$`. This is host metadata/layout validation; no
Taichi numerical kernel, database or GPU was run, and the full numerical matrix
was not repeated for this structural-only change.

Raw bundle: `/root/hcp-workspace/evidence/exchange-contract-a26dafd7`, with
workspace archive `setup-evidence/exchange-contract-a26dafd7.tar.gz`.
Its manifest digest is
`3d520a4062c9a3f405a563a8099f0d838a96e43cafa4e2afaee470e4283ab457`.
The bundle retains commands, source/submodule pin, build settings, toolchain,
binary hashes and verbose results. The initial `0dd2a068` test expectation
incorrectly counted the repeated-letter-plus-combining-accent fixture as two
bytes; it contains four. The corrected assertion retains all eight nibble
contributions. That failed log is separately retained, not passing evidence.

This is contributor-local evidence, not independently inspected merely because
its hash is published. Committed occurrence-to-native-handle binding, source
ledger validation, momentum/reporting and atomic exchange remain outstanding.

## Private committed-source binding view — 2026-11-05

The [binding prerequisite](../../docs/EXCHANGE-BINDINGS.md) follows provisionally
accepted #216 without changing native numerical kernels. Test/source pin:
`a574ec7d660dfdf5ba69a63500f862ffed2f762b`, modified Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`. Subsequent changes are documentation
only. WSL Ubuntu/Clang 15 runs reported the actual CPU backend.

Passed:

- `live_hierarchy_test --threads 1 --case exchange_bindings` (bulk/default).
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case exchange_bindings`.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all 18 combined cases. The existing compiler audit remains 18 kernels and 27
  unsafe controls per 52/8-bit profile, with emitted LLVM retained.

The binding witness checks both directions, separate copies, nonidentity records
and fields, inherited membership counts, all full-handle components, generation
and work limits, changed mass/count/share/order/endpoints, extra incident source,
retry and unready-owner refusal. Thirty-two read-only validations observe no
recurring C++ allocation, no synchronization/transfers and unchanged execution,
history and prepared backing. Preparation allocates separate host binding
metadata; this does not qualify the enduring owner's total resource budget.

Build: `cmake --build engine/build --target live_hierarchy_test -j 2`. Runs source
`engine/build/engine-env.sh`, use `nice -n 10`, and retain combined emitted code
with `HCP_B2_IR_DIR`. Raw bundle:
`/root/hcp-workspace/evidence/exchange-bindings-a574ec7d`, matching Windows archive
`setup-evidence/exchange-bindings-a574ec7d.tar.gz`. Manifest digest:
`0c8471aadb50637bb8a562f76c3776943ff24a21d163b69bdb23938eb49eccd5`.
Commands, pins, binary/runtime hashes, build settings, toolchain and logs are
retained. The initial `e101daf3` fixture attempted multiple hierarchy roots and
failed setup; the corrected single-root fixture preserves the same exact logical
source rows. That failed log is separate, not passing evidence.

Raw evidence remains contributor-local, not independently verified by publishing
its digest. Full 115-suite regression, other compiler modes, PostgreSQL and GPU
were not rerun. Shared-host timing is not performance evidence. This private
generation-bound check assumes trusted initial owner mappings; it is not an
operational binding API, numerical conservation check or atomic exchange.

## Hosted acceptance of structural declaration #216 — 2026-11-05

Reviewed head `3e273a8a14904af171ee77bfbfe0f2e15ea22ab2`, merged as
`639901b106fcd4319af3c411644d03e40a2ac856`; final review
[5785904822](https://github.com/Human-Cognome-Project/human-cognome-project/pull/216#pullrequestreview-5785904822).
All 18 checks passed. Inspected hosted run
[42032548669](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42032548669)
covers all 115 test indexes exactly once across 29/29/29/28-test shards,
180 integration checks (24 addressed reads, zero private reads), and four
compiler artifacts retaining 18 kernels/27 unsafe controls per 52/8-bit profile
and passing extra no-unroll placement witnesses in every worker/source-path mode.
The reviewer also directly compiled the structural test with GCC and passed
91 checks; that focused result is separate from hosted and contributor-local evidence.
This accepts immutable structural declarations only. #217's committed-source
binding view remains provisionally reviewed with hosted validation pending;
native conservation and complete atomic exchange remain outstanding.

## Contributor-local exchange accounting — 2026-11-05

Tested source `bdccf872b65d458ab1a6014dc1e4a5c2490369ff`, actual native CPU in
WSL Ubuntu with matching Taichi fork and LLVM/Clang 15. This follows #217's
provisional source review 5785905901; #216 is fully accepted above. Later changes
on this branch are documentation only.

Build: `cmake --build engine/build --target live_hierarchy_test -j 2`; source
`engine/build/engine-env.sh`, run with `nice -n 10`:

- `live_hierarchy_test --threads 1 --case exchange_placement`: passed 52/8-bit
  compression/expansion accounting, mass/source rejection, missing/unsupported
  exact metadata, native velocity and momentum corruption, fractional weighted
  velocity, actual centre/nonzero offset, retry and unchanged state/backing.
- `live_hierarchy_test --threads 1 --case placement_compiler`: bulk/default
  audit passed 19 kernels and 29 unsafe controls per 52/8-bit profile.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all 18 combined cases passed, including the same 19-kernel/29-control audits.

Both audit modes retain emitted LLVM via `HCP_B2_IR_DIR`. The initial source
`a5af9d07` test fixture declared exact masses 1,1,1 for physical masses 1,3,2;
the new native check correctly rejected it. The corrected metadata and additional
corruption witnesses are in the tested source. Its failed log is retained
separately and is not passing evidence.

Raw bundle `/root/hcp-workspace/evidence/exchange-accounting-bdccf872`; Windows
archive `setup-evidence/exchange-accounting-bdccf872.tar.gz`. SHA256SUMS digest:
`2e09b6665cebbfd2ccf7abd52422c2e207a3596ceb83c7f2488dee2750167a18`.
Commands, source/fork pins, build settings, toolchain, binary/runtime hashes,
logs and emitted code are retained. Raw evidence is contributor-local; publishing
its digest does not constitute independent inspection.

[Accounting scope and arithmetic bounds](../../docs/EXCHANGE-ACCOUNTING.md) cover
private scalar reconciliation and diagnostics only. Logical incoming source
validation, prospective hierarchy/contact integration, atomic publication and
total operational owner accounting remain outstanding. Full 115-suite regression,
remaining compiler configurations, PostgreSQL and GPU were not rerun. Shared-host
timing is not performance evidence. Hosted CI remains authoritative for its scope.

## Contributor-local private atomic exchange — 2026-11-05

Tested source `9a016b5e1e6cb2117713c19eacfbdee08bd200e8`, following #218's
[provisional review 5786373902](https://github.com/Human-Cognome-Project/human-cognome-project/pull/218#pullrequestreview-5786373902)
at `0b0093453eaa8098582d7dc2e8a52939af058f4c`. Main's disposition `f271b4f5`
is integrated. #217/#218 remain provisional dependencies; no duplicate hosted
suite was requested. Later changes on this branch are documentation only.

Actual native CPU, WSL Ubuntu, matching Taichi source/runtime and LLVM/Clang 15:

- Build: `cmake --build engine/build --target live_hierarchy_test -j 2`.
- Source `engine/build/engine-env.sh`; run with `nice -n 10`.
- `live_hierarchy_test --threads 1 --case exchange_commit`: passed all eight
  scenarios, including multibody expansion/compression, particle claim and contact
  exhaustion, and failure after claims/numerical/contact publication or immediately
  before logical mapping publication.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all **19 combined cases** passed. Compiler audit remains **19 kernels and 29
  unsafe controls per 52/8-bit profile**, with emitted LLVM retained via
  `HCP_B2_IR_DIR`. Native numerical kernels are unchanged.

The two-byte letter witnesses retain independent aggregate and byte copies and
nonidentity record/field mappings. They check sealed incoming identity/definition,
committed-source revalidation, invalid prospective hierarchy, work reservation,
old-plus-new claims, unchanged rejected execution/history/mappings, retry,
transferred motion and retained target/centre/offset/momentum diagnostics,
actual-claim occurrence mappings, unchanged survivor numerical state, one unticked
generation, consumed-plan replay refusal, following ticks and stable native
backing with no observed recurring C++ allocation. Late failures leave the harness
unready and do not publish a healthy new logical mapping.

Initial `c7c159a8` single-byte and `a5a0af0b` multibyte compression fixtures left
empty retained hierarchy fields; native preflight correctly rejected them.
`515e7e7a` retained an independent byte copy and passed the corrected scenarios.
Final source additionally qualifies retained reports, invalid prospective hierarchy
and the mapping-publication failure seam. These earlier logs are preserved
separately and are not final-source evidence.

Raw bundle `/root/hcp-workspace/evidence/atomic-exchange-9a016b5e`; Windows archive
`setup-evidence/atomic-exchange-9a016b5e.tar.gz`. SHA256SUMS digest:
`57f2324797c83938fa138c4d0168b5897640f4a36d200031d369d313267831bc`.
Commands, source/fork pins, settings, toolchain, binary/runtime hashes, logs and
emitted code are retained. Raw evidence is contributor-local; the digest does not
constitute independent inspection.

[Private coordinator scope](../../docs/ATOMIC-EXCHANGE.md): existing field inventory,
trusted prepared intrinsic/layout associations and a consumed generation-bound
transaction. Field-family creation/retirement, enduring owner/projection lifecycle,
complete owner resource accounting and operational controls remain separate.
Full 115-suite regression, remaining compiler modes, PostgreSQL and GPU were not
rerun. Shared-host timing is not performance evidence; hosted CI remains
authoritative for the checks it can perform.


## Maintainer hosted acceptance — #217/#218, 2026-11-05

Reviewed #217 head `34e323925b6d02a1ce5b79042673dd2369ae7edc` and #218 head
`0b0093453eaa8098582d7dc2e8a52939af058f4c` are accepted. All 18 hosted checks
passed on each head. Independently inspected runs 42038267169 and 42042807142:
each covers 115 unique regression tests exactly once and 180 database checks
with zero private reads. Retained artifacts cover all eight compiler configurations
and all four additional no-unroll placement matrices. #217 audits 18 kernels/27
unsafe controls per 52/8-bit profile; #218 audits 19/29 and passes the native mass,
momentum and offset witnesses. Local evidence above remains separately attributed.

Integration preserves reviewed engine/test source and main's review history;
the conflict resolution changes documentation only. #219's private coordinator
is provisionally reviewed in review 5787134942, with hosted run 42050645525 still
pending at disposition. No duplicate native run was requested. Enduring owner,
field-family lifecycle, total owner resources and GPU qualification remain separate.

## Contributor-local exchange field lifecycle — 2026-11-05

Tested source `d7676dea8d03a7a90f37ff97c578eb04143da102`, following #219's
[provisional review 5787134942](https://github.com/Human-Cognome-Project/human-cognome-project/pull/219#pullrequestreview-5787134942)
at `2b0d002ea85a3c0388cc4cc28a6e60ea39f24162`. Main's accepted #217/#218
integration `372d47f5` is merged, retaining both evidence histories. #219 remains
the provisional dependency; its reviewed branch and running CI were not changed.
Later changes on this branch are documentation only.

Actual native CPU in WSL Ubuntu, matching Taichi source/runtime and LLVM/Clang 15:

- Build: `cmake --build engine/build --target live_hierarchy_test -j 2`.
- Source `engine/build/engine-env.sh`; run with `nice -n 10`.
- `live_hierarchy_test --threads 1 --case exchange_fields`: all five scenarios
  passed: expansion, compression, old-plus-new field capacity, focal retirement
  rejection and late mapping-publication failure.
- `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all **20 combined cases** passed. Compiler audit remains **19 kernels and 29
  unsafe controls per 52/8-bit profile**; emitted LLVM retained via `HCP_B2_IR_DIR`.
  Native numerical kernels and prepared numerical arrays are unchanged.

The field witnesses start with only one alternative of a two-byte letter present.
Expansion creates six fields and retires two; compression creates two and retires
six. The membership/focal field survives with its original full handle. Tests
check missing/duplicate/extra logical fields, aliased native addresses, actual-claim
field mappings, no outgoing record reuse, full occurrence/field mapping preservation
on rejection/failure, unchanged execution/history and retry, one unticked generation,
stable native backing, no observed recurring C++ allocation and a following tick.
Final seven-field demand still requires nine simultaneous claims. Retiring a focal
field rejects without implicit focus changes. Existing atomic exchange witnesses
also pass with fields shared by independent copies.

Initial `cd5a4e49` passed creation/retirement in both directions, then its capacity
fixture setup correctly rejected Fields=7 with Focus=70. Final source sets Focus=7
and adds complete mapping/address-alias checks. The earlier log is preserved
separately and is not final-source passing evidence.

Raw bundle `/root/hcp-workspace/evidence/exchange-fields-d7676dea`; Windows archive
`setup-evidence/exchange-fields-d7676dea.tar.gz`. SHA256SUMS digest:
`e88e03c6754e312c5a54a19ddb11e9c974f548b09111574322f4c9a457d47932`.
Commands, source/fork pins, build settings, toolchain, binary/runtime hashes, logs
and emitted code are retained. This is contributor-local evidence; publishing the
digest does not constitute independent inspection.

[Field lifecycle scope](../../docs/EXCHANGE-FIELDS.md) remains a consumed private
plan with trusted initial identity/layout bindings and an owner-supplied prospective
hierarchy. Enduring owner/projection lifecycle, complete owner resource accounting
and operational controls remain separate. Full 115-suite regression, remaining
compiler modes, PostgreSQL and GPU were not rerun. Shared-host timing is not
performance evidence. Hosted CI remains authoritative for its scope.

## Maintainer hosted acceptance — #219, 2026-11-05

Reviewed head `2b0d002ea85a3c0388cc4cc28a6e60ea39f24162` is accepted in
[review 5790172617](https://github.com/Human-Cognome-Project/human-cognome-project/pull/219#pullrequestreview-5790172617).
All 18 hosted checks passed. Independently inspected run 42050645525 covers
115 unique tests exactly once, eight compiler configurations (19 kernels/29
unsafe controls per 52/8-bit profile), all four extra no-unroll atomic exchange
matrices, and 180 database checks with zero private reads. Contributor-local
evidence remains separately attributed. Integration preserves reviewed engine/test
source and both acceptance histories. #221 field lifecycle is provisionally
accepted in review 5790162176, with hosted run 42086483712 pending. Enduring owner,
projection lifecycle, total resources and GPU qualification remain separate.

## Local persistent exchange-owner qualification — 2026-11-05

Tested code: `51de69c116b190b451330d2af439234ec8edeaca`, with matching Taichi
`5ba579a8a11255b7a258cde93801070956d81d57` and LLVM/Clang 15. This contribution
depends on provisionally accepted #221; accepted #219/#220 and refreshed main
guidance are integrated by normal merge `89b28986`. It is not final acceptance.

Actual native CPU in WSL Ubuntu, with `engine/build/engine-env.sh` sourced:

- Build `live_hierarchy_test` with `cmake --build engine/build --target live_hierarchy_test -j 2`.
- `nice -n 10 engine/build/live_hierarchy_test --threads 1 --case exchange_owner`:
  32 alternating exchanges and following ticks passed, then the injected late
  owner-publication failure correctly left execution unready.
- `nice -n 10 engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`:
  all **21 combined cases** passed, including the same repeated-owner sequence.
  Compiler audits remain **19 kernels/29 unsafe controls per 52/8-bit profile**;
  emitted LLVM is retained via `HCP_B2_IR_DIR`.
- Rebuilt and passed `letter_projection_test`, `membership_fields_test` and
  `structural_recipe_test` through CTest. No numerical kernel was added or changed.

The new witness retains one study UUID, pinned immutable recipe and occurrence
keys through 32 changes of live exposure. It checks active/structural projection,
exact parent and inherited membership contributions, actual occurrence/field
handles, retained focal membership, coherent maps/reports, consumed-plan rejection,
unchanged stale/work/ordinary rejection and retry, and late unready behavior.
Native backing remains stable. C++ `new` guards cover successful execution,
ordinary rejection and ticks, excluding preparation and exception construction.
The repeated-owner fixture has one logical occurrence; existing `exchange_commit`
separately exercises preservation of independent copies.

Initial `999c072f` built successfully but its focused test incorrectly assumed
retained membership was the last field-map entry. Final source locates it by
logical identity and also checks current structural-handoff exposure. The failed
focused log and deliberately stopped initial combined run are retained separately;
neither is passing final-source evidence.

Raw bundle `/root/hcp-workspace/evidence/exchange-owner-51de69c1`; Windows archive
`setup-evidence/exchange-owner-51de69c1.tar.gz`. SHA256SUMS digest:
`a20339045e490902109e71eeb5302c8f5cc7ca6bc51249b781b258e0fe26f7da`.
Commands, source/fork pins, toolchain/settings, binary/runtime hashes, logs and
emitted code are retained. Publishing a digest does not constitute independent
inspection of this contributor-local evidence.

The [private owner](../../docs/EXCHANGE-OWNER.md) borrows its harness and uses trusted
initial mappings/layout associations. Host preparation still allocates/copies
projection, command, map and hierarchy data; these surfaces are outside the native
scratch manifest. Reusable allocation-free preparation, complete owner resources,
remaining-operation integration and controls remain subsequent work. Unrelated
generation changes make the owner stale. Full 115-suite regression, remaining
compiler modes, PostgreSQL and GPU were not rerun. Shared-host timing is not
performance evidence; hosted CI remains authoritative for its scope.

## Local reusable-projection qualification — 2026-11-05

Tested code: `3ba9dd84a99c6d30dd67627dd4ef5fabe83cbeb1`, matching Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, LLVM/Clang 15, actual native CPU
in WSL Ubuntu. This contribution is stacked on provisionally accepted #222 head
`c8461b52101c658839d1a79badba0f9f9fdde60c`, following
[review 5790946060](https://github.com/Human-Cognome-Project/human-cognome-project/pull/222#pullrequestreview-5790946060).
#221 remains its parent dependency; their hosted validation/final integration are
pending at this check. No parent branch or running hosted suite was modified.

Build `live_hierarchy_test`, `letter_projection_test`, `membership_fields_test`
and `structural_recipe_test` with `cmake --build engine/build --target ... -j 2`;
source `engine/build/engine-env.sh`. Recorded commands use `nice -n 10`:

- Bulk/default one worker: `live_hierarchy_test --threads 1 --case exchange_owner`
  and `--case exchange_owner_multiple` passed. Each runs 32 exchanges and following
  ticks; the single-occurrence witness also checks late unready publication failure.
- Direct/no-unroll two workers: `live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement`
  passed all **22 combined cases**. Audits remain **19 kernels/29 unsafe controls
  per 52/8-bit profile**; final-source emitted LLVM is retained via `HCP_B2_IR_DIR`.
- CTest passed all three projection, membership and structural-recipe tests.

First and repeated projection staging use the two views reserved at adoption,
without observed C++ `new`. Active and spare parent objects retain their nested
UUID, address and occurrence-path storage. Both view addresses and total projection
capacity charges remain stable through exchanges and rejection/retry. The new
four-copy witness edits two occurrences in succession while checking previously
edited and untouched copies, survivor numerical state and full native handles,
shared fields/focus and exact contribution inventory. Permanent copies keep both
field alternatives live; the original single-copy witness still exercises field
creation/retirement. Successful execution, ordinary rejection and ticks also pass
the C++ allocation guards, with stable native backing. Other preparation and
exception construction remain excluded.

| Fixture | Retained views | Constructed parent rows, active + spare | Capacity charge |
|---|---:|---:|---:|
| One logical occurrence | 2 | 10 | 54,556 bytes |
| Four logical occurrences | 2 | 40 | 86,264 bytes |

These charges cover both projection objects and their owned vector/text capacities,
including unused vector slots and nested object addresses. Inline strings are
conservatively charged again; this is not heap/RSS/driver measurement or a setup
peak. Tests verify overflow leaves the report unchanged and growing only one
object-address reservation changes the charge by exactly its added element capacity.

Initial `65d4d682` passed both focused owner sequences and the value tests, but
inspection found that its census omitted object-address vectors. Its combined run
was stopped. Final source adds these buffers and the targeted capacity-delta test.
Earlier logs and incomplete totals are retained separately and are not final-source
complete accounting or combined passing evidence.

Raw bundle `/root/hcp-workspace/evidence/reusable-projection-3ba9dd84`; Windows
archive `setup-evidence/reusable-projection-3ba9dd84.tar.gz`. SHA256SUMS digest:
`1a21329a9648440b84fd0d456a6767e592065e3d71896b214fd6d8a06228ba81`.
Commands, source/fork pins, toolchain/settings, binary/runtime hashes, logs and
emitted code are retained. This is contributor-local evidence; publishing its
digest is not independent inspection of the raw bundle.

[Scope](../../docs/EXCHANGE-OWNER.md): reusable projection staging and its retained
capacity census only. Declaration/binding/coordinator/map/hierarchy preparation
still allocates. Complete owner resources and lifetime/peak accounting, reusable
transaction preparation, remaining operations and controls remain subsequent work.
Full 115-suite regression, other compiler modes, PostgreSQL and GPU were not rerun.
Shared-host timing is not performance evidence; hosted CI remains authoritative
for its scope.

## PR #223 hosted timeout correction

[Review 5793702801](https://github.com/Human-Cognome-Project/human-cognome-project/pull/223#pullrequestreview-5793702801)
provisionally accepts reviewed head `2ad8151f` with no blocking implementation
finding. Hosted run 42122775107 is not passing: job 127581880062 timed out
`placement_2_bulk` at 1200.23 seconds and finished 28/29 registered tests.
Direct inspection of that job log confirms 21 of 22 placement cases passed,
including both owner witnesses, with no case-failure lines before the final
admission-contact remap case was killed. The other three regression shards, four
compiler jobs, build, projection values and database job were green per the review;
that does not override the failed aggregate gate.

Correction `f080e8894f9bb501913d7aa29bee4c02ba50c171` changes only the four
`placement_{1,2}_{bulk,direct}` CTest limits from 1200 to **2400 seconds**. All
115 registered test names, commands, indexes and other properties remain identical.
Every existing placement case and compiler/backend configuration is retained;
there is no test split or shard-index change. Hosted job limits remain unchanged.

Affected-gate validation on actual local CPU, using the existing matching
Taichi/LLVM15 build:

- Compared before/after CTest JSON inventories: exactly four placement timeouts
  changed; all names, commands, remaining properties and non-placement timeouts
  are unchanged. The inventory still contains 115 tests.
- `bash engine/scripts/check_hosted_cpu_ci.sh` passed its real shard partition,
  failure propagation, inventory and bundle-provenance checks.
- `ctest --test-dir engine/build -j 1 -R '^placement_2_bulk$' --no-tests=error --output-on-failure`
  passed in **734.49 seconds**, all **22 internal cases** and both
  **19-kernel/29-control** compiler-audit profiles included. The registered CTest
  timeout was used, with no command-line timeout override.
- Executable SHA-256 matches both its pre-configuration hash and the previously
  qualified `3ba9dd84` feature binary. No engine implementation was rebuilt or changed.

The local orchestration initially stopped because `jq` was absent, before starting
any native test. Installed `jq` 1.7.1 and resumed against the preserved inventories.
This was a resolved local dependency, not an assertion failure or approval block.

Raw evidence `/root/hcp-workspace/evidence/placement-budget-f080e889`; Windows
archive `setup-evidence/placement-budget-f080e889.tar.gz`. SHA256SUMS digest:
`903ba64ffabfd53b71a80061fb9431769cfca381cfc882263d61a279ff68d978`.
It retains source/fork pins, before/after inventories and hashes, CI-contract output,
CTest/JUnit/full test logs, command scripts and a labelled hosted-log excerpt.
The final documentation commit also corrects a CMake comment; executable settings
are the tested settings. These are contributor-local results, not independent
inspection or a corrected hosted pass. Other tests/modes, PostgreSQL and GPU were
not rerun; prior feature evidence and emitted LLVM remain separately pinned.
Local elapsed time is not a hosted performance prediction.

Dependency reconciliation from the review: #221 run 42086483712 is green; #222 run
42094921159 was **cancelled**, not pending or passing, after approximately 51 minutes
of package installation and the 90-minute job limit. Its aggregate gate failed.
No parent rerun or branch change was requested. Final parent integration and final
#223 acceptance require maintainer reconciliation and the corrected hosted gate.

<a id="maintainer-integrated-acceptance--221223-2026-11-06"></a>

## Maintainer integrated acceptance — #221–#223, 2026-11-06

Accepted integrated source: `2286950ae07180c5a23b6d0de38792f064a570f5`.
[Hosted run 42134552198](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42134552198)
and smoke run 42134552247 passed. The maintainer inspected all four regression
job logs: 115 distinct test indexes, 1–115, each passed exactly once.
All four compiler jobs (one/two workers, bulk/direct, including their no-unroll
checks), build, projection-values and aggregate gate passed.
The database log records 180 integration checks, 24 addressed reads and zero
private reads. The four placement registrations passed: one-worker bulk
994.37 s, one-worker direct 969.40 s, two-worker bulk 1150.36 s and
two-worker direct 1176.38 s.

The reviewed correction changes only the four placement CTest timeouts to
2400 seconds; test names, commands, indexes and coverage are retained.
This clears the earlier #223 timeout blocker. Run 42122775107 remains a failed
historical run. #221's run 42086483712 is green; #222's run 42094921159 remains
cancelled/incomplete. Acceptance of the combined #222 owner behavior rests on
the successful descendant run above, not on relabelling that cancelled run.
The accepted stack includes #221 field lifecycle, #222 persistent exchange
identity/projection and #223 reserved projection reuse/capacity census.

Contributor-local bundles remain separately attributed and were not independently
executed or inspected in this acceptance. The capacity census is a conservative
retained-capacity charge, not complete owner memory accounting or physical RSS.
Reusable declaration/binding/coordinator/map/hierarchy preparation, complete
peak/lifetime resources, admission/removal/focus/rebase/restart owner integration
and same-execution controls remain open. Actual GPU qualification remains separate.
Continue from current main; no baseline rerun is required.
# Contributor-local coordinator backing reuse — 2026-11-06

Tested source `43ae245cfebe7301fac1a9e8a9a165122afdf16a`, based on accepted
main `e69adbaa`. Final contribution documentation is separate from this source
pin. Taichi remains `5ba579a8a11255b7a258cde93801070956d81d57`.

Actual CPU, native Clang/LLVM 15, WSL Ubuntu. Commands after sourcing
`engine/build/engine-env.sh`:

```sh
cmake --build engine/build --target live_hierarchy_test -j 2
engine/build/live_hierarchy_test --threads 1 --case exchange_owner
engine/build/live_hierarchy_test --threads 1 --case exchange_owner_multiple
HCP_B2_IR_DIR=<retained-llvm-directory> engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement
```

Both one-worker bulk/default owner cases passed, each exercising 32 successful
exchanges and following ticks. The two-worker direct/no-unroll combined matrix
passed all 22 cases. Both 52/8-bit placement compiler audits passed: 19 kernels,
29 unsafe controls each. Runtime logs identify the CPU backend. Executables were
run at `nice -n 10`; this is functional evidence, not a performance comparison.

The owner tests check one persistent coordinator and stable addresses/capacities
for command, intrinsic and all three hierarchy buffers across repeated edits.
First and repeated hierarchy sealing run under the C++ allocation guard. Partial
preparation failure invalidates earlier pending intent, preserves committed
projection/maps/reports and permits valid preparation/retry. Existing field
creation/retirement, shared-field survival, native publication, rejected execution,
late-unready and tick checks remain active. Caller hierarchy construction is
outside that guard, as are declaration/binding/nested-map preparation.

Initial source `b94fd650` passed the single-owner witness but its multi-owner
guard also covered allocating caller `RefreshRequest::spec()` construction.
The final source moves that construction outside owner sealing; the owner guard
remains in place from its first call. The initial failed log is retained and is
not final-source passing evidence.

Retained local bundle: `exchange-preparation-43ae245c`, including source/submodule
pins, toolchain/configuration, binary hashes, logs, emitted LLVM and prior attempt.
`SHA256SUMS` digest:
`b59c6f52d66e6d754e716596aae7ea2b250387d2701cf6daf7b564482483e181`.
The archive is local; the hash does not make its raw contents remotely available.
These are contributor-executed results, not independent maintainer or hosted
validation. Full 115-test baseline, other compiler configurations, database and
GPU suites were not rerun. Whole-owner allocation-free preparation, complete
peak/lifetime accounting and other-operation integration remain open. The
projection census does not include the newly retained coordinator buffers.
# Contributor-local retained exchange declarations — 2026-11-06

Tested source `25187e911bdb2590a787f7a47b5b2d9096771d25`, stacked on provisionally
accepted #224 head `4d0a6efdfaed99a1c7b6765c3dbefa1728f5cce1` under
[review 5801533371](https://github.com/Human-Cognome-Project/human-cognome-project/pull/224#pullrequestreview-5801533371).
Final contribution documentation is separate. Taichi remains
`5ba579a8a11255b7a258cde93801070956d81d57`.

Actual native CPU, Clang/LLVM 15, WSL Ubuntu. After sourcing
`engine/build/engine-env.sh`, built `live_hierarchy_test` with two build workers
and ran at `nice -n 10`:

```sh
engine/build/live_hierarchy_test --threads 1 --case exchange_owner
engine/build/live_hierarchy_test --threads 1 --case exchange_owner_multiple
HCP_B2_IR_DIR=<retained-llvm-directory> engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement
```

Both bulk/default one-worker cases passed with 32 exchanges and following ticks
each. All 22 direct/no-unroll two-worker combined placement cases passed. Both
52/8-bit compiler audits passed, with 19 kernels and 29 unsafe controls each.
These results qualify the changed path, not the entire 115-test baseline.

The tests retain the original two/eight declaration identities, compare both
directions against fresh derivation (full source descriptors, ordered leaves,
exact mass, exposure and comparison work), guard successful selection against
C++ allocation, and verify that the binding holds the same declaration object.
The four-copy case includes mixed initial exposure and alternating edits while
preserving other occurrences/shared fields. Foreign/unknown/nested owner keys,
current exposure and invalid target requests reject; pending intent is invalidated,
committed state remains unchanged and valid retry succeeds. Previous projection,
coordinator backing, sealing, native publication, rejection and tick guards remain.

Initial `4973768b` failed test compilation because the foreign-study witness used
a string operation on the UUID value type. `25187e91` uses `warm::new_uuid()`;
no native test ran from the failed build. Its build log remains in the bundle.

Local bundle `exchange-declarations-25187e91` retains source/submodule pins,
toolchain/configuration, binary hashes, logs and emitted LLVM. `SHA256SUMS` digest:
`eaf53effaa7bada9d5c993b2a24151514c4f0267ac5a297baf044ecac6d269cc`.
This is contributor-executed local evidence, not independent maintainer or hosted
execution. The raw bundle remains local; its digest is provenance only.

Full 115-test baseline, other compiler configurations, database and GPU suites
were not rerun. Binding/membership/nested-map preparation still allocates. Retained
declaration descriptors/table/control blocks are excluded from the projection
census; complete owner peak/lifetime resources, other operations and controls
remain open. Parent #224's native run 42232659936 is still in progress; its smoke
run passed. No duplicate parent run was requested or started.
# Contributor-local binding capture reuse — 2026-11-06

Tested source `340cf999c74fe6bc57219222f3c566a34c7a5a62`, stacked on provisionally
accepted #225 head `9bb9ab71185881aa7f8c31b1056fb7443e8edcf5` and #224 head
`4d0a6efdfaed99a1c7b6765c3dbefa1728f5cce1`, under
[review 5802233579](https://github.com/Human-Cognome-Project/human-cognome-project/pull/225#pullrequestreview-5802233579).
Final contribution documentation is separate. Taichi remains
`5ba579a8a11255b7a258cde93801070956d81d57`.

Actual native CPU, Clang/LLVM 15, WSL Ubuntu. Built `live_hierarchy_test` with
two build workers; after sourcing `engine/build/engine-env.sh`, ran at `nice -n 10`:

```sh
engine/build/live_hierarchy_test --threads 1 --case exchange_owner
engine/build/live_hierarchy_test --threads 1 --case exchange_owner_multiple
HCP_B2_IR_DIR=<retained-llvm-directory> engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement
```

Both bulk/default one-worker sequences passed, with 32 exchanges and following
ticks each. All 22 combined direct/no-unroll two-worker cases passed. Both 52/8-bit
compiler audits passed, with 19 kernels and 29 unsafe controls each.

The tests preserve the original binding identity and three ordinary vector
addresses/capacities plus both packed-mask capacities across first/repeated edits,
including alternating edits across four copies. The newly adopted binding is
invalid until capture. A private test adapter introduces an invalid field handle
after mass capture, checks that validation rejects the partial view and that the
committed owner remains unchanged, then performs a successful retry with the same
backing. Existing source-binding negative cases, publication, field lifecycle,
survivor/retry, declaration, coordinator/projection and tick checks remain active.

Local bundle `exchange-bindings-340cf999` retains source/submodule pins, toolchain,
configuration, binary hashes, logs and emitted LLVM. `SHA256SUMS` digest:
`f5c26fd0beb8d733b9c11af8e077388932d5c24a1c1c78be988aa9fe2d040d87`.
This is contributor-executed local evidence; raw contents remain local and are
not independently inspected maintainer evidence. No performance claim follows.
These are buffer-reuse checks, not allocation-free whole-binding preparation or
measurement of packed-mask addresses. Membership, nested maps and temporary
binding values still allocate. Complete owner peak/lifetime accounting, remaining
operations and controls remain open. Full 115-test baseline, other compiler
configurations, database and GPU suites were not rerun. No GPU was used.

Parent-status reconciliation at publication: #224 run 42232659936 is cancelled,
not pending. Job [127963277696](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42232659936/job/127963277696)
was cancelled during dependency installation; inspected logs show stalled/retried
Ubuntu toolchain downloads from 14:59 to 15:30 UTC, with a configured 30-minute
database job budget. Restore and integration steps were skipped, so this is
incomplete database evidence, not an assertion failure or a pass. The job summary
reports build/projection, all four regression and all four compiler jobs successful;
the aggregate gate failed because database did not pass. #225 native run
42239847643 remains in progress. Both smoke runs passed. No parent rerun was started
and no dependency branch or CI budget was changed by this contribution.
