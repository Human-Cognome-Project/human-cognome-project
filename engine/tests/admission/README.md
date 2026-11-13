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
# Contributor-local membership reuse and source capture — 2026-11-06

Tested source `2fef2829e7b6a1fe2259e93883c0fa7119a81f40`, stacked on provisionally
accepted #226 head `67eb5f1ff1f17f3b8b123bc3cd3d616f5f2ba9f5` and its #225/#224
dependencies, under [review 5802947489](https://github.com/Human-Cognome-Project/human-cognome-project/pull/226#pullrequestreview-5802947489).
Final contribution documentation is separate. Taichi remains
`5ba579a8a11255b7a258cde93801070956d81d57`.

Actual native CPU, Clang/LLVM 15, WSL Ubuntu. Built `live_hierarchy_test` and
`membership_fields_test` with two build workers. After sourcing
`engine/build/engine-env.sh`, ran:

```sh
ctest --test-dir engine/build --output-on-failure -R '^membership_fields_test$'
engine/build/live_hierarchy_test --threads 1 --case exchange_owner
engine/build/live_hierarchy_test --threads 1 --case exchange_owner_multiple
HCP_B2_IR_DIR=<retained-llvm-directory> engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement
```

The native executables ran at `nice -n 10`. All 61 membership value checks passed,
including 32 exposure combinations comparing an immutable all-alternative catalogue
with fresh live derivation. Cases include repeated copies, language-context
separation with shared byte definitions, shared ancestor paths, multibyte/repeated
characters, exact source paths, group pins and counts. Ordinary live binding of a
catalogue rejects. Existing public live-membership and graph rejection checks pass.

Both bulk/default one-worker owner sequences passed, with 32 exchanges and following
ticks each. All 22 combined direct/no-unroll two-worker placement cases passed.
Both 52/8-bit compiler audits passed, with 19 kernels and 29 unsafe controls each.
Private adapters stage the production maps/metadata separately, then guard first
and repeated successful source-capture calls against C++ allocation. Owner and
binding retain the same catalogue; existing binding/projection/coordinator backing,
partial-failure/retry, shared-field/survivor, native publication and tick checks remain.
Nested map staging and caller preparation are outside the capture allocation guard.

Initial `435bdfe8` omitted explicit focus in the new structural value fixture.
`68ce6b79` added focus but changed selection flags without regenerating matching
structural floors. Both attempts failed value tests before native owner tests began.
Final `2fef2829` normalizes each independent comparison view with explicit focus;
production implementation is unchanged across these test corrections. Both failed
logs are retained separately from final-source evidence.

Local bundle `exchange-memberships-2fef2829` contains source/submodule pins,
toolchain/configuration, binary hashes, complete value/native logs and emitted LLVM.
`SHA256SUMS` digest:
`bc84f7cc82708684c428e3ce06f78be70a0a302d573588440f61eb14977e3b88`.
Raw contents remain local; this is contributor-executed evidence, not independently
inspected maintainer evidence or a performance qualification. Full 115-test baseline,
other compiler configurations, database and GPU suites were not rerun. No GPU used.

Catalogue construction and graph temporaries now occur at adoption. Catalogue rows,
nested keys/pins and shared ownership remain outside the projection census. Nested
maps still allocate; allocation-free whole-owner preparation, complete peak/lifetime
and work accounting, remaining operations and controls remain open.

Parent status: review 5802947489 records #225 run 42239847643, including database,
as successful, validating the combined #224/#225 code. #224 run 42232659936 remains
cancelled/incomplete historical evidence; it is not relabelled passing. #226 run
42245987431 is still in progress and smoke passed. All three parents remain
unmerged at publication; no duplicate parent run was started.

<a id="maintainer-integrated-acceptance--224227-2026-11-07"></a>
## Maintainer integrated acceptance — #224–#227, 2026-11-07

Accepted stack heads: #224 `4d0a6efdfaed99a1c7b6765c3dbefa1728f5cce1`,
#225 `9bb9ab71185881aa7f8c31b1056fb7443e8edcf5`,
#226 `67eb5f1ff1f17f3b8b123bc3cd3d616f5f2ba9f5`, and
#227 `b9ea490f86333ba1bd572abd74a202fe19c7534a`.
The final head descends from integrated main `e69adbaa7d4afbfbff17035167588492a169da51`.
#227 tested local source `2fef2829e7b6a1fe2259e93883c0fa7119a81f40`
differs from its reviewed head only in six documentation files.

[Native run 42252528197](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42252528197)
and [smoke 42252525837](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42252525837) passed at the final head.
All 12 native jobs report success. Maintainer-inspected regression logs
128015190991, 128015190953, 128015191278 and 128015190994 cover all
115 test numbers exactly once. All four compiler jobs passed.
[Database job 128015191019](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42252528197/job/128015191019)
records 180 warm-to-active integration checks, addressed_reads=24 and private_reads=0.
#226 [native run 42245987431](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42245987431) also completed successfully;
#225 [native run 42239847643](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42239847643) was already verified successful.

#224 [run 42232659936](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42232659936) remains cancelled/incomplete.
[Database job 127963277696](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42232659936/job/127963277696)
stalled during Ubuntu LLVM dependency downloads and exhausted its 30-minute budget
before database restore/tests. Other native test jobs and smoke passed, but its
aggregate gate did not. Successful descendant runs validate the combined source;
they do not retroactively turn this cancelled attempt into a pass. No duplicate
baseline or parent rerun was requested.

Source review accepts retained coordinator buffers (#224), immutable declarations
(#225), reusable binding/source buffers with invalid-before-capture semantics (#226),
and adoption-retained membership alternatives plus source capture reuse (#227).
The catalogue preserves exact occurrence identity, pinned targets and contribution
counts; only selected outgoing/incoming occurrences participate. Alternative
catalogues reject both public live binding methods. Native handle, source mass,
participation and generation checks remain fresh. First/repeated no-allocation
guards apply to the scoped capture phase after map staging, not entire preparation.
The 32-combination value fixture and repeated exchange/rejection/retry tests remain
part of the reviewed source. Contributor-local bundle hashes and initial fixture
failures above are preserved as contributor evidence, not independently executed
maintainer tests.

Integration preserves all stack ancestry and changes only current-status and
acceptance documentation beyond the tested head. Next: nested map staging reuse,
complete owner peak/lifetime/work accounting, admission/removal/focus/rebase/restart
integration, then same-execution controls. Catalogue construction temporaries,
nested keys/pins, coordinator maps and caller preparation remain outside the scoped
projection census. The harness is borrowed; no full resource, performance, public
operation or GPU qualification follows from this acceptance.

## Contributor-local nested map reuse — 2026-11-07

Tested source: `73e41847d91c3df1a17cd85bae896dff2e70e266`, based on accepted
main `8f88635b5e9b2f84e603abe1e7890b0d601bea95`. The submitted head adds only
status/scope/evidence documentation. Actual CPU, WSL Ubuntu, native Clang/LLVM 15;
Taichi pin `5ba579a8a11255b7a258cde93801070956d81d57`. No GPU was used.

Commands (from the Linux checkout, after sourcing `engine/build/engine-env.sh`):

```sh
cmake --build engine/build --target live_hierarchy_test membership_fields_test -j 2
ctest --test-dir engine/build --output-on-failure -R '^membership_fields_test$'
nice -n 10 engine/build/live_hierarchy_test --threads 1 --case exchange_owner
nice -n 10 engine/build/live_hierarchy_test --threads 1 --case exchange_owner_multiple
nice -n 10 engine/build/live_hierarchy_test --threads 1 --case exchange_commit
nice -n 10 engine/build/live_hierarchy_test --threads 1 --case exchange_fields
HCP_B2_IR_DIR=/root/hcp-workspace/logs/exchange-maps-ir nice -n 10 \
  engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case placement
```

All passed: 61 membership value checks; both bulk/default one-worker owner sequences
with 32 exchanges/ticks each; focused commit and field lifecycle suites; and all
22 combined direct/no-unroll two-worker cases. Both 52/8-bit compiler audits passed,
with 19 kernels and 29 unsafe controls each. The capacity-focused pair ran in an
independent process concurrently with the main qualifier after its build completed;
these are functional checks, with no timing/performance interpretation.

First and repeated successful owner preparation, including valid retries, are
covered by C++ `new` guards. Separate map-staging/capture, hierarchy sealing,
execution and tick guards remain. The complete pool of six outer map buffers and
all constructed path/address buffers retains its addresses/capacities across field
retirement and publication swaps. Tests cover erasure order, full slots, oversized
keys, checked reservation overflow, copied empty-slot capacity and retry. An
oversized field key cancels pending intent without changing committed owner state
or backing. Existing independent-copy, shared-field, survivor, source-validation,
ordinary-rejection and late-unready witnesses remain unchanged.

The initial `68698240` owner and four-copy sequences passed; its combined run was
stopped intentionally while adding reservation-copy corrections/backing witnesses.
`2ce859f9` built and passed values; its owner run was stopped when trusted initial
field-key bounds were added. Neither interrupted suite is counted as completed.
`044883ab` passed values and focused owners but failed combined `exchange_commit`
and `exchange_fields`: host candidate maps were incorrectly bounded by native live
capacity, preventing native peak-exhaustion witnesses from reaching preflight.
That combined run was stopped after the failures were confirmed. The final code
reserves checked `Particles + Batch` and `Fields + min(Fields, Batch)` candidate
metadata in all three stores, preserving the unchanged native `CapacityExhausted`
results and live claim limits. No expected result was weakened. Failed/interrupted
logs are retained separately from the complete final-source run.

Local bundle `exchange-maps-73e41847` contains source/submodule pins, configuration,
toolchain and binary hashes, complete logs, commands and final emitted LLVM.
`SHA256SUMS` digest:
`74e6134fa48c973f57620eb0a7c79f870e5cd66f57d2226a334c1d3260b93084`.
Raw contents remain local contributor evidence, not independently inspected
maintainer evidence. Full 115-test baseline, other compiler configurations,
database and GPU suites were not rerun. Hosted checks remain authoritative.

Adoption, standalone adapter construction, caller inputs/hierarchy and exception
construction may allocate. Constructed spare map slots, UUID/path/address storage
and field-command capacity are outside `projection_storage()`. Complete owner
peak/lifetime/work accounting and byte-budget enforcement remain next, followed by
remaining-operation integration and same-execution controls. This is nested-map
reuse qualification, not complete owner resource, performance or GPU acceptance.

## Combined owner retained-capacity census — 2026-11-07

Contribution on explicit provisional #228/#230 dependencies. Tested code:
9bff8d55c454223b21f3eb02e05b9c017bbcea5b. Main documentation clarification
7f938878c8178e27b582069847c84d713c18de95 is retained by the separate integration
merge, including analyst conceptual design and post-PoC documentation factoring.

Native Clang/LLVM 15 CPU build of live_hierarchy_test passed. Taichi pin:
5ba579a8a11255b7a258cde93801070956d81d57. The focused commands were
live_hierarchy_test --backend cpu --threads 1|2 --case exchange_owner
and --case exchange_owner_multiple, with the default bulk path and with
--direct --compiler-no-unroll. All eight runs passed, case_failures=0.

Both sequences exercise 32 exchanges/ticks. Added checks cover allocation-free
combined reporting, all retained component families, inactive nested-map and
field-command capacity deltas, aggregate overflow rejection, shared allocation
counts and stable totals across publication. Existing numerical, survivor,
rejection/retry and late-failure witnesses remain unchanged.

The report counts retained payload capacity. It does not measure setup peak,
allocator/control-block bytes or reclamation, and does not enforce a complete
byte/preparation-work budget. The [resource inventory](../../docs/OWNER-RESOURCES.md)
records those remaining gates, borrowed surfaces and constructor/cleanup paths.

Source-pinned logs, build output, commands, configuration and binary/toolchain
identities are retained locally as owner-storage-9bff8d55. SHA256SUMS verified;
its SHA-256 is:

    d36466089ecb2f0bc333b6aca1f4d65cdce5267108a49d75dc956f7d4a8faf19

The final head adds documentation only after tested code. Raw evidence remains
contributor-local; its hash does not make it remotely available. No full regression,
compiler-audit matrix, database, GPU or performance rerun is claimed for this
inspection-only change. #230's full baseline evidence retains its own source pin;
hosted native run 42377616217 was still running and smoke 42377616279 passed at
this check. No duplicate baseline run was started.

## Owner adoption peak and failure reclamation — 2026-11-07

Test-only continuation on provisional #228/#230/#231. Tested source:
ad7f8552baa20fbc2ba15ef70a36a116514be3bd. Native Clang/LLVM 15 CPU;
Taichi 5ba579a8a11255b7a258cde93801070956d81d57. Production source unchanged.

The focused live_hierarchy_test build and all eight runs passed:
--backend cpu --threads 1|2 --case exchange_owner|exchange_owner_multiple,
using bulk/default and --direct --compiler-no-unroll. Each fixture's successful
adoption trace was followed by a failing attempt at every allocation ordinal,
full tracked reclamation after each failure, a successful retry and exact native
snapshot comparison. Existing 32-exchange/tick sequences and oracles passed.

| Fixture | Allocation/failure sites per run | Retained requested bytes | Peak requested bytes | Retained capacity charge | Maximum failed-attempt peak |
|---|---:|---:|---:|---:|---:|
| One occurrence | 4,192 | 276,329 | 276,874 | 277,017 | 276,859 |
| Four occurrences | 7,856 | 331,491 | 333,533 | 332,083 | 333,518 |

All four configurations produced those figures. Zero tracked blocks/bytes
remained after final alias release, each failure unwind and the successful retry.
Observer controls checked exact bytes, scalar/array/aligned/sized-delete paths,
pre-existing deallocation exclusion and injected failure.

These are current-thread requested C++ allocation bytes around adoption, including
by-value argument copies and the standard library's shared allocations. They are
not allocator metadata, stack, RSS, driver/GPU or combined process peaks. The
capacity charge can be above or below the observed peak; neither substitutes for
the other. Detailed scope and outstanding budget enforcement are in
[OWNER-RESOURCES.md](../../docs/OWNER-RESOURCES.md).

Raw local evidence: owner-lifetime-ad7f8552, including build, commands, all logs,
configuration, toolchain and binary identities. SHA256SUMS verified; its SHA-256:

    6976596b8186d62447e2b7826ffd1f805e4ae59d404706616bbb41fbfe41724f

Final head differs from tested code only in documentation. Raw evidence remains
contributor-local, not independently hosted review evidence. No full regression,
compiler-audit matrix, database, GPU or performance rerun is claimed. Parent #231
smoke 42378741344 passed and native 42378741420 remained in progress at the check;
no duplicate parent baseline was started. Complete byte/preparation-work budget
enforcement and broader resource qualification remain next.

## Explicit hierarchy-sealing budgets — 2026-11-07

Tested source 6689c292dd3b3f40ddbe986a2e83b2ee2d75d74a, on provisional
#228/#230/#231/#232 dependencies. Native Clang/LLVM15 CPU build passed;
Taichi pin 5ba579a8a11255b7a258cde93801070956d81d57.

All sixteen focused runs passed: exchange_owner, exchange_owner_multiple,
exchange_commit and exchange_fields, each with --backend cpu --threads 1|2,
bulk/default or --direct --compiler-no-unroll. New owner checks cover separate
one-unit byte/work shortages before unreadable caller pointers are accessed,
overflow with an empty report, unchanged pending/native/backing state and retry
with exact allowances. Existing owner allocation-failure/reclamation,
32-exchange/tick, numerical and standalone field-lifecycle witnesses passed.

The byte charge covers caller table spans plus reserved destination capacities.
Copy work counts input payload bytes. Neither is a whole-owner/process budget;
adoption allocations, other preparation work, caller slack and native/driver
resources remain outside this gate. See [resource scope](../../docs/OWNER-RESOURCES.md#hierarchy-sealing-gate-contribution-awaiting-review).
Native hierarchy validation/work limits remain unchanged.

Raw local evidence hierarchy-budget-6689c292 includes build, commands, all
sixteen logs, configuration, toolchain and binary identities. SHA256SUMS verified;
its SHA-256 is:

    8d775f769d35ff9ead6662bd9f2afe5ddf68e88c9e5c13eb9d0cf2e8399113fe

A shared Taichi cache-lock warning occurred after the one-worker bulk commit
suite passed; that process exited successfully and the runner completed all
remaining cases. The warning remains in the raw logs. No timing claim follows
from these concurrent processes on a shared host.

Final head changes documentation only beyond tested code. Raw logs remain
contributor-local, not independently inspected maintainer evidence. No full
115-test baseline, complete compiler-audit matrix, database, GPU or performance
rerun is claimed. Parent #232 smoke passed and native remained running at the
last check; no duplicate baseline was started. Complete combined byte and
remaining preparation-work enforcement remain next.

## Incremental source-capture work gate — 2026-11-07

Tested source eff2bb538901858817a45d2b7b87d4746362a7e1 on provisional #228/#230/#231/#232/#233.
Native Clang/LLVM15 CPU build passed; Taichi pin
5ba579a8a11255b7a258cde93801070956d81d57. All twenty focused runs passed:
exchange_owner, exchange_owner_multiple, exchange_bindings, exchange_commit,
exchange_fields × --threads 1|2 × bulk/default or --direct --compiler-no-unroll,
all with --backend cpu.

The first single-occurrence exchange traversed 235 successive capture charge
boundaries, reaching the exact 5,087-unit successful allowance in all four
configurations. Each exhausted attempt retained invalid binding/pending state
and made forward progress through next_required. Zero allowance, one-unit-short
late exhaustion and exact-charge retry were also checked across both directions,
with unchanged committed/native state and backing. Product/sum overflow witnesses
passed. Existing allocation-failure/reclamation, 32-exchange/tick, source validation,
commit/field-capacity and numerical witnesses passed without oracle changes.

Work is defined structural units, including upper-bound variable key bytes and
the existing final validation charge, not instructions or timing. This gate
covers capture only, after declaration/projection/map staging and before command
construction. Adoption/standalone construction explicitly retains the maximum
capture allowance pending separate setup-work admission. The internal stop token
is caught at capture; no C++ new was observed in tested exhaustion paths, but
exception-runtime allocation is outside that witness. See
[resource scope](../../docs/OWNER-RESOURCES.md#source-capture-work-gate-contribution-awaiting-review)
and the separate combined-domain/double-counting discussion.

Evidence capture-budget-eff2bb53 retains build, commands, twenty logs, toolchain,
configuration and binary identities locally. SHA256SUMS verified; its SHA-256:

    bc77c79ca282e4ae815c5c9fd83af9949ac764c122173bafb4e85e404ee072ba

Shared Taichi cache-lock warnings after passing one-worker bulk commit/field
suites remain in the raw logs; all processes exited successfully. No performance
claim follows from concurrent checks on the shared host. Final head differs from
tested code only in documentation. Raw evidence remains contributor-local.

No full 115-test baseline, complete compiler-audit matrix, database or GPU rerun
is claimed. Parent #233 smoke passed and native remained running at the last
check; no duplicate baseline was started. Complete adoption-byte/setup-work,
remaining preparation gates and combined caller/borrowed resource policy remain
outstanding before full owner-resource qualification.

## Separate command-construction work gate — 2026-11-07

Tested source 749b769dbded7f98d0b93f16625357c377d21cf4, on provisional
#228/#230/#231/#232/#233/#234 dependencies. The shared preparation-meter extraction
is a separate mechanical commit before command enforcement. Native Clang/LLVM15
CPU build passed; Taichi pin 5ba579a8a11255b7a258cde93801070956d81d57.

All twenty focused runs passed: exchange_owner, exchange_owner_multiple,
exchange_bindings, exchange_commit and exchange_fields × --threads 1|2 ×
bulk/default or --direct --compiler-no-unroll, all with --backend cpu.
The first exchange checked 645 successive command-charge boundaries and an exact
12,015-unit command allowance in all four configurations, separately from capture.
Zero/late-short allowances, phase results, consumed/unsealed partial candidates,
unchanged owner/native/backing state and exact two-allowance retry passed.
An independent nested key-copy charge check and existing capture overflow,
allocation-failure/lifetime, 32-exchange/tick and numerical witnesses also passed.

These are structural work charges, including ABI object sizes and variable key
payload, not instructions/time or memory reservations. Standalone coordinator
setup explicitly uses maximum allowance pending setup admission. Preceding
declaration/projection/map staging, adoption-byte/setup-work and the combined
caller/borrowed resource policy remain outstanding. Phase Complete means capture
and command preparation, not hierarchy sealing or native publication. Internal
stop-token exception-runtime costs remain outside C++ new observation.
See [resource scope](../../docs/OWNER-RESOURCES.md#command-construction-work-gate-contribution-awaiting-review).

Source-pinned evidence command-budget-749b769d retains build, commands, twenty
logs, toolchain/configuration and binary identities locally. SHA256SUMS verified;
its SHA-256 is:

    2dcd73e91ed5855eda6d4bc6a706ceaa924dfa6c64be856cc9bf91e2336b6c62

Shared Taichi cache-lock warnings remain in the raw logs, including after the
passing two-worker direct commit suite. All processes exited successfully. No
performance claim follows from concurrent shared-host execution.

Final head changes documentation only after tested code. Raw evidence remains
contributor-local. No full 115-test baseline, complete compiler-audit matrix,
database or GPU rerun is claimed. Parent #234 smoke passed and native remained
running at the last check; no duplicate baseline was started. Full owner-resource
qualification, remaining operations and broader #229 factoring remain open.

<a id="maintainer-integrated-acceptance--228235-2026-11-08"></a>

## Maintainer integrated acceptance — #228/#230–#235 (2026-11-08)

All seven reviewed contributions completed their hosted native and smoke gates successfully. This closes their provisional CI dependencies and integrates their original ancestry into main.

| PR | Reviewed head | Native run (passed) | Smoke run (passed) |
|---|---|---|---|
| [#228](https://github.com/Human-Cognome-Project/human-cognome-project/pull/228) | `4596c3535bba92efa533616f074d702ede85939c` | [42370220296](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42370220296) | [42370220331](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42370220331) |
| [#230](https://github.com/Human-Cognome-Project/human-cognome-project/pull/230) | `d0a57fbd08449a3ee6fc51a7e97f44348e31fea0` | [42377616217](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42377616217) | [42377616279](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42377616279) |
| [#231](https://github.com/Human-Cognome-Project/human-cognome-project/pull/231) | `cadc00dc8e45482c792fa52b91a006d256ab2faa` | [42378741420](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42378741420) | [42378741344](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42378741344) |
| [#232](https://github.com/Human-Cognome-Project/human-cognome-project/pull/232) | `d72acae0cefb5105f67e6903b1a91af661d2d299` | [42380990903](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42380990903) | [42380991092](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42380991092) |
| [#233](https://github.com/Human-Cognome-Project/human-cognome-project/pull/233) | `4e6af996bac8663240dafd2b58e6d78334696108` | [42383533878](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42383533878) | [42383533954](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42383533954) |
| [#234](https://github.com/Human-Cognome-Project/human-cognome-project/pull/234) | `ba7076f266a9b1312750a16fc38442e699dffe3d` | [42385728834](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42385728834) | [42385728835](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42385728835) |
| [#235](https://github.com/Human-Cognome-Project/human-cognome-project/pull/235) | `db3789ee35a7139be96cd3099ef6d3c5ea5a07c8` | [42388030237](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42388030237) | [42388030193](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42388030193) |

The integrated code source is #235 head `db3789ee35a7139be96cd3099ef6d3c5ea5a07c8`, tree `484913e4a819d03070ddd5b6109ea15d57c9a206`. It already includes main `7f938878c8178e27b582069847c84d713c18de95`, including the analyst architecture clarification. The integration adds status/handoff/evidence documentation only; accepted-baseline CI is not rerun for that reconciliation.

Independent inspection of native run 42388030237 found all 12 jobs successful. Regression job logs 128450516390, 128450516387, 128450516539 and 128450516375 cover all 115 numbered regression tests exactly once (29/29/29/28). Database job 128450516314 records 180 warm-to-active integration checks and `addressed_reads=24 private_reads=0`. All four compiler jobs passed; this statement records hosted job status, not a new independent audit of every compiler artifact.

Accepted scope:
- #228 retains all six nested map stores and witnesses allocation-free first/repeated owner preparation, including retry.
- #230 extracts numerical kernels mechanically; it completes the first slice of #229.
- #231 provides the combined retained-capacity census.
- #232 adds scoped adoption allocation-peak, allocation-failure and final-alias reclamation witnesses.
- #233 gates hierarchy sealing by payload bytes and copy work before table access.
- #234 gates source capture incrementally, retaining invalid partial candidates and safe retry.
- #235 gates command construction separately and reports capture/command charges and the stopping phase.

These are bounded CPU contributions. Retained capacity is not RSS or complete allocator accounting; the adoption witness does not measure all machine memory. Local evidence remains local evidence with its original source/configuration pins. Earlier failed, interrupted and cancelled attempts remain historical records, including the cancelled #224 path; they are not rewritten as successful runs.

The next coding agent starts from current main with declaration/projection/map staging gates, adoption-byte/setup-work limits and combined caller/borrowed resource policy and accounting. Charge before the guarded work/allocation, preserve ordinary rejection and exact retry, and count retained destinations once. Then continue admission/removal/focus/rebase/restart integration and same-execution controls. No full owner-resource, public-operation or integrated GPU qualification follows from this acceptance.

Keep #229 open for further factoring guided by the resource/lifetime inventory. Broad AGENTS restructuring remains deferred until after the engine PoC. Analyst functions remain conceptually designed in the overall architecture; their implementation structure and calibration await engine results on real data structures.

## Owner staging work allowance — contributor CPU evidence (2026-11-08)

Contribution awaiting review, based on integrated main b37c73ec. Tested code:
`b6321a4a5ffd6890192e38cba902d4ac91a8606d`; final documentation is separate.
The private owner adds staging work before capture and command construction,
covering retained declaration lookup, exposure/projection staging and initial
binding-map copies. Phase results distinguish all three allowances. Whole
parent-pool transfers are prepaid; rejected partial partitions remain retryable.

The focused native CPU matrix uses the unchanged Taichi pin
`5ba579a8a11255b7a258cde93801070956d81d57`, Clang/LLVM 15 and the CPU-only build,
with CUDA_VISIBLE_DEVICES=-1 on the shared personal host. Five cases run at
one/two workers using bulk/default and direct/compiler-no-unroll paths:
exchange_owner, exchange_owner_multiple, exchange_bindings, exchange_commit,
and exchange_fields. Final result and manifest digest are recorded below.

Both single-occurrence directions traverse every normalized staging charge
boundary: parts 246 boundaries / 7,848 units; aggregate 225 / 8,284 in all four
configurations. Every rejected attempt is followed by successful recovery.
Zero, late-short and exact allowances also cover repeated single/four-occurrence
edits; completed owner/native state and reserved backing remain unchanged.
Existing capture/command boundaries, numerical, lifetime/allocation-failure and
32-exchange/tick witnesses remain active. Work is structural and ABI/path-dependent,
not time or memory. The next-charge hint is attempt-local; successful recovery
can normalize the retained scratch order before an exact-charge replay.

Raw evidence is contributor-local in staging-budget-b6321a4a (build, commands,
case logs, toolchain, configuration and binary hashes). No full 115-test baseline,
complete compiler audit, database or GPU rerun is claimed. Hosted checks remain
authoritative for their scope. Adoption-byte/setup-work limits and combined
caller/borrowed accounting remain open, including exception-runtime costs outside
C++ new observation. This contribution does not qualify complete owner resources,
remaining operations or public controls.

Result: all 20 focused runs passed; all processes exited zero. SHA256SUMS verified;
its SHA-256 is:

    0ad3fb2fd10ae095974ad206eb478305a7f901e34d7a7a1dd094d4bc04677807

The local archive is setup-evidence/staging-budget-b6321a4a.tar.gz. Raw files stay
local; the committed digest identifies them and does not imply remote availability.

Main advanced during validation with documentation-only onboarding #236 at
c36901c289bba4b1545c537631c0c9a6e2904686. The contribution merges that history and
moves its status into CURRENT-STATE; engine source/tests still match the tested
code above. No duplicate native run was started for documentation integration.

## Adoption input snapshot admission — contributor CPU evidence (2026-11-08)

Contribution awaiting review on provisionally accepted #237 head
`84a7f9c10af98a88c5db5aa4cfb4334654111efc`. The
[#237 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/237#pullrequestreview-5818063830)
permits this continuation; native run 42484820959 remains pending at the recorded
check, smoke 42484821017 passed. No duplicate parent baseline was started. The
CURRENT-STATE opening summary, immediate task and next row are reconciled to that
disposition; integrated accepted code remains #235 until final acceptance/merge.

Tested code: `ebe0644aad7d2344594128f15949fe243a41c88e`. Later commits are documentation
only. The const-reference adoption factory admits caller/input-snapshot payload,
inspection work and copy work before owner allocation or by-value argument copies.
This is the input boundary only: later constructor allocations/setup and actual
allocator/native peak remain ungated by this allowance. Complete means admission,
not construction. See [scope and exclusions](../../docs/OWNER-RESOURCES.md#adoption-input-snapshot-admission-contribution-awaiting-review).

The CPU-only Clang/LLVM 15 build uses Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, CUDA_VISIBLE_DEVICES=-1, on the shared
personal host. The focused run comprises 53 projection checks plus twenty native
case runs: exchange_owner, exchange_owner_multiple, exchange_bindings,
exchange_commit and exchange_fields × workers 1/2 × bulk/default or
--direct --compiler-no-unroll. Final aggregate result and manifest follow below.

| Fixture | Inspection boundaries | Inspection work | Caller capacity charge | Snapshot payload/copy-work charge | Combined input bytes |
|---|---:|---:|---:|---:|---:|
| One selection | 1,161 | 1,905 | 27,367 | 25,890 | 53,257 |
| Four selections | 1,713 | 2,923 | 45,015 | 39,651 | 84,666 |

All four CPU configurations reproduce these input charges. Every inspection
boundary rejects through the actual factory under the no-C++-new guard. Zero/short
byte and copy allowances reject before allocation; exact allowances permit fresh
adoption with unchanged native state. Capacity-only outer/nested changes affect
caller charge, populated path and recipe-text changes affect snapshot size, and
checked arithmetic preserves accumulators on overflow. Inspection failure exposes
no partial byte report. The existing full allocation-failure traces remain 4,192
and 7,856 sites; final-alias reclamation returns to zero tracked blocks. Those
traces measure requested C++ allocation bytes separately from the input payload
charge and are not constrained by it.

Raw evidence stays local in adoption-input-ebe0644a (build, commands, projection
and native logs, toolchain/configuration and binary hashes). No full 115-test
baseline, complete compiler audit, database or GPU rerun is claimed. Source pins
and a local manifest identify contributor evidence, not independently hosted
results. Subsequent constructor-byte/setup-work gates, combined caller/borrowed
resources, exception-runtime/allocator overhead and remaining operations stay open.

Result: all twenty focused native runs and all 53 projection checks passed;
all processes exited zero. SHA256SUMS verified; its SHA-256 is:

    804a37628d0b2e54de00d9bfe4e6bf9f13e0d55c9f5f7360ca2f8ebc2a9ab886

The contributor-local archive is setup-evidence/adoption-input-ebe0644a.tar.gz.
The final source delta from the tested pin contains documentation only.

<a id="maintainer-integrated-acceptance--237238-2026-11-08"></a>

## Maintainer integrated acceptance — #237/#238 (2026-11-08)

#237 staging work and #238 adoption input-snapshot admission are accepted and integrated with their original ancestry. Their prior provisional and contributor records above retain their original dates and scope.

| PR | Reviewed source | Native run (passed) | Smoke run (passed) |
|---|---|---|---|
| #237 | `84a7f9c10af98a88c5db5aa4cfb4334654111efc` | 42484820959 | 42484821017 |
| #238 | `5366dc6b275b15d14e5d112325845529c51a490e` | 42486720445 | 42486720490 |

[Review #238](https://github.com/Human-Cognome-Project/human-cognome-project/pull/238#pullrequestreview-5818542620) records acceptance. The delta from contributor-tested `ebe0644aad7d2344594128f15949fe243a41c88e` to reviewed #238 contains only five Markdown files. Integration adds documentation reconciliation only beyond that reviewed source; no duplicate baseline CI is requested.

All 12 jobs in [native run 42486720445](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42486720445) succeeded. Independently inspected regression logs 128751381895, 128751381972, 128751381922 and 128751381937 cover all 115 numbered tests exactly once (29/29/29/28). Database log 128751381866 records 180 warm-to-active integration checks and `addressed_reads=24 private_reads=0`. Compiler success is confirmed by hosted job status; contributor-local raw bundles were not inspected and the local matrix was not rerun by the maintainer.

The input factory admits caller capacity plus logical snapshot payload and inspection/copy work before owner allocation/argument copying. This is not an allocator-capacity or complete constructor/setup bound. Subsequent constructor-byte/setup-work gates and combined caller/borrowed accounting remain next, with unwind/retry and single counting of moved snapshots/retained destinations. Remaining operations and #229 factoring stay open; GPU qualification is separate.

## Binding/coordinator reservation domain — contributor CPU evidence (2026-11-08)

Contribution awaiting review, based on main e19a134f386b4f3cb0e85d41fbeccf476a141759
through accepted #238. The [#238 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/238#pullrequestreview-5818542620)
found no source blockers and verified both #237/#238 hosted gates; this work does
not repeat their accepted baseline. Tested code:
`09858db0632deeef3ce16052914c2711c526879e` (implementation 721ff3d5 plus arithmetic
control). Later source changes are documentation only.

A shared reservation allocator now gates actual binding/coordinator buffer requests
before allocation. Its domain spans per-selection temporary bindings and retained
buffers, records live/peak requested bytes and cumulative allocation work, and
survives constructor unwind. Growth includes old/new simultaneous backing; release
refunds live bytes only. Underlying allocation failure acquires no live charge.
The owner census includes the domain object without adding its buffer report twice.
See [scope/exclusions](../../docs/OWNER-RESOURCES.md#bindingcoordinator-allocation-domain-contribution-awaiting-review).

CPU-only Clang/LLVM 15 with Taichi 5ba579a8a11255b7a258cde93801070956d81d57,
CUDA_VISIBLE_DEVICES=-1, shared personal host. The focused native matrix is
exchange_owner, exchange_owner_multiple, exchange_bindings, exchange_commit and
exchange_fields × workers 1/2 × bulk/default or --direct --compiler-no-unroll.
Final aggregate result and manifest follow below.

| Fixture | Byte rejection boundaries | Work rejection boundaries | Live/peak domain bytes | Cumulative allocation work |
|---|---:|---:|---:|---:|
| One selection | 11 | 16 | 98,448 / 98,448 | 141,728 |
| Four selections | 11 | 31 | 98,448 / 98,448 | 271,568 |

All four configurations reproduce these values. Each rejected constructor returns
no owner, zero remaining domain bytes and zero observer-tracked allocations, with
successful exact-budget retry and unchanged native snapshot. Independent allocator
controls verify packed-bit/integer requests against observed C++ allocation sizes,
reallocation overlap rejection preserving old vector contents, nonrefundable work,
product/sum overflow and underlying allocator failure. Existing adoption failure
sweeps now cover 4,193/7,857 sites (the domain object adds one), with final-alias
reclamation and repeated exchange/tick oracles retained. Whole observed adoption
peaks are 277,034 / 333,693 requested bytes, deliberately distinct from this bounded
98,448-byte buffer subset; no total-adoption claim follows.

Raw contributor evidence is local in adoption-reservation-09858db0, retaining build,
commands, twenty case logs, configuration/toolchain and binary identities. No full
115-test baseline, complete compiler audit, database or GPU run is claimed. The
remaining map/key, projection, catalogue/declaration, object/control-block and graph
setup allocations/work, allocator/exception-runtime overhead and borrowed native
resources remain outside this domain. Complete owner resources and remaining
operations are still open.

Result: all 20 focused native runs passed; every process exited zero. Manifest
SHA256SUMS verified; its SHA-256 is:

    e958ca1efab122ef2c5fb232e47b492cc9ec64cc36f038da1246250c403a63a7

Contributor-local archive: setup-evidence/adoption-reservation-09858db0.tar.gz.
The final delta from tested code changes documentation only. Raw evidence remains
local; this manifest does not make it remotely available or replace hosted CI.

## Map outer-store allocation domain — contributor CPU evidence (2026-11-08)

Contribution awaiting review on provisionally accepted #239 at
`df3625fffbf37118a40d5c702005f834a626f3a5`. The
[#239 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/239#pullrequestreview-5818720258)
found no blockers and permits this continuation. At the recorded check smoke
42499395649 passed and native 42499395644 remained in progress; no duplicate
parent baseline was started. Integrated acceptance remains through #238.
CURRENT-STATE and the continuation entry reflect this provisional dependency.

Tested source: `66ec7c761bcf9155b921319a17468b5c48e851ed`. Final changes after this
pin are documentation only. All six retained map outer vectors and initial owner
map copies now attach to the reservation domain, together with every temporary
validation map and its explicit trusted/staging copies. Nested allocated UUID,
path and address payload is excluded; fixed nested object headers are inside the
outer slot size. Plain copied snapshots detach from the source owner's domain,
while setup-owned copies explicitly select one. See [scope and lifetime](../../docs/OWNER-RESOURCES.md#map-outer-store-allocation-domain-contribution-awaiting-review).

CPU-only Clang/LLVM 15, Taichi 5ba579a8a11255b7a258cde93801070956d81d57,
CUDA_VISIBLE_DEVICES=-1, shared personal host. The focused matrix is
exchange_owner, exchange_owner_multiple, exchange_bindings, exchange_commit and
exchange_fields × workers 1/2 × bulk/default or --direct --compiler-no-unroll.
The final aggregate result and verified manifest are recorded below.

| Fixture | Byte rejection boundaries | Work rejection boundaries | Domain live/peak bytes | Cumulative allocation work |
|---|---:|---:|---:|---:|
| One selection | 20 | 29 | 172,368 / 172,368 | 241,256 |
| Four selections | 20 | 59 | 172,368 / 172,368 | 453,816 |

All four configurations reproduce these charges. Every rejected constructor
reclaims domain and observer-tracked allocations; exact allowances retry with an
unchanged native snapshot. Controls reject before outer allocation/nested-row
construction, preserve existing backing and logical rows on growth denial, verify
outer bytes separately from larger outer-plus-nested observed bytes, check bounded
copies and detached snapshots after source-domain destruction, retain destination
allocators on assignment and propagate them with swapped backing. The retained
domain total equals six outer map capacities plus binding/coordinator buffers,
without counting nested payload or the census twice.

The full allocation-failure traces remain 4,193/7,857 sites, with final-alias
reclamation and existing repeated exchange/tick/numerical witnesses. The additional
allocator pointers add 48 observed retained bytes; whole-adoption observed requested
peaks are 277,082 / 333,741 bytes and remain distinct from this selected-domain peak.
Cumulative work includes released setup copies and growth blocks; it is not peak
memory or allocator CPU time.

Evidence remains contributor-local in map-store-domain-66ec7c76 (build, commands,
twenty case logs, configuration/toolchain and binary hashes). No full 115-test
baseline, complete compiler audit, database or GPU run is claimed. Remaining
nested-key, projection, catalogue/declaration, object/control-block and graph
allocation/work limits, allocator/exception-runtime overhead and combined borrowed
native accounting remain open, ahead of remaining-operation integration.

Result: all 20 focused native runs passed; every process exited zero. SHA256SUMS
verified; its SHA-256 is:

    6a6aba0e91a30353b81b94be3deb3931a98e77cd44abae82458be03be8ea2306

Contributor-local archive: setup-evidence/map-store-domain-66ec7c76.tar.gz.
The final delta from tested code changes documentation only. The local manifest
identifies retained evidence and does not imply remote availability or hosted success.

<a id="maintainer-integrated-acceptance--239240-2026-11-08"></a>

## Maintainer integrated acceptance — #239/#240 (2026-11-08)

The selected binding/coordinator buffer domain (#239) and map outer-store extension (#240) are accepted with original ancestry preserved. Earlier provisional and contributor records retain their original scope.

| PR | Reviewed source | Native run (passed) | Smoke run (passed) |
|---|---|---|---|
| #239 | `df3625fffbf37118a40d5c702005f834a626f3a5` | 42499395644 | 42499395649 |
| #240 | `46afdb0a41aac3372864c758e91943276923c2ae` | 42503182912 | 42503182939 |

[Maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/240#pullrequestreview-5819269075) records the source assessment. Tested code `66ec7c761bcf9155b921319a17468b5c48e851ed` differs from reviewed #240 only in five Markdown files. Integration reconciles documentation only beyond that green source; no duplicate accepted-baseline CI was requested.

All 12 jobs in [native run 42503182912](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42503182912) passed. Independently inspected regression logs 128801943220, 128801943268, 128801943218 and 128801943214 cover all 115 tests exactly once (29/29/29/28). Database log 128801943213 records 180 checks and `addressed_reads=24 private_reads=0`. Compiler success is verified by hosted status. Contributor-local raw bundles were not inspected or rerun by the maintainer.

Accepted scope includes actual requests for selected buffers, all six map outer stores and explicit setup copies, old/new growth overlap, domain lifetime through unwind and allocator-aware copy/assignment/swap behavior. Independent snapshots detach from the owner's domain. Separately allocated nested keys, projection, catalogue/declaration, object/control-block and graph-temporary resources remain next, followed by combined caller/borrowed accounting and remaining-operation integration. Do not double-count slot headers, moved snapshots or the domain report alongside retained capacity. No complete-constructor, machine-memory, GPU or performance qualification follows. #229 remains open.

## Shared owner allocation domain — contributor CPU evidence (2026-11-08)

Contribution from merged main `56a195f4b5a5276c3e31a6b8d7b159c4d9d69767`,
following accepted #239/#240 and the [maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/240#pullrequestreview-5819269075).
No accepted baseline was rerun. This contribution adds actual allocation limits
for the declaration pointer table and shared declaration/catalogue object and
control-block requests. The allocator holds the domain through final strong/weak
release; standalone table snapshots detach their backing allocation domain.
Nested payload, preceding graph/construction work and remaining unique objects
are outside this increment. See [exact scope](../../docs/OWNER-RESOURCES.md#shared-object-allocation-domain-contribution-awaiting-review).

Final tested source: `c6f9dd993653ec246b5d7266600e5c262393b4ff`.
Documentation changes after that pin do not alter executable source. CPU-only
Clang/LLVM 15 with Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
CUDA_VISIBLE_DEVICES=-1, shared personal host. The focused matrix is
exchange_owner, exchange_owner_multiple, exchange_bindings, exchange_commit and
exchange_fields × workers 1/2 × bulk/default or --direct --compiler-no-unroll.

The initial `91c698e1` run stopped all four configurations at a new owner
assertion: it incorrectly included nested declaration vector capacity in the
selected outer-table/shared-object sum. Shared/weak lifetime controls had passed.
`9492e975` corrects that test boundary; `c6f9dd99` adds an exact reconciliation
against independently observed allocation blocks containing each shared pointee.
The failed logs/manifest are preserved in shared-owner-domain-91c698e1; their
manifest SHA-256 is 1b11eb18b2ed11d5a8e3375c965caf0e6ef45590e5f5036e439746cf931e84c9.
The intermediate corrected source was built; the final source includes the
independent observer assertion and is the qualification pin.

Controls compare an aligned shared object's complete request against the separate
allocation observer, reject before construction, retain charge through weak
references, release the domain without a cycle, preserve prepaid work on a throwing
constructor, retry exactly, and detach copied table backing. The owner witness
retains declaration/catalogue aliases after owner destruction, then retains a weak
catalogue after pointee destruction and verifies final charge release. Actual
retained selected bytes are reconciled exactly with independently observed shared
requests plus table/map/buffer capacity, without adding nested census payload.
The observer reconciliation qualifies this standard library's colocated shared
allocation layout; production admission uses each actual rebound allocation type.

All four configurations passed both owner fixtures with the same charges:

| Fixture | Byte rejection boundaries | Work rejection boundaries | Domain live/peak bytes | Cumulative allocation work |
|---|---:|---:|---:|---:|
| One selection | 22 | 34 | 173,072 / 173,072 | 241,976 |
| Four selections | 25 | 72 | 174,704 / 174,704 | 456,264 |

Every budget rejection reclaims all observed allocations, keeps native state
unchanged and permits exact fresh retry. Full allocation-failure traces cover
4,193 / 7,857 sites, with zero remaining blocks. Whole-adoption retained requested
bytes are 276,625 / 331,883; observed peaks are 277,170 / 333,925. These are separate
from selected domain bytes and the conservative retained-capacity census
(277,249 / 332,315), and must not be added together. Allocation work includes
released temporary buffers and old table blocks; it is not simultaneous peak or
allocator CPU time. New shared allocator/domain references add 88 / 184 observed
retained bytes compared with #240, without adding allocation sites.

All 20 focused native runs passed; all processes exited zero. The run retained
one non-fatal offline-cache lock warning after a passing exchange-commit case;
no cache cleanup or test restart was needed. The final SHA256SUMS verified, with
SHA-256:

    607a2589a4bd4d98cdfb615faa4ff41e91c382133048d41a1184c07a94a42d24

Contributor-local evidence: shared-owner-domain-c6f9dd99 (build, exact commands,
twenty logs, source/configuration/toolchain and binary hashes). Archive:
setup-evidence/shared-owner-domain-c6f9dd99.tar.gz. The initial failed archive is
setup-evidence/shared-owner-domain-91c698e1-failed.tar.gz. Raw bundles remain local;
their hashes identify evidence, not remote availability or hosted acceptance.

No full 115-test baseline, full compiler audit, database or GPU run is claimed.
Nested payload, projection and other unique objects, graph temporaries, complete
setup-work limits, allocator/domain overhead and combined caller/borrowed resources
remain open before remaining-operation integration. #229 remains open.

<a id="maintainer-integrated-acceptance--241-2026-11-09"></a>

## Maintainer integrated acceptance — #241 (2026-11-09)

Shared declaration/catalogue object/control-block requests and declaration-table backing are accepted at reviewed head `918312b7b289c00263095b89cebbc25dbb387fce`. [Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/241#pullrequestreview-5825596717) records scope and source assessment. Native [42515939810](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42515939810) and smoke [42515939806](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42515939806) passed.

All 12 native jobs succeeded. Independently inspected regression logs 128840295371, 128840295498, 128840295484 and 128840295440 cover 115 tests exactly once (29/29/29/28). Database log 128840295307 records 180 checks and `addressed_reads=24 private_reads=0`. Compiler success is verified by hosted job status. The contributor-local raw archive was not inspected or rerun by the maintainer.

The tested `c6f9dd993653ec246b5d7266600e5c262393b4ff` to reviewed-head delta contains only five Markdown files. Integration adds documentation reconciliation only beyond that green head; no duplicate baseline CI is requested. Original ancestry, the failed initial assertion, its corrected scope and the added independent exact allocation reconciliation remain preserved.

Shared allocators retain the domain through final weak release without an ownership cycle. Ordinary table snapshots detach backing accounting while retaining pointee ownership. Actual shared requests and table growth are covered; preceding catalogue/declaration construction, nested payload, projection/other unique objects, graph temporaries, complete setup work, domain overhead and combined caller/borrowed resources remain open. Reference release remains exclusive and synchronous. No complete constructor, GPU or performance qualification follows; #229 and remaining-operation integration remain open.

## Unique owner object allocation domain — contributor CPU evidence (2026-11-09)

Contribution from merged main `3eaf6e457ba060fd0e8b79422160820ea1fa34f2` through accepted #241. The
[maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/241#pullrequestreview-5825596717)
found no blocking corrections; native 42515939810 and smoke 42515939806 passed.
No accepted baseline was rerun. This increment admits actual storage for the
owner, two projection objects, coordinator and persistent binding before their
constructors run. Shared declaration/catalogue and table gates are unchanged.
Nested payload and complete setup work remain outside this scope. See
[unique object allocation and lifetime](../../docs/OWNER-RESOURCES.md#unique-object-allocation-domain-contribution-awaiting-review).

Tested source: `fa9809145dc077c865acc2b6ee4f8c1c3cefd70a`; subsequent changes are
documentation only. CPU-only Clang/LLVM 15, matching Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, CUDA_VISIBLE_DEVICES=-1,
shared personal host. Focused matrix: exchange_owner, exchange_owner_multiple,
exchange_bindings, exchange_commit and exchange_fields × workers 1/2 ×
bulk/default or --direct --compiler-no-unroll.

New controls check pre-construction rejection, outer bytes acquired before nested
member work, nested budget denial, constructor exception unwind, underlying
allocation failure without phantom live bytes, prepaid request work, exact fresh
retry, alignment, pointer/deleter moves and swaps, and domain release on reset.
Null-domain standalone bindings use matching allocation/deallocation. The existing
independent allocation observer additionally locates each unique object's actual
request, checks its size and reconciles all selected retained allocation bytes
exactly. Existing shared/weak release, adoption rejection/retry, full failure-site
reclamation and repeated numerical/exchange witnesses remain active.

All four configurations passed both owner fixtures with these charges:

| Fixture | Byte rejection boundaries | Work rejection boundaries | Domain live/peak bytes | Cumulative allocation work |
|---|---:|---:|---:|---:|
| One selection | 24 | 39 | 176,248 / 176,248 | 245,152 |
| Four selections | 27 | 77 | 177,880 / 177,880 | 459,440 |

The five object requests add 3,176 selected bytes in each fixture; these requests
already existed outside the quota in #241. Embedded deleter references increase
whole-adoption retained requested bytes by 64, with no additional allocation sites.
Whole-adoption retained requested bytes are 276,689 / 331,947; observed peaks are
277,234 / 333,989. Conservative retained census totals are 277,313 / 332,379.
These overlapping measurements must not be added together. Allocation work is
cumulative request payload, not simultaneous peak or allocator CPU time.

Every rejected owner construction releases all tracked allocations and permits
an exact fresh retry with unchanged native state. Full failure traces cover
4,193 / 7,857 allocation sites with zero remaining blocks. New unique-object
requests reconcile exactly with the independent observer alongside already-covered
buffers, map/table backing and shared objects; nested capacity is not added.

All 20 focused native runs passed; every process exited zero. Final SHA256SUMS
verified, with SHA-256:

    591fa54b648a38673870ff568bd0a1f06c429c255cd2f8ce91b5b5a996dd31db

Contributor-local evidence: unique-owner-domain-fa980914 (build, exact commands,
twenty case logs, source/configuration/toolchain and binary hashes). Archive:
setup-evidence/unique-owner-domain-fa980914.tar.gz. Source was clean before and
after execution. No offline-cache lock warning appeared in this run. Raw evidence
remains local; hashes identify it, not remote availability or hosted acceptance.

No full 115-test baseline, full compiler audit, database or GPU run is claimed.
Nested key/projection payload, preceding catalogue/declaration construction, graph
temporaries, complete setup-work limits, domain/allocator/exception overhead and
combined caller/borrowed resources remain open before remaining-operation
integration. #229 remains open. The final delta from the tested pin changes five
Markdown files only.

## Projection table allocation domain — contributor CPU evidence (2026-11-09)

Contribution on provisionally accepted #242 at
`ad370dd564171650686a00e42d01b8d6be4fee76`. The
[maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/242#pullrequestreview-5827980299)
found no blockers and authorizes bounded continuation. At the initial check,
smoke 42642498358 passed and native 42642498836 remained in progress; no duplicate
parent run was started. Integrated acceptance remains through #241. The handoff
now marks the five unique-object storage requests provisional instead of listing
them as unimplemented work.

Tested source: `aab3308dc79049fb96bef6c48a719367b44e28bd`; subsequent changes are
documentation only. This extends actual allocation admission to both retained
projections' occurrence, exposure and active/spare parent tables, including initial
copies and setup reservations. The owner directly copies its const-reference
projection into the selected domain, replacing the old by-value argument plus
move. Ordinary snapshots detach table accounting; moved values retain it. Nested
UUID/path/address, recipe and input payload remain outside this gate. See
[scope and lifetime](../../docs/OWNER-RESOURCES.md#projection-table-allocation-domain-contribution-awaiting-review).

CPU-only Clang/LLVM 15, matching Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, CUDA_VISIBLE_DEVICES=-1, shared personal
host. The focused exchange matrix is exchange_owner, exchange_owner_multiple,
exchange_bindings, exchange_commit and exchange_fields × workers 1/2 ×
bulk/default or --direct --compiler-no-unroll. The same source also builds/runs
letter_projection_test, membership_fields_test, active_workbench_test,
letter_projection_native_test and membership_fields_native_test through CTest.
Their executable hashes and combined CTest log are retained with the matrix.

New controls reconcile table bytes with the separate outer-plus-nested allocation
observer, check explicit-domain and detached snapshots (including inactive parent
rows), use snapshots after source-domain destruction, verify moved-domain lifetime,
and reclaim preceding recipe allocations when the first table request is denied.
The full owner witness independently locates every retained table request and
checks the complete selected-domain sum without double-counting nested storage.
Existing budget/failure-site sweeps, exact retry, unchanged state, shared/weak
lifetimes and repeated native operations remain active.

All four configurations passed both owner fixtures with these charges:

| Fixture | Byte rejection boundaries | Work rejection boundaries | Domain live/peak bytes | Cumulative allocation work |
|---|---:|---:|---:|---:|
| One selection | 29 | 49 | 181,856 / 181,856 | 251,048 |
| Four selections | 34 | 87 | 199,928 / 199,928 | 484,368 |

Selected retained bytes increase by 5,608 / 22,048 from #242, including 128 bytes
of allocator references embedded in the two projection objects; their object
requests already participate in the domain. Whole-adoption retained requested
bytes are 276,817 / 332,075 and observed peaks 277,362 / 334,117. Conservative
retained census totals are 277,441 / 332,507. Whole retained/peak/census bytes
increase by 128 without new allocation sites. These overlapping measurements
must not be summed; cumulative allocation work is not peak memory or CPU time.

Full failure traces remain 4,193 / 7,857 sites, with zero remaining blocks and
unchanged native state. Expanded byte/work rejection sweeps reclaim every stopped
construction and permit exact fresh retry. The five additional CTest cases passed
(100%, zero failures). All 20 exchange runs and all five additional tests passed; every process exited
zero. The final SHA256SUMS verified, with SHA-256:

    3f9e4c03ff1de3f709e82c28bb9b4f47cca24df901c2b0aacaaa23c4d8388ac7

Contributor-local evidence: projection-table-domain-aab3308d (build, commands,
twenty exchange logs, five-case CTest log, source/configuration/toolchain and
executable hashes). Archive: setup-evidence/projection-table-domain-aab3308d.tar.gz.
Source was clean before/after execution. No offline-cache lock warning appeared.
The final delta from tested source changes five Markdown files only. Raw bundles
remain local; their hashes identify evidence, not remote availability or hosted
acceptance.

No full 115-test baseline, full compiler audit, database or GPU run is claimed.
Nested key/recipe/input payload, preceding catalogue/declaration construction and
graph temporaries, complete setup work, domain/allocator/exception overhead and
combined caller/borrowed resources remain open before remaining operations.
#156/#145/#229 remain open.

## Maintainer integrated acceptance — #242 (2026-11-09)

Reviewed head `ad370dd564171650686a00e42d01b8d6be4fee76`, merged at `01d25b046ec838295b6ce3ffe9b39ad2fa92a57d`. [Final review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/242#pullrequestreview-5828999730) reconciles provisional acceptance. Native 42642498836 and smoke 42642498358 passed; all 12 native jobs succeeded. Independently inspected regression logs 129251736445/129251736337/129251736451/129251736436 cover 115 distinct passing tests. Database log 129251736659 records 180 checks, addressed_reads=24/private_reads=0. Compiler jobs passed; local raw evidence remains contributor-retained, not independently rerun. The tested `fa9809145dc077c865acc2b6ee4f8c1c3cefd70a` to reviewed-head delta is five Markdown files only. No duplicate baseline requested.

Five unique-object allocation gates are accepted; nested payload, preceding construction, complete setup and combined resources remain open. #243 (`01943e888b6e3583cb5c9845f860919aa98d9ac9`) is provisionally accepted for the retained projection-table extension by review 5829010722; its hosted native run 42646168341 remains pending. Its reported 20 exchange runs plus five reader/value cases are local contributor evidence. This status reconciliation does not merge that pending implementation or infer GPU qualification.

## Maintainer integrated acceptance — #243 (2026-11-09)

Reviewed head `01943e888b6e3583cb5c9845f860919aa98d9ac9`; [final review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/243#pullrequestreview-5829428119) reconciles provisional review 5829010722. Native 42646168341 and smoke 42646168346 passed; all 12 native jobs succeeded. Independently inspected regression logs 129265803857/129265804209/129265803867/129265803958 cover 115 distinct passing tests (29/29/29/28). Database log 129265803763 records 180 checks, addressed_reads=24/private_reads=0. All compiler jobs passed. Contributor-local 20 exchange runs and five reader/value cases remain separately attributed; raw archives were not independently rerun. The tested `aab3308dc79049fb96bef6c48a719367b44e28bd` to reviewed-head delta contains five Markdown files only.

Integration preserves main's #242 acceptance record and #244 Docker release-security requirement, with documentation reconciliation only beyond the reviewed head. No duplicate accepted-baseline CI is requested. Both retained projections' four table stores are admitted before allocation, snapshots detach accounting, and moved values retain domain lifetime. Nested key/recipe/input payload, preceding catalogue/declaration/graph construction, complete setup work and combined caller/borrowed resources remain open. No complete resource, performance or GPU qualification follows; #156/#145/#229 remain open.

## Projection setup work — contributor CPU evidence (2026-11-09)

Tested source: `50374ed94e7f7e7bc62d7bc81b5b0f9e00d49965`. The later merge
of main `a7ceb088fe3c10aa77d04071291e8a94cc17e64b` and handoff/evidence changes
are Markdown only. Parent #243 is accepted and merged; its hosted native
42646168341 and smoke 42646168346 both passed. No duplicate baseline was run.

The new explicit work allowance gates spare-projection inspection/copy and both
exposure-pool setup operations; the initial snapshot keeps its existing input
admission. The scope and result phases are described in
[OWNER-RESOURCES](../../docs/OWNER-RESOURCES.md#projection-setup-work-gate-contribution-awaiting-review).
This is a bounded setup-work contribution, not nested allocation admission or
complete constructor-resource qualification.

Validation uses CPU-only Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, CUDA disabled, on the shared personal
host after its Ubuntu package updates and WSL kernel update to
6.18.54.1-microsoft-standard-WSL2. No GPU or database work is included.

Both owner fixtures passed zero allowance, every successive rejection boundary
through all five setup phases, one-unit-short rejection and exact fresh retry.
Every rejected construction releases all observed C++ allocations. Zero allowance
performs no observed allocation. Readiness, generation and tick count remain
unchanged throughout the sweep; final native snapshot and caller parent-pool sizes
remain unchanged. Partial pools are discarded on rejection rather than reused.

| Fixture | Setup rejection boundaries | Exact structural work |
|---|---:|---:|
| One selection | 1,556 | 51,284 |
| Four selections | 4,628 | 137,062 |

All four worker/compiler configurations report the same figures. Existing
allocation-failure traces still cover all 4,193 / 7,857 sites with zero blocks
remaining, plus repeated exchange/tick and exact reservation retry controls.
Selected domain live/peak remains 181,856 / 199,928 bytes, with allocation-request
work 251,048 / 484,368. Whole-adoption retained/peak figures remain
276,817 / 277,362 and 332,075 / 334,117 bytes; retained census remains
277,441 / 332,507. These overlapping accounting domains must not be summed.

The focused matrix is owner, multiple-owner, bindings, commit and fields × workers
1/2 × bulk/default or direct/compiler-no-unroll. Additional CTest cases are
letter_projection_test, membership_fields_test, active_workbench_test,
letter_projection_native_test and membership_fields_native_test. Logs, commands,
source pins, toolchain/configuration and executable hashes are retained under
`projection-setup-work-50374ed9`; raw evidence remains contributor-local.

Nested UUID/path/address and recipe/input allocations, catalogue/declaration
construction, graph temporaries, remaining setup work, domain/allocator/exception
costs, combined caller/borrowed accounting and remaining-operation integration
remain open. No full baseline, compiler audit or GPU qualification is claimed.
Refs #156/#145; #229 remains open.
All 20 focused exchange processes and all five additional CTest cases passed;
each process exited zero. No source changes or offline-cache lock warnings were
observed during the run. The final delta from tested source is five Markdown
files only, including the accepted main documentation merge.
Verified SHA256SUMS manifest SHA-256:

    59ae89d3466a3102f8e6edd424b9e0c8778f5d963d5a0b347c7aceb8d93b6b35

Local archive: `setup-evidence/projection-setup-work-50374ed9.tar.gz`. The hash
identifies retained evidence; it does not make the raw archive remotely available
or replace hosted CI.

## Membership construction allocation domain — contributor CPU evidence (2026-11-09)

Tested source: `ba85ddce108e55d838d802e978828dc32d46d7a6` on provisionally accepted #245 head
`9023dbec59124bc4983d0e6b37fac6445c6aef14`. The
[review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/245#pullrequestreview-5829660424)
found no blocking source findings and explicitly permits bounded continuation.
At inspection, parent smoke 42661328034 passed and native 42661328044 remained
in progress; no duplicate baseline was started. Integrated acceptance remains #243.

Catalogue collection now attaches its contribution table and temporary graph map/
set nodes before constructing them. Nested target sets receive an explicit domain
allocator. Shared catalogue-object admission still occurs after collection.
See [scope](../../docs/OWNER-RESOURCES.md#membership-construction-allocation-domain-contribution-awaiting-review).
Separately allocated keys, callable backing, graph work/stack and other setup
surfaces are excluded; no complete catalogue or owner-resource limit is claimed.

CPU-only Clang/LLVM 15, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
workers 1/2, bulk/default and direct/compiler-no-unroll, shared personal host.
Both owner fixtures pass catalogue-only byte/work boundary sweeps, exact retry,
complete reclamation and detached-copy use after domain destruction. Snapshot
identities, target pins and counts match standalone collection. The whole owner
independently matches the actual contribution backing request, then releases that
charge at last strong alias and the remaining control block at last weak alias.
The existing projection setup sweeps and repeated exchange/tick oracles remain.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Allocation failure sites |
|---|---:|---:|---:|---:|
| One selection | 57 / 81 | 182,768 | 255,472 | 4,109 |
| Four selections | 67 / 124 | 203,528 | 494,312 | 7,773 |

All injected failure sites reclaim every observed block. Whole-adoption retained/
peak requested bytes are 276,833 / 277,378 and 332,091 / 334,133; conservative
retained census is 277,457 / 332,523. Whole retained/peak/census increases by 16
bytes for the catalogue's embedded allocator reference. Selected live bytes also
include 896 / 3,584 bytes of contribution backing previously outside the domain.
The trace now has 84 fewer observed allocation sites than #245; this measurement
is not a performance qualification. Temporary node requests contribute to
allocation work and instantaneous overlap, not final retained catalogue bytes.
Selected/census/whole-scope figures overlap and must not be summed.

The source-pinned validation matrix is owner, multiple owner, bindings, commit
and fields × workers 1/2 × bulk/default or direct/compiler-no-unroll. The affected
reader/value cases are letter_projection_test, membership_fields_test,
active_workbench_test, letter_projection_native_test and
membership_fields_native_test. No GPU, database, full baseline or full compiler
qualification is claimed. Nested key/recipe/input allocation, declaration
construction, other setup work and combined caller/borrowed resources remain
open before remaining operations; #156/#145/#229 remain open.
All 20 focused exchange processes and five additional CTest cases passed; each
process exited zero. Source remained clean during execution. Final changes after
the tested source are four Markdown files only. Contributor-local evidence is
`membership-construction-domain-ba85ddce`, including build/commands, twenty case
logs, CTest log, source/configuration/toolchain and executable hashes. Archive:
`setup-evidence/membership-construction-domain-ba85ddce.tar.gz`. Verified manifest
SHA-256:

    84f7707897a68ed942927efe052f07bc2ccd8ea7678dc2fd9109d8e111d9a910

Raw evidence remains local; the hash identifies the bundle without claiming
remote inspection or replacing hosted CI.

## Maintainer integrated acceptance — #245/#246 (2026-11-10)

Reviewed combined head `8c0468168e3837a45e0526c51dce55707a2f81c3`; [#246 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/246#pullrequestreview-5841384226) and [#245 reconciliation](https://github.com/Human-Cognome-Project/human-cognome-project/pull/245#pullrequestreview-5841384443). Native 42665443526 and smoke 42665443451 passed. All 12 native jobs succeeded, including the final aggregate and compiler matrix. Personally inspected regression logs 129331678215/129331678210/129331678162/129331678220 cover 115 distinct passing tests (29/29/29/28); database log 129331678034 reports 180 checks, addressed_reads=24/private_reads=0.

Parent #245 native 42661328044 has conclusion failure, despite success for all 11 listed jobs; no final native-cpu aggregate job is listed. The cause is unestablished. Preserve that outcome, rather than claiming the exact-head workflow passed. The successful full-stack descendant run above qualifies combined integration without duplicating the parent baseline. Parent smoke 42661328034 passed. Contributor-local 20+5 matrices remain separately attributed; their raw archives were not remotely inspected or rerun. #246 tested `ba85ddce108e55d838d802e978828dc32d46d7a6` to reviewed-head delta is four Markdown files only.

Integration preserves both PR ancestries and changes only documentation beyond the green head. Projection setup work, retained membership contribution backing and temporary map/set node requests are accepted within their stated scopes. Nested key/recipe/input payload, declaration construction, remaining graph traversal/setup work, domain/allocator/exception overhead and combined caller/borrowed resources remain open before remaining-operation integration. No complete resource, performance or GPU qualification follows; #156/#145/#229 stay open.

## Declaration table allocation domain — contributor CPU evidence (2026-11-10)

Tested source: `b09b41243b82238679c6b5626033a82d5516670c`, based on accepted
main `f1c2a260` through the combined #245/#246 stack. The
[review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/246#pullrequestreview-5841384226)
reports no blocking findings and full descendant native 42665443526 / smoke
42665443451 success. Parent #245 native 42661328044 remains a failed run with
unestablished cause and no final aggregate job listed; it is not relabeled green.
No duplicate parent baseline was run.

Outgoing/incoming source and ordered-leaf tables now enter the owner allocation
domain during declaration construction, including temporary aggregate/parts
backing and the explicit reverse copy. Ordinary value copies detach, moves retain
the domain. Nested parent/key backing and construction work remain excluded.
See [scope](../../docs/OWNER-RESOURCES.md#declaration-table-allocation-domain-contribution-awaiting-review).

CPU-only Clang/LLVM 15 and Taichi `5ba579a8a11255b7a258cde93801070956d81d57`
on the shared personal host. Matrix: owner, multiple-owner, bindings, commit and
fields × workers 1/2 × bulk/default or direct/compiler-no-unroll. Reader/value
cases include structural_recipe_test, letter_projection_test,
membership_fields_test, active_workbench_test, letter_projection_native_test
and membership_fields_native_test. No GPU or database work is included.

New construction/explicit-copy controls sweep byte/work rejection boundaries,
verify all observed storage is reclaimed, and retry with exact allowances.
Snapshots are checked after domain destruction; moved declarations retain their
domain and complete structural values. The owner independently finds every
retained source/leaf table request and reconciles the selected-domain sum.
Existing complete allocation-failure, projection setup, catalogue, shared/weak
lifetime and repeated exchange/tick witnesses remain active.

| Fixture | Owner byte/work rejection boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 58 / 90 | 184,912 | 258,144 | 4,101 |
| Four selections | 73 / 160 | 212,104 | 505,000 | 7,741 |

All failure sites reclaim all observed blocks. Whole-adoption retained/peak
requested bytes are 276,929 / 277,474 and 332,475 / 334,517. Conservative census
is 277,553 / 332,907. These totals increase 96 / 384 bytes from #246 for the
three embedded allocator references in each of two declarations per selection.
The selected domain additionally covers 2,048 / 8,192 bytes of pre-existing table
backing. Aggregate construction now moves its source row rather than copying an
initializer-list element; the trace observes 8 / 32 fewer allocation sites.
This is not performance qualification. Selected, census and whole-scope figures
overlap and must not be added together. Allocation work is not peak memory or
measured CPU work.

Projection setup still covers 1,556 / 4,628 boundaries with structural work
51,284 / 137,062. Nested parent/UUID/path/address and recipe/input allocation,
declaration and graph setup work, combined caller/borrowed resources and remaining
operations stay open; #156/#145/#229 remain open. No full baseline/compiler audit
or GPU qualification follows from these focused CPU checks.
All 20 exchange processes and all six additional CTest cases passed; each
process exited zero. Source remained clean during execution. The final changes
after tested source are four Markdown files only. Contributor-local evidence:
`declaration-construction-domain-b09b4124` (build/commands, twenty case logs,
six-case CTest log, source/configuration/toolchain and executable hashes).
Archive: `setup-evidence/declaration-construction-domain-b09b4124.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    811de35b07db3369afcfd2a4d2ff8de91db3795fc94df011e8d68eac4cec30e7

The raw bundle remains local; its hash does not imply remote inspection or
replace hosted acceptance.

## Declaration parent allocation domain — contributor CPU evidence (2026-11-10)

Tested source: `6db0beaefe817cc13f1d532923ddd9a4bf5ecad8` on provisionally
accepted #247 head `a13d3125b814662fd1f03d5aca975ef349b645ba`. The
[review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/247#pullrequestreview-5841698860)
found no blocking corrections and permits bounded continuation. At inspection,
parent smoke 42813498122 passed; native 42813498120 remained in progress. No
duplicate baseline was run. Integrated acceptance remains through #246; the
historical #245 failed workflow remains recorded as such.

The explicit owner declaration construction/copy paths now attach each direct
parent vector before copying rows. Ordinary source/declaration snapshots detach
nested parent backing; moves carry its allocator with the backing. The explicit
copy reserves source tables and constructs domain-bound parent copies directly,
without first allocating detached parents. Scope excludes parent UUID/address
payload, other recipe/input vectors, setup work and complete resource accounting;
see [inventory](../../docs/OWNER-RESOURCES.md#declaration-parent-allocation-domain-contribution-awaiting-review).
Awake/asleep mechanics remain outside this work.

CPU-only Clang/LLVM 15, pinned Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 and bulk/default or
direct/compiler-no-unroll on the shared personal host. Tests extend construction
and explicit-copy sweeps to the nested requests, verify every parent allocator,
exact selected-domain totals and snapshot detachment, plus independent actual
allocation lookup in the owner. Full parent values remain part of structural
equivalence. Existing exact retry, reclamation, moved lifetime, projection setup,
catalogue, failure-site and repeated exchange/tick checks remain active.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 61 / 96 | 186,208 | 259,456 | 4,101 |
| Four selections | 74 / 184 | 217,288 | 510,248 | 7,741 |

All failure sites reclaim every observed allocation. Whole-adoption retained/peak
requested bytes are 277,025 / 277,570 and 332,859 / 334,901; retained census is
277,649 / 333,291. The 96 / 384-byte increase over #247 is embedded parent-allocator
references in source rows. An additional 1,200 / 4,800 bytes of pre-existing parent
backing is now inside the selected domain. Allocation site counts are unchanged.
Projection setup remains 1,556 / 4,628 boundaries and work 51,284 / 137,062.
Selected/whole/census figures overlap and must not be summed; allocation-request
work is not peak memory or CPU timing.

Validation matrix: owner, multiple owner, bindings, commit and fields × workers
1/2 × bulk/default or direct/compiler-no-unroll. Additional cases:
structural_recipe_test, letter_projection_test, membership_fields_test,
active_workbench_test, letter_projection_native_test and membership_fields_native_test.
No GPU/database/full baseline/compiler audit was run. Remaining nested UUID/path/
address and recipe/input allocations, declaration/catalogue/graph setup work and
combined caller/borrowed accounting remain open before remaining operations.
#156/#145/#229 remain open.
All 20 focused exchange processes and six CTest cases passed; each process exited
zero. The two-worker direct exchange_commit log contains an offline-cache lock
warning after its complete PASS result. That warning is preserved; no cache
cleanup or duplicate run was used, and cache-performance qualification is not
claimed. Source remained clean throughout execution. Final changes after tested
source are four Markdown files only.

Contributor-local evidence: `declaration-parent-domain-6db0beae`, including build,
commands, twenty case logs, six-case CTest log, source/configuration/toolchain and
executable hashes. Archive: `setup-evidence/declaration-parent-domain-6db0beae.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    45734103d38aa22a8924067c79ff8061a797fcace3ce7361f77e1f18cacbf834

Raw logs remain local; the hash identifies evidence without implying remote
inspection or replacing hosted acceptance.

## Declaration occurrence-path domain — contributor CPU evidence (2026-11-10)

Tested source: `c3bce3c9fbda68d7144e9bc56430214b5e811598`, stacked on
provisionally accepted #250 at `2c07c0e339d11ee66d0db175d55c81344d502393`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/250#pullrequestreview-5841949730)
found no blocking source corrections. Parent smoke 42816893735 passed and native
42816893666 remained in progress at inspection. #247 integration also remains a
parent dependency; integrated acceptance is through #246. The #250 post-PASS
cache-lock warning and exit-zero qualification above remain unchanged.
Deferred activity/state-on-demand research (#248/#251) is outside this work.

A separate mechanical commit moves reservation primitives into neutral `common`
support with engine aliases. Declaration owner/source paths explicitly use that
domain before ordinal copies. Other recipe/input/projection/map paths remain
detached. The warm path container type/layout changes, including embedded allocator
references, while identity, ordering and PostgreSQL encoding remain unchanged.
See the [scope and exclusions](../../docs/OWNER-RESOURCES.md#declaration-occurrence-path-allocation-domain-contribution-awaiting-review).

CPU-only Clang/LLVM 15 and Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
workers 1/2 × bulk/default or direct/compiler-no-unroll on the shared personal
host. Controls extend declaration construction/copy denial, exact retry, allocator
attachment/detachment and lifetime checks to source paths. The independent owner
observer reconciles path requests with selected-domain totals. Existing failure
injection, projection setup, catalogue, native exchange/tick and zero-new checks
remain active. Standalone warm clients build without engine linkage; a new
server-free libpq test checks exact parameter rendering and decoding, identity,
ordering, copy detachment and move/release behavior. Hosted warm CI runs it too.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 61 / 100 | 193,664 | 269,280 | 4,102 |
| Four selections | 74 / 200 | 226,952 | 529,976 | 7,745 |

Both owner failure sweeps reclaim all observed allocations. Whole-adoption
retained/peak requested bytes are 284,609 / 285,170 and 343,035 / 345,173;
retained census is 285,233 / 343,467. Projection setup boundaries remain
1,556 / 4,628, with work 51,636 / 138,566. These figures include changed embedded
path-container sizes throughout retained values, not just newly gated ordinal
backing. Selected/whole/census reports overlap and must not be summed.
Allocation-request work is neither peak memory nor CPU timing.

Validation scope: owner, multiple owner, bindings, commit and fields × four
worker/compiler variants; six structural-recipe/projection/membership/workbench
CTest cases; one server-free occurrence-path CTest. No GPU, live database,
full baseline rerun or separate full compiler audit. Remaining UUID/address and
recipe/input paths, full setup work and combined caller/borrowed accounting remain
open before remaining-operation integration. #156/#145/#229 remain open.

All 20 focused exchange processes and seven CTest cases passed, each with exit
zero. The warm-active integration client also compiled successfully; it was not
run against a database. No warning was found in this run's matrix/build logs.
Source remained clean and unchanged throughout the test run. Final changes after
tested source are five Markdown files only.

Contributor-local evidence: `declaration-path-domain-c3bce3c9`, including raw logs,
commands, source/configuration/toolchain, warm/integration builds and executable
hashes. Archive: `setup-evidence/declaration-path-domain-c3bce3c9.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    001192cfaeadffcbc4272e098e9f63dd155ff39e9ba37585e0eee665dcdd48c5

Raw evidence stays local; its hash does not imply independent remote inspection
or replace authoritative hosted CI. This is no cache-performance qualification.

## Projection occurrence-path domain — contributor CPU evidence (2026-11-10)

Tested source: `008c93200d22e25a8ac67fba05d5760461dba977`, stacked on
provisionally accepted #252 at `8b60de7f45efd9f6a5e64d742c6e252c351ec320`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/252#pullrequestreview-5842244532)
found no blocking source corrections and permits bounded continuation. At this
inspection, #252 smoke 42820621262 and warm-toolbox 42820621241 passed; native
42820621407 remained in progress. #247/#250 are still integration dependencies;
integrated implementation acceptance remains through #246. The historical #250
post-PASS cache warning and exit-zero qualification remain recorded above.
Deferred activity/state-on-demand research (#248/#251) is unchanged.

Both explicit retained-projection copies and inactive-pool setup now construct
occurrence paths with the selected domain before copying ordinals. This includes
optional provenance-owner keys and both parent pools. Ordinary value snapshots
detach; moved paths preserve domain lifetime and active/spare transfers reuse
backing. Recipe/input, catalogue contribution and map paths, plus UUID/address payload, remain excluded. There
is no additional value-layout or encoding change beyond inherited #252.
See [scope](../../docs/OWNER-RESOURCES.md#projection-occurrence-path-allocation-domain-contribution-awaiting-review).

The controls verify every nested allocator, populated spare-pool copying,
detached snapshots after source death and moved lifetime. Projection construction
plus pool setup sweeps each successive byte/work denial boundary, checks complete
reclamation and retries with exact allowances. The owner independently locates
all projection path requests alongside table requests in both retained views.
Existing owner failure injection, setup rejection, catalogue/declaration controls
and repeated native exchange/tick checks remain active.

CPU-only Clang/LLVM 15, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
workers 1/2 × bulk/default or direct/compiler-no-unroll, on the shared personal
host. No GPU, live database, full baseline or separate full compiler audit rerun.
Unchanged warm format tests were not duplicated from #252.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 63 / 112 | 193,760 | 269,376 | 4,102 |
| Four selections | 90 / 248 | 227,336 | 530,360 | 7,745 |

All owner failure sites reclaim observed storage. Whole-adoption retained/peak
requested bytes remain 284,609 / 285,170 and 343,035 / 345,173; retained census
remains 285,233 / 343,467. The existing 96 / 384 bytes of projection path backing
are newly inside the selected domain, without increasing total retained memory
or allocation-site counts. Projection setup remains 1,556 / 4,628 boundaries and
work 51,636 / 138,566. Selected/whole/census reports overlap and must not be added;
allocation-request work is not peak memory or CPU timing.

Validation scope: owner, multiple owner, bindings, commit and fields × four
worker/compiler variants, plus structural_recipe_test, letter_projection_test,
membership_fields_test, active_workbench_test, letter_projection_native_test and
membership_fields_native_test. Complete recipe/input/catalogue/map/UUID/address allocation,
setup work and combined caller/borrowed accounting remain open before remaining
operations. #156/#145/#229 remain open.

All 20 exchange processes and six supporting CTest cases passed, each with exit
zero. No warnings were found in this run's build/matrix logs. Source stayed clean
and unchanged during execution; final changes after tested source are four
Markdown files only.

Contributor-local evidence: `projection-path-domain-008c9320`, including build,
commands, raw case/CTest logs, source/configuration/toolchain and binary/runtime
hashes. Archive: `setup-evidence/projection-path-domain-008c9320.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    f3eefecc75cf2acc0e93f1db5fde44619b2ec934e4d35cdbe2ea6ea9b9c60291

Raw evidence remains local. This hash identifies artifacts without implying
independent remote inspection or replacing authoritative hosted checks.

## Catalogue occurrence-path domain — contributor CPU evidence (2026-11-10)

Tested source: `793ea992c95e5a0efe1334c7e3bab2e10797c89a`, stacked on #253's
updated handoff head `b373601e4fb37330ec7a5c900cca912fd9e9bd62`. The
[review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/253#pullrequestreview-5842598923)
provisionally accepts source `22de9b62b39fa18c76eb1554bf4ea6a552d82dba` and
requests this catalogue-path slice. The updated parent adds only two Markdown
handoff files. Parent smoke 42824232342 passed; native 42824232291 remains in
progress at inspection. #252 warm-toolbox 42820621241 passed. Dependency-stack
integration is still pending; integrated implementation acceptance remains through
#246. The historical #250 cache warning and exit-zero qualification remain intact.
Deferred wake/sleep research is unchanged; GPU work remains independently scoped.

`MembershipFields::collect` constructs the contribution key's path in the supplied
domain before ordinal copies and moves it into its row. Existing table/header
requests and ordinal backing are counted separately once. Table growth and moved
catalogues preserve paths and their domain. Ordinary snapshots detach. Nested
path storage releases at last strong alias, while shared object/control-block
accounting continues through the final weak reference. Membership traversal,
deduplication, target pins and values are unchanged; UUID/address and map/recipe/
input payload remain excluded. See [scope](../../docs/OWNER-RESOURCES.md#catalogue-occurrence-path-allocation-domain-contribution-awaiting-review).

Focused catalogue controls now include every path's allocator and capacity,
snapshot detachment and usability after domain death, moved lifetime, successive
byte/work rejection, reclamation and exact retry. Existing complete membership
value comparisons remain active. The owner observer independently locates path
requests, includes them in the selected-domain sum and verifies their release
before the last weak control-block release. Complete failure-site and repeated
allocation-free exchange/tick checks remain active.

CPU-only Clang/LLVM 15, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
workers 1/2 × bulk/default or direct/compiler-no-unroll on the shared personal
host. Validation: owner, multiple owner, bindings, commit and fields × four
variants, plus structural_recipe_test, letter_projection_test,
membership_fields_test, active_workbench_test, letter_projection_native_test and
membership_fields_native_test. No GPU, live database, full baseline or separate
full compiler audit rerun; unchanged warm-format checks were not duplicated.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 64 / 114 | 193,776 | 269,392 | 4,102 |
| Four selections | 91 / 256 | 227,400 | 530,424 | 7,745 |

Existing path backing of 16 / 64 bytes is newly admitted. Whole-adoption retained/
peak requested bytes remain 284,609 / 285,170 and 343,035 / 345,173; retained census
remains 285,233 / 343,467. Total retained memory and allocation-site counts are
unchanged. Projection setup remains 1,556 / 4,628 boundaries, work 51,636 / 138,566.
Selected/whole/census reports overlap and must not be summed; allocation-request
work is not peak memory or CPU timing.

Next concrete slice: occurrence-map path allocation in initial construction,
explicit setup copying and inactive-slot reservation of `ExchangeMap<OccurrenceBinding>`,
with appropriate domain lifetime, detached snapshots and allocation-free reuse.
Field address backing, recipe/input paths, UUID/address payload, complete setup
work and combined caller/borrowed accounting remain open before operation
integration and same-execution controls. #156/#145/#229 remain open.

All 20 focused exchange processes and six supporting CTest cases passed, each
with exit zero. All owner failure sites reclaimed observed storage. No warnings
were found in this run's build/matrix logs. Source stayed clean and unchanged
throughout execution. Final changes after tested source are four Markdown files.

Contributor-local evidence: `catalogue-path-domain-793ea992`, including build,
commands, raw logs, source/configuration/toolchain and binary/runtime hashes.
Archive: `setup-evidence/catalogue-path-domain-793ea992.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    08b678ca9e7bc22a8482b5ea0a3f8cb9de938fa3b82ab699989fae8a13d5cac6

Raw artifacts remain local; their hash does not imply independent remote
inspection or replace authoritative hosted checks.

## Occurrence-map path domain — contributor CPU evidence (2026-11-10)

Tested source: `3f6a9c45d2c6b25d09e81938838d282409f45401`, stacked on
provisionally accepted #254 at `9375b39c60d19441c2d48b7a47643d644f14c1a2`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/254#pullrequestreview-5842799829)
found no blocking corrections and explicitly accepts this next map-path slice.
Parent smoke 42826713819 passed; native 42826713814 remains in progress at
inspection. Dependency integration is pending; integrated implementation
acceptance remains through #246. Historical #250 cache-warning qualifications
remain unchanged. Deferred activity research and independent GPU work are unchanged.

Owner/binding/coordinator setup now supplies shared domains to maps. Each map
retains that lifetime before its raw outer allocator, including when empty.
Initial copies and newly reserved inactive slots attach paths before copying or
reserving ordinal backing. Explicit setup copies reserve full nested capacity
before assigning values, avoiding detached/duplicate size-then-capacity copies.
Ordinary snapshots detach and preserve inactive capacity; assignment keeps the
destination allocator, erase rotates slots, and swap exchanges backing plus
lifetime. Field addresses, UUID payload and full setup work remain outside this
gate. See [scope](../../docs/OWNER-RESOURCES.md#occurrence-map-path-allocation-domain-contribution-awaiting-review).

Map-specific controls cover initial construction and explicit copies with active
and inactive paths, successive byte/work refusal, underlying allocation-site
failure injection, full reclamation and exact retry. Existing old/new growth
overlap and no-allocation early-copy denial checks remain. Detached snapshots
survive source-domain destruction, and empty/populated maps retain domain lifetime.
Assignment/append/erase/cross-domain swap run under the no-allocation guard.
Independent owner observation locates every retained map path, including inactive
slots in all three occurrence maps. Existing membership/projection/declaration,
owner failure-site and repeated exchange/tick controls remain active.

CPU-only Clang/LLVM 15, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`,
workers 1/2 × bulk/default or direct/compiler-no-unroll on the shared personal
host. Validation scope: owner, multiple owner, bindings, commit and fields × four
variants, plus structural_recipe_test, letter_projection_test,
membership_fields_test, active_workbench_test, letter_projection_native_test and
membership_fields_native_test. No GPU, live database, full baseline or separate
full compiler audit rerun.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 610 / 674 | 197,232 | 273,968 | 4,102 |
| Four selections | 588 / 1,252 | 230,856 | 538,488 | 7,745 |

All owner failure sites reclaim observed storage. Existing retained map paths of
3,360 bytes are newly admitted in each fixture. The six embedded map lifetime
handles add 96 bytes to retained objects. Whole-adoption retained/peak requested
bytes are 284,705 / 285,266 and 343,131 / 345,269; census 285,329 / 343,563.
Allocation-site counts are unchanged. Projection setup remains 1,556 / 4,628
boundaries, work 51,636 / 138,566. Reports overlap and must not be summed;
allocation-request work is not peak memory or CPU timing.

Initial builds at `4708e779` and `a9cb3597` failed in the new test observer because
an initializer list mixed const/non-const map pointers. The first correction did
not apply due to mixed line endings; `3f6a9c45` fixes traversal explicitly.
Both failed build logs are retained. Production targets compiled in those attempts;
no runtime pass is claimed for either failed source. The final build and test
results are pinned to the corrected revision above.

Next concrete slice: field-map address backing with engine-independent allocator
support, preserving address identity and codec/database formats. Other UUID/address
and recipe/input payload, complete setup and combined caller/borrowed accounting
remain open before operation integration and same-execution controls.
#156/#145/#229 remain open.

All 20 focused exchange processes and six supporting CTest cases passed at the
corrected source, each with exit zero. No warnings were found in the final build
or runtime logs. Earlier failed builds remain explicitly separate. Source stayed
clean and unchanged during execution; final changes after tested source are four
Markdown files only.

Contributor-local evidence: `map-path-domain-3f6a9c45`, including final build,
commands, raw runtime logs, both initial failed builds, source/configuration/
toolchain and binary/runtime hashes. Archive:
`setup-evidence/map-path-domain-3f6a9c45.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    c842cb94a753e86440177eb41a008f7f3f86741c0ec87c8a28070c998eb6682e

Raw artifacts remain local; their hash does not imply independent remote
inspection or replace authoritative hosted checks.

## Field-map address domain — contributor CPU evidence (2026-11-10)

Tested source: `eb10ce19f0247d0ce395fb0a68a5b60d21d7f5e4`, stacked on
provisionally accepted #255 at `52a5e5d00ee4aea0774d1bc135334114c78b6262`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/255#pullrequestreview-5843122210)
found no blocking source corrections and accepts this field-address slice with
codec/warm compatibility controls. Parent smoke 42830224176 passed; native
42830224093 remains in progress at inspection. Dependency integration is pending;
integrated implementation acceptance remains through #246. Prior #255 failed
build attempts and #250 cache qualifications remain recorded separately above.
Deferred activity research and independent GPU sequencing are unchanged.

`codec::Address` uses neutral shared reservation support with the same element
representation and codec rules. Field-map slots explicitly attach before initial
copy, setup copying and inactive-slot reservation. All other address backing
remains detached unless explicitly attached by a caller; ordinary snapshots and
codec results detach. C++ type/layout changes are explicit, including embedded
headers in values outside the selected domain. Identity, validation, order and
literal-pair PostgreSQL encoding are unchanged; no schema migration occurs.
See [scope](../../docs/OWNER-RESOURCES.md#field-map-address-allocation-domain-contribution-awaiting-review).

The map controls now run for both map types with their actual element sizes:
initial/explicit copying, active/inactive attachment, byte/work refusal,
allocation-site failures, reclamation, exact retry, outer growth overlap,
detached/empty-map lifetime and allocation-free assignment/append/erase/swap.
Independent owner observation locates all retained field-map address requests,
including inactive slots, alongside occurrence paths. Existing native owner,
projection/catalogue/declaration and repeated-exchange controls remain active.

CPU native checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants plus six structural-recipe/
projection/membership/workbench CTests. Warm clients and warm-active integration
compile. Two server-free warm CTests pass: occurrence paths and address allocator/
identity/validation/parameter rendering/decoding via a local libpq result.
Six additional GCC-built pure checks pass using the smoke workflow's commands:
codec unit and adversarial, command IR, span planner, WAL recognition and report.
Their compiler version, build/run logs and binary hashes are retained. No GPU,
live database, full native baseline or separate full compiler audit rerun.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,155 / 1,240 | 211,372 | 293,322 | 4,102 |
| Four selections | 1,551 / 2,304 | 248,356 | 577,228 | 7,745 |

All owner failure sites reclaim observed storage. Whole-adoption retained/peak
requested bytes are 294,753 / 295,362 and 356,827 / 359,109; census 295,377 /
357,259. Enlarged address-container headers contribute throughout retained values,
not just newly gated field-map backing; allocation-site counts remain unchanged.
Projection setup boundaries remain 1,556 / 4,628, with work 53,060 / 141,190.
Selected/whole/census reports overlap and must not be summed. Allocation-request
work is not peak memory or CPU timing.

Next concrete slice: declaration source definition, parent target and ordered leaf
address backing during collection and explicit reverse copies. Other projection/
catalogue/recipe/input addresses, UUID payload, complete setup work and combined
caller/borrowed accounting remain open before operation integration and
same-execution controls. #156/#145/#229 remain open.

All 34 scoped checks pass: 20 exchange processes, six native supporting CTests,
two warm compatibility CTests and six pure codec/command/WAL executables. Each
process exits zero. No warnings were found in the build or runtime logs. Source
remained clean and unchanged throughout validation. Final changes after tested
source are six Markdown files only.

Contributor-local evidence: `map-address-domain-eb10ce19`, including the native
build/commands/raw logs/configuration/toolchain/runtime hashes plus the complete
compatibility sub-bundle with warm build, six pure builds, commands, logs and
binary hashes. Archive: `setup-evidence/map-address-domain-eb10ce19.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    c823d0449c1cf0a62124830c484fc5594ebd48d2d02838053d8fbb10660ca142

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Declaration address domain — contributor CPU evidence (2026-11-10)

Tested source: `ebb5e11680375851186c5e710a9519077f3faa0d`, stacked on
provisionally accepted #256 at `959142ac03a1aa2bb663bae6a431fe4481b42689`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/256#pullrequestreview-5843531653)
found no blocking source corrections and accepts this declaration-address slice.
Parent smoke 42834146582 and warm 42834146478 passed; reviewer-inspected warm
logs contain 90 Stage A, three pre-migration and 16 structural checks. Parent
native 42834146576 remains in progress at inspection. Dependency integration
is pending; integrated implementation acceptance remains through #246. Earlier
#255 build failures and #250 cache qualifications remain recorded above.

Source-definition, direct-parent-target and ordered-leaf addresses now attach to
the supplied domain before collection/copy allocation. Explicit reverse copies
retain the domain; ordinary snapshots detach and survive source-domain death.
Exact references, parent/leaf order, matching and detached standalone values
remain unchanged. This adds no type, codec, schema or numerical changes.
See [scope](../../docs/OWNER-RESOURCES.md#declaration-address-allocation-domain-contribution-awaiting-review).

Construction and explicit-copy controls include every address capacity in their
exact backing sums. Byte/work rejection sweeps reclaim all observed allocations;
exact retry preserves structural values. Move/snapshot controls check address
domain attachment and lifetime. Independent owner allocation observation locates
each retained address request, including reverse declarations. Existing whole-owner
failure-site injection and repeated native zero-new checks remain active.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline or separate compiler audit rerun; accepted #256 codec/warm compatibility
is not repeated for this declaration-only change.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,159 / 1,264 | 211,732 | 293,682 | 4,102 |
| Four selections | 1,551 / 2,400 | 249,796 | 578,668 | 7,745 |

All owner failure sites reclaim observed storage. Newly selected backing adds
360 / 1,440 bytes. Whole-adoption retained/peak requested bytes remain
294,753 / 295,362 and 356,827 / 359,109; census remains 295,377 / 357,259.
Selected-domain, whole-owner and census reports overlap and must not be summed.
Allocation-request work is not peak memory or CPU timing.

Next concrete slice: retained projection occurrence definitions, optional
provenance character/complete-character refs and active/spare parent field
addresses during explicit copies and parent-pool setup. UUID text, catalogue/
recipe/input payload, complete setup work and combined caller/borrowed accounting
remain open before remaining operations and same-execution controls. Deferred
activity/GPU sequencing is unchanged; #156/#145/#229 remain open.

All 26 scoped checks pass: 20 exchange processes and six supporting CTests.
Each process exits zero. No warnings were found in build or runtime logs. Source
remained clean and unchanged throughout validation. Final changes after tested
source are four Markdown files only.

Contributor-local evidence: `declaration-address-domain-ebb5e116`, including
commands, build/raw logs, source/configuration/toolchain and binary/runtime hashes.
Archive: `setup-evidence/declaration-address-domain-ebb5e116.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    577ba4ea6c24330036792e51add623bec1c198a21a471650279e17ecfb01fc52

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Projection address domain — contributor CPU evidence (2026-11-10)

Tested source: `80f2a43dea6eec4999f30dc51395e23604c88440`, stacked on
provisionally accepted #257 at `7ace326603b59de4dfb6514a1b8e1092a963e9f8`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/257#pullrequestreview-5843840961)
found no blocking corrections and accepts the projection-address slice, including
populated spare-pool copies. Parent smoke 42837316590 passed; native 42837316813
and #256 native 42834146576 remain running at review inspection. Dependency
integration is pending; integrated implementation acceptance remains through #246.
Earlier #255 build failures and #250 cache qualifications remain recorded above.

Both retained projections now attach occurrence-definition, optional byte-
provenance character/complete-character and active/spare parent-field addresses
before allocation. Explicit copies of populated spare pools and initial inactive-
pool setup use the same domain. Exact refs, field kinds, ordinals/counts, exposure
and pool order are unchanged. Ordinary snapshots detach, moves retain domain
lifetime and exchanges reuse existing backing. No new type/layout, codec, schema
or numerical change. See [scope](../../docs/OWNER-RESOURCES.md#projection-address-allocation-domain-contribution-awaiting-review).

Copy/reservation controls reconcile every address capacity with exact backing
sums and verify attachment/detachment. Structural comparisons check every
occurrence reference, optional provenance value and parent row in order.
Snapshots preserve those values after source-domain destruction; moves retain
populated parent pools. Byte/work refusal sweeps cover both ordinary source
copies plus reservation and already populated spare-pool copies, with complete
reclamation and exact retry. Independent whole-owner observation locates each
retained address request in both projections. Existing allocation-failure and
repeated native zero-new witnesses remain active.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline, separate compiler audit or repeated parent codec compatibility run.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,166 / 1,286 | 212,062 | 294,012 | 4,102 |
| Four selections | 1,585 / 2,488 | 251,116 | 579,988 | 7,745 |

All owner failure sites reclaim observed storage. Newly selected backing adds
330 / 1,320 bytes. Whole-adoption retained/peak requested bytes remain
294,753 / 295,362 and 356,827 / 359,109; census remains 295,377 / 357,259.
Selected-domain, whole-owner and census reports overlap and must not be summed.
Allocation-request work is not peak memory or CPU timing.

Next concrete slice: retained catalogue contribution field and pinned-target
addresses in `MembershipFields::collect`. Temporary graph key addresses, UUID
text, recipe/input payload, complete setup work and combined caller/borrowed
accounting remain open before remaining operations and same-execution controls.
Deferred activity/GPU sequencing is unchanged; #156/#145/#229 remain open.

All 26 scoped checks pass: 20 exchange processes and six supporting CTests.
Each process exits zero. No warnings were found in build or runtime logs. Source
remained clean and unchanged throughout validation. Final changes after tested
source are four Markdown files only.

Contributor-local evidence: `projection-address-domain-80f2a43d`, including
commands, build/raw logs, source/configuration/toolchain and binary/runtime hashes.
Archive: `setup-evidence/projection-address-domain-80f2a43d.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    da222cf737d9d5fe9a12a5c422360d4ef8d1bbafc6205fa976eedb4882b9c13d

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Catalogue address domain — contributor CPU evidence (2026-11-10)

Tested source: `7910e75d45f208a52f5eeed329825334bad15cc0`, stacked on
provisionally accepted #258 at `a10838d811e7f2de50f668cda3568a4d1ba8015a`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/258#pullrequestreview-5846910693)
found no blocking corrections and accepts retained catalogue addresses next.
Parent smoke 42868181997 passed; native 42868181981 remains running at review
inspection. #257 native 42837316813 has now passed; earlier pending-parent
statements are historical observations. Dependency integration remains pending;
integrated implementation acceptance remains through #246. Earlier #255 build
failures and #250 cache qualifications remain recorded above.

`MembershipFields::collect` now attaches retained contribution field-object and
pinned-target addresses before allocation. Finished rows move into the table;
target resolution remains one lookup per contribution. Exact scope/revision,
occurrence, field kind, counts and order are unchanged, as are graph traversal
and deduplication. Ordinary snapshots detach; moves retain domain lifetime.
No new type/layout, codec, schema or numerical change. See
[scope](../../docs/OWNER-RESOURCES.md#catalogue-address-allocation-domain-contribution-awaiting-review).

Catalogue controls reconcile both addresses with exact retained backing and
check attachment/detachment. Exact ordered rows are compared after source-domain
destruction, move and exact retry. Existing byte/work refusal sweeps reclaim all
observed allocations; whole-owner allocation-site failures and repeated native
zero-new controls remain active. Independent owner observation locates every
retained address request. The expanded membership byte total also verifies that
final strong release reclaims table/path/address backing while the control block
remains charged until final weak release.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline, separate compiler audit or repeated parent codec compatibility run.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,170 / 1,292 | 212,152 | 294,102 | 4,102 |
| Four selections | 1,589 / 2,512 | 251,476 | 580,348 | 7,745 |

All owner failure sites reclaim observed storage. Newly selected backing adds
90 / 360 bytes. Whole-adoption retained/peak requested bytes remain
294,753 / 295,362 and 356,827 / 359,109; census remains 295,377 / 357,259.
Selected-domain, whole-owner and census reports overlap and must not be summed.
Allocation-request work is not peak memory or CPU timing.

Next concrete slice: temporary catalogue graph-key addresses in `tools`, `state`,
`reached` and nested `Targets`, including ancestor propagation. Their node limits
do not cover nested addresses. UUID text, recipe/input payload, complete setup
work and combined caller/borrowed accounting remain open before remaining
operations and same-execution controls. Deferred activity/GPU sequencing is
unchanged; #156/#145/#229 remain open.

All 26 scoped checks pass: 20 exchange processes and six supporting CTests.
Each process exits zero. No warnings were found in build or runtime logs. Source
remained clean and unchanged throughout validation. Final changes after tested
source are four Markdown files only. The first source build at `81eb1a32` passed;
a pre-validation refinement avoided a duplicate target lookup, and the final
source was rebuilt before the single qualification run.

Contributor-local evidence: `catalogue-address-domain-7910e75d`, including initial/
final build scripts and logs, test commands/raw logs, source/configuration/toolchain
and binary/runtime hashes. Archive:
`setup-evidence/catalogue-address-domain-7910e75d.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    444901cf5771824d98af1480955badb5819f76d016fd3b401e27e500a169579a

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Maintainer integrated acceptance — #247 through #259 (2026-11-10)

Integrated at `d85213d2744a9ad4bb7cb733d7ad61dfef7e1ea9` through #259, reviewed head `a2636cad6e85b9582f7e37aa1dd537bbd7b5c430`. Native 42874078745 and smoke 42874078677 passed; all 12 native jobs succeeded. Inspected regression shards passed 29 + 29 + 29 + 28 = 115 tests; database log reports addressed_reads=24/private_reads=0.

Reviewed stack: #247, #250, #252, #253 (including its two Markdown-only handoff commits), #254–#259. The final contributor test pin is `7910e75d45f208a52f5eeed329825334bad15cc0`; its delta to reviewed #259 head is four Markdown files only. Contributor-local artifacts remain distinct from inspected hosted logs. Regression jobs: 130036766716, 130036766528, 130036766848, 130036766609. Database job: 130036766421. Compiler jobs all passed. No duplicate baseline was run.

Preserve earlier failures and warnings as recorded. This acceptance supersedes pending/integration status in earlier dated evidence without relabeling those observations. Remaining temporary graph addresses, UUID/recipe/input payload, complete setup and combined resource accounting, operation/control integration and GPU qualification remain open under #156/#145. Next bounded task: catalogue temporary graph-key addresses.

## Catalogue graph-address domain — contributor CPU evidence (2026-11-10)

Tested source: `26dc6b16af74a990f60ae65f247d8ae496b208f7`, based on integrated
main `f4907da096924599a850cca1d4ce97237bac23f0`. The #247/#250/#252–#259 stack
was merged at `d85213d2744a9ad4bb7cb733d7ad61dfef7e1ea9`; #259 native
42874078745 and smoke 42874078677 passed, including all 12 native jobs.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/259#pullrequestreview-5849105996)
found no blocking corrections and accepted this temporary-address slice;
[integration disposition](https://github.com/Human-Cognome-Project/human-cognome-project/pull/259#issuecomment-6778916353)
explicitly authorizes continuation from main. No accepted baseline was rerun.

Temporary `tools`/`state`/`reached` keys and nested `Targets` addresses now attach
before insertion. State/reached and target lookups avoid constructing known keys;
ancestor propagation explicitly constructs missing addresses rather than copying
a detached range. Tool duplicates preserve first-value behavior and reclaim their
temporary keys. Traversal, exact identity, deduplication and cycle rejection remain
unchanged. No new type/layout, codec, schema or numerical change. See
[scope](../../docs/OWNER-RESOURCES.md#catalogue-graph-address-allocation-domain-contribution-awaiting-review).

The diamond fixture independently identifies its five-element address requests
by their 15-byte size, distinct from its UUID text, paths, nodes/tables, comparator
text and callable requests. Optional nonthrowing test-only allocation callbacks
check prepayment and count acquired/released bytes and peak overlap. Four visited
vertices, tool keys and ancestor-set sizes 3+1+1+0 predict 570 temporary bytes;
retained contributions use 180 bytes. Measured address peak is exactly 750 bytes;
all 570 temporary bytes release before return, and all 180 retained bytes release
at destruction. This classification is fixture-specific, not an allocator or ABI
assumption for arbitrary production inputs.

The diamond also passes byte/work refusal, exact retry, all 1,414 observed
allocation-failure sites with complete reclamation, post-failure retry, duplicate
tool insertion and post-validation cycle rejection. Existing owner failure-site,
independent retained-request, strong/weak lifetime and repeated native zero-new
controls remain active. Comparator text, UUID payload, callable/traversal/stack
costs and complete setup accounting remain outside this new address gate.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline, separate compiler audit or repeated codec compatibility run.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,198 / 1,320 | 212,152 | 294,522 | 4,118 |
| Four selections | 1,617 / 2,540 | 251,476 | 580,768 | 7,761 |

The newly admitted transient address requests add 420 work bytes per owner fixture;
selected retained/peak totals remain unchanged because other phases dominate the
owner peak. Additional lookup comparisons create transient text requests (outside
the selected domain); observed whole-owner allocation sites rise by 16. No timing
or allocation-count improvement is claimed. Whole-adoption retained/peak requests
remain 294,753 / 295,362 and 356,827 / 359,109; census remains 295,377 / 357,259.
All failure sites reclaim storage. These overlapping reports must not be summed;
allocation-request work is not peak memory or CPU timing.

Initial source `b3e9ea1178c509e1110c6b2d8f582ec324a0f896` built, but all four
owner lanes stopped because the new structural fixture omitted explicit focus.
Six supporting CTests passed; later exchange cases were not reached. The corrected
fixture supplies a membership focus. Failed raw evidence is retained separately
as `catalogue-graph-address-domain-b3e9ea11`, archive
`setup-evidence/catalogue-graph-address-domain-b3e9ea11-failed.tar.gz`, verified
manifest SHA-256:

    8d12dac1bbe2069c9ab6a9a698d55483c4c7e591984628d3c33d48255cd02c9e

At corrected source all 26 scoped checks pass: 20 exchange processes and six
supporting CTests, all exit zero, with no build/runtime warnings. Source remained
clean and unchanged during validation. Final delta is four Markdown files only.
Next: retained projection recipe addresses; recipe paths/tables, UUID/text/input
payload, complete setup, combined resources and operation/control integration
remain open. Deferred activity/GPU sequencing is unchanged.

Final contributor-local evidence: `catalogue-graph-address-domain-26dc6b16`,
including commands, build/raw logs, source/configuration/toolchain and binary/
runtime hashes. Archive:
`setup-evidence/catalogue-graph-address-domain-26dc6b16.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    f3cd38ad26c5e0e92e19d2c8fefc2c9a14ecb5103f0936e4209c5d2949dc5977

Raw evidence remains local; these hashes do not imply independent remote inspection
or replace authoritative hosted checks.

## Retained recipe address domain — contributor CPU evidence (2026-11-10)

Tested source: `64e85a5fcdcdc9da719def6f8d49f585ada61e06`, stacked on
provisionally accepted #261 at `942c93aaa5f4487bd7abcecd0b6f9fb4cca64c9e`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/261#pullrequestreview-5849608177)
found no blocking corrections and accepted this recipe-address slice. Parent
smoke 42903689287 passed; native 42903689200 remains running at review inspection.
Integrated acceptance remains through #259. The earlier #261 missing-focus fixture
failure remains preserved in its separate source-pinned record above.

Explicit owner copies now build addressed recipe rows directly in the supplied
domain: tool refs, parent targets, member-of refs, geometry, legacy/structural
occurrence definitions and focal-field objects. No detached whole-recipe copy
precedes admission. Finished rows move into reserved ordinary recipe vectors;
those tables, recipe paths and text remain separately scoped. Every Bundle field
and pin is preserved. No new type/layout, codec, schema or numerical change.
See [scope](../../docs/OWNER-RESOURCES.md#recipe-address-allocation-domain-contribution-awaiting-review).

Projection address visitors now cover recipe addresses in exact backing totals,
nested attachment/detachment, moved lifetime and independent retained requests.
The nonallocating equivalence check compares every recipe field, including tool
metadata/optional values, parent/membership order, geometry/spheres, nodes,
legacy positions/velocities, selections, contacts and focal fields. Structural
and populated spare-pool controls retain byte/work refusal, reclamation, exact
retry and source-death snapshot checks. A validated legacy fixture with mixed
exposures, nonzero state/geometry and populated contacts runs the same copy and
lifetime controls without structural-only pool reservation. Whole-owner allocation
failures and repeated native zero-new witnesses remain active.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline, separate compiler audit or repeated codec compatibility run.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,267 / 1,458 | 214,222 | 296,592 | 4,118 |
| Four selections | 1,695 / 2,696 | 253,816 | 583,108 | 7,761 |

Newly selected backing adds 2,070 / 2,340 bytes. Whole-adoption retained/peak
requested bytes remain 294,753 / 295,362 and 356,827 / 359,109; census remains
295,377 / 357,259. Allocation-site counts are unchanged and all failure sites
reclaim storage. Selected-domain, whole-owner and census reports overlap and
must not be summed; allocation-request work is not peak memory or CPU timing.

Next: retained recipe paths in legacy/structural occurrence IDs, selection parts
and contact endpoints. Recipe tables, UUID/text/input payload, complete setup and
combined resources remain open before operation/control integration. Deferred
activity/GPU sequencing is unchanged; #156/#145/#229 remain open.

All 26 scoped checks pass: 20 exchange processes and six supporting CTests.
Each process exits zero; no build/runtime warnings. Source remained clean and
unchanged during validation. Final delta is four Markdown files only.

Contributor-local evidence: `recipe-address-domain-64e85a5f`, including commands,
build/raw logs, source/configuration/toolchain and binary/runtime hashes. Archive:
`setup-evidence/recipe-address-domain-64e85a5f.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    6d002e0ca6c5760030ba908d92b301d2ee028c0779e737439d126a4bc628f1bf

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Retained recipe path domain — contributor CPU evidence (2026-11-10)

Tested source: `8c66c95ee51916271e2a6b9daa6193c629e50147`, stacked on
provisionally accepted #262 at `3e04a02b07a4aa2872e2d974c39cc0d98b01a7da`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/262#pullrequestreview-5850532407)
found no blocking corrections and accepted this recipe-path slice. Parent smoke
42912109232 passed; native 42912109277 remains running at review inspection.
Integrated implementation acceptance remains through #259. The earlier #261
missing-focus fixture failure remains preserved in its separate record above.

Explicit owner recipe copies now construct legacy/structural occurrence paths,
every selection part path and both contact endpoint paths with the destination
domain before copying ordinals. The shared projection-ID helper preserves entry
numbers; finished IDs and rows move into their tables. Empty paths retain the
domain without an ordinal allocation. Recipe addresses and all other values
remain unchanged. No type/layout, codec, schema or numerical change. See
[scope](../../docs/OWNER-RESOURCES.md#recipe-path-allocation-domain-contribution-awaiting-review).

Expanded path visitors include all recipe paths in exact selected backing,
attachment/detachment, moved lifetime and source-death snapshot controls.
Independent observation checks actual path allocation requests. Complete recipe
comparisons preserve entries, order, metadata and contact semantics. Validated
legacy and structural fixtures now include contacts with nonempty paths at both
ends, as well as empty aggregate endpoints, occurrence IDs and selection parts.
Both versions run refusal/reclamation/exact-retry and lifetime controls; populated
spare-pool, whole-owner failure injection and repeated native zero-new witnesses
remain active.

CPU checks use Clang/LLVM 15, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, workers 1/2 × bulk/default or
direct/compiler-no-unroll on the shared personal host. Scope: owner, multiple
owner, bindings, commit and fields × four variants, plus six structural-recipe,
projection, membership and workbench CTests. No GPU, live database, full native
baseline, separate compiler audit or repeated codec compatibility run.

| Fixture | Owner byte/work boundaries | Selected live/peak bytes | Allocation-request work | Failure sites |
|---|---:|---:|---:|---:|
| One selection | 1,271 / 1,466 | 214,286 | 296,656 | 4,118 |
| Four selections | 1,711 / 2,728 | 254,072 | 583,364 | 7,761 |

Newly selected backing adds 64 / 256 bytes. Whole-adoption retained/peak requested
bytes remain 294,753 / 295,362 and 356,827 / 359,109; census remains 295,377 /
357,259. Allocation-site counts are unchanged and all failure sites reclaim
storage. Selected-domain, whole-owner and census reports overlap and must not
be summed; allocation-request work is not peak memory or CPU timing.

Next: the eight top-level retained Bundle table stores, with engine-independent
allocator support and explicit C++ container/layout compatibility qualification.
Nested recipe tables, UUID/text/input payload, complete setup and combined
resources remain open before operation/control integration. Deferred activity/
GPU sequencing is unchanged; #156/#145/#229 remain open.

All 26 scoped checks pass: 20 exchange processes and six supporting CTests.
Each process exits zero. No build warnings were found. The two-worker direct
commit case (`2-direct-exchange_commit.log`) emitted a post-PASS Taichi offline-
cache lock warning at `offline_cache.h:run@187` for the shared `ticache.lock`.
The warning is preserved; no cache cleanup or repeat run was used to suppress it,
and no performance qualification is claimed. Source remained clean and
unchanged during validation. Final delta is four Markdown files only.

Contributor-local evidence: `recipe-path-domain-8c66c95e`, including commands,
build/raw logs, source/configuration/toolchain and binary/runtime hashes. Archive:
`setup-evidence/recipe-path-domain-8c66c95e.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    8796ef6e9e153b491b680a08ad2b26819c98d2307b7cbfa5dfc7557d3672d5d9

Raw evidence remains local; this hash does not imply independent remote inspection
or replace authoritative hosted checks.

## Retained recipe table domain — contributor CPU evidence (2026-11-10)

Tested source: `73f015f1df59aaede65f6cfc2f5c056204e501cd`, stacked on
provisionally accepted #263 at `8647cfc77fd00f9cb2cf7ac0a8b3d9619c06dfdf`.
[Review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/263#pullrequestreview-5851136271)
found no blocking source findings and approved this eight-table slice. Parent
smoke 42917037200 and native 42917037269 are now confirmed passed; #262 native
42912109277 passed as reported in that review. Integrated implementation
acceptance remains through #259 pending stack integration. Earlier pending
observations, #261's fixture failure and #263's post-PASS cache-lock warning
remain historical evidence, not superseded results.

All eight top-level Bundle stores now use engine-independent RecipeTable storage.
Explicit projection copies attach the destination domain before reserve and move
completed rows with attached addresses/paths. Nodes also construct in that domain.
Ordinary snapshots detach recursively; moves retain domain/backing. Complete
values/order and encoding remain unchanged. See [scope](../../docs/OWNER-RESOURCES.md#recipe-table-allocation-domain-contribution-awaiting-review).

This is a C++ container/layout change, requiring rebuilds. The pinned x64
Clang 15/libstdc++ layout probe measures ordinary/table headers at 24/40 bytes,
and prior/current Bundle at 328/456 bytes (+128). Iterator conversions adapt the
legacy focal adapter and PostgreSQL fixture. Prepared contacts use RecipeTable;
the structural adapter test explicitly converts to the unchanged std::vector
parameter. Full configured native targets and standalone warm targets build,
including warm_active_integration, warm_postgres_test, structural_postgres_test
and recipe_reader_probe. These client builds do not qualify live database I/O.

The table visitor includes all eight stores in selected backing, attachment,
snapshot/move lifetime and independent actual-request checks. Validated legacy
and structural fixtures preserve complete recipe values, both contact endpoints
and empty version-specific tables. Existing copy charge-boundary sweeps,
refusal/reclamation/exact retry, populated inactive pools, whole-owner failure
injection and repeated native zero-new controls remain active.

CPU results (workers 1/2, bulk/default and direct/compiler-no-unroll):

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1279 / 1478 | 1718 / 2740 |
| Selected live / peak requested bytes | 232910 / 232910 | 275256 / 275256 |
| Selected cumulative allocation work | 315280 | 604548 |
| Whole-owner allocation sites, all reclaimed | 4118 | 7761 |
| Whole-owner retained / peak requested bytes | 295009 / 295618 | 357083 / 359365 |
| Retained capacity census | 295633 | 357515 |

Selected totals increase by 18624 / 21184 bytes, of which 18368 / 20928 is
newly selected existing table backing and 256 is the two larger retained Bundle
headers. Whole-owner retained/peak and census increase by 256; allocation-site
counts are unchanged. Selected reports overlap the census and must not be added.
No allocator-metadata, physical-residency or performance qualification is implied.

Validation outcome: all 40 scoped checks passed, exit zero, with no warnings in the final build/run logs. Source remained clean and pinned;
final tested-to-head changes are documentation only. The 20 exchange processes
cover owner, owner_multiple, bindings, commit and fields in four configurations.
Supporting checks comprise six projection/membership/workbench/structural CTests,
12 prepared-execution/update CTests and two standalone warm allocation CTests.
No GPU, live database, full baseline or repeated codec qualification was run.

Two earlier compiler failures are preserved, not counted as passes:

- `0faa6998`: structural recipe test passed the new focal table directly to the
  unchanged std::vector adapter parameter; corrected by explicit iterator conversion.
  Standalone warm build and two tests passed at this source; native execution was
  not reached. Archive `setup-evidence/recipe-table-initial-0faa6998.tar.gz`;
  manifest SHA-256 `a794f1481ef50c7a2940a6dcf7c5adcd0ab425e562fff641568a8f8e1e49c6e2`.
- `aa09786c`: broad native rebuild exposed prepared_sources.h assigning an ordinary
  contact vector into the recipe table; corrected with RecipeTable fixture values.
  Native execution was not reached. Warm logs retained in this failure directory
  are earlier unchanged logs from the first source, not a second run at aa09786c.
  Archive `setup-evidence/recipe-table-consumers-aa09786c.tar.gz`;
  manifest SHA-256 `5672c7e0d8acdec4b4f6492a963e4d603bedada651636c0e9a32b63bc4cc762b`.

Final local evidence: `recipe-table-domain-73f015f1`, including commands, raw logs,
source/configuration/toolchain, layout probe and binary/runtime hashes. Archive
`setup-evidence/recipe-table-domain-73f015f1.tar.gz`. Verified SHA256SUMS manifest:

    3529818cef1fde7b0de182854d3cce4a381a1b1008ae4351f2624fceb0a0c169

Raw evidence remains local, not independently inspected remotely. Hosted checks
remain authoritative. Next: nested Tool.parents/member_of, Geometry.spheres and
Selection.parts tables, with before-allocation attachment and C++ compatibility
qualification. UUID/text/input payload, full setup and combined resources remain
open before operation/control integration. Activity/GPU sequencing is unchanged;
#156/#145/#229 remain open.


## Nested retained recipe table domain — contributor CPU evidence (2026-11-10)

Tested source: `efefae4373037c1b2c81935b02be97c81d94ab79`, stacked on #264
at `f52506c2f4a4e3ee5a683f1624651330763ec650`. #263 is maintainer-approved
in the continuation instruction but remains open/unmerged at inspection. #264
has no review notes at inspection and is not labeled accepted. Its smoke
42925890864 and warm 42925891058 passed; native 42925891067 was running.
Integrated implementation acceptance remains through #259 pending stack integration.

Tool.parents, Tool.member_of, Geometry.spheres and Selection.parts now use the
same engine-independent RecipeTable type as top-level Bundle stores. Explicit
projection copies select the destination domain before reserve, then move rows
with attached nested addresses/paths. Spheres copy directly into attached storage.
Ordinary snapshots detach recursively; moves preserve domain/backing. Empty nested
tables retain the allocator without element allocation. Complete recipe values,
ordering, optional state, encoding and numerical behavior remain unchanged.

Recursive nonallocating test visitors now include every top-level/nested table
in exact selected backing, allocator attachment and independent allocation-request
checks for both owner projections. Validated v1/v2 fixtures exercise complete
recipe equivalence, charge-boundary refusal/reclamation/exact retry, snapshots
after source death, moved lifetime and populated inactive pools. Whole-owner
allocation-failure injection and repeated native zero-new controls stay active.

C++ nested container types and row layouts change and require rebuilding consumers.
The pinned Clang 15/libstdc++ x64 layout probe measures Tool 304 -> 336 bytes,
Geometry 120 -> 136, Selection 40 -> 56; Bundle stays 456 bytes. The legacy
validator's expected-parts temporary uses detached RecipeTable for same-type exact
comparison; its validation semantics are unchanged. Full configured native and
standalone warm targets build, including warm_active_integration and PostgreSQL
clients/probe. These builds do not qualify live database I/O or a stable C++ ABI.

CPU results (workers 1/2; bulk/default and direct/compiler-no-unroll):

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1311 / 1538 | 1754 / 2806 |
| Selected live / peak requested bytes | 243502 / 243502 | 286520 / 286520 |
| Selected cumulative allocation work | 325872 | 615812 |
| Whole-owner allocation sites, all reclaimed | 4118 | 7761 |
| Whole-owner retained / peak requested bytes | 296961 / 297570 | 359131 / 361413 |
| Retained capacity census | 297585 | 359563 |

Selected totals grow by 10592 / 11264 bytes versus #264: 8640 / 9216 newly
selected existing nested backing plus 1952 / 2048 larger retained row headers.
Whole-owner retained/peak and census grow by 1952 / 2048; allocation-site counts
are unchanged. Selected reports overlap the retained census and must not be added.
No allocator-metadata, physical-residency or performance claim follows.

Validation outcome: all 40 scoped checks passed, exit zero, with no warnings in the final build/run logs. The matrix comprises 20 exchange
processes (owner, owner_multiple, bindings, commit, fields across four CPU modes),
six projection/recipe CTests, 12 prepared-execution/update CTests and two standalone
warm allocation CTests. Source remained clean/pinned; tested-to-head delta is
four Markdown files only. No GPU, live database, full baseline or repeated codec
qualification was run.

The initial `a66e7dd5` native build failed because two converted callback visitor
calls lacked closing syntax. Corrected in `efefae43`; no runtime matrix or warm
build was reached in that failed attempt. Preserve its raw build log/source pin in
`setup-evidence/recipe-nested-table-initial-a66e7dd5.tar.gz`, manifest SHA-256:

    24a36e4b7a4983ca8f2f1d93786332a6801a1f960934d1ac2cc04964df1a62ab

Earlier #261 fixture failure, #263 post-PASS cache warning and #264 consumer
compile failures remain preserved in their own records, not relabeled as passes.

Final contributor-local evidence: `recipe-nested-table-domain-efefae43`, including
commands, raw build/run logs, source/configuration/toolchain, binary/runtime hashes
and layout probe. Archive `setup-evidence/recipe-nested-table-domain-efefae43.tar.gz`.
Verified SHA256SUMS manifest SHA-256:

    378138ddc3a9fd34d39f2d976558ed442545e1f5619933eb1eae39df6b8bc3f2

Raw evidence remains local, not independently inspected remotely. Hosted checks
remain authoritative. Next: Tool.recipe and optional Tool.notation text payload,
including short/long strings, terminators, exact text/optional semantics, detached
snapshots and before-allocation attachment. UUID/input payload, full setup and
combined resources remain before operation/control integration. Activity/GPU
sequencing is unchanged; #156/#145/#229 remain open.


## Retained recipe text domain — contributor CPU evidence (2026-11-11)

Tested source: `fe064fcc275dc910d446263c5fe42eea23f8a9cc`, stacked on
provisionally accepted #265 at `21ae236fe19f8c4721ef46fbc602004014e730cb`.
[Review 5851571884](https://github.com/Human-Cognome-Project/human-cognome-project/pull/265#pullrequestreview-5851571884)
found no blocking source findings and approved this recipe/notation text slice.
#264 is also provisionally accepted by review 5851571748; earlier awaiting-review
statements remain historical. #265 smoke 42928854772 and warm 42928854774 passed;
native 42928854860 was running at review inspection. Integrated implementation
acceptance remains through #259 pending stack integration.

Tool.recipe and optional Tool.notation use RecipeText with the existing shared
reservation allocator. Explicit owner copies select the domain before heap
character allocation. Heap backing includes capacity plus terminator; inline
characters are already in row headers and incur no separate request. The helper
detects inline storage by data location within the object rather than a fixed
capacity threshold. Retained census excludes inline recipe text; logical input
copy admission excludes text fitting the default inline capacity and otherwise
charges size plus terminator. Other UUID/input string accounting is unchanged.

Absent/empty optional state, exact bytes, ordinary detached copies and moved
lifetime are preserved. Warm validation uses string views; SQL parameters,
retained-data fixtures and viewport labels/capture convert explicitly between
recipe and ordinary strings. Canonical immutable-body encoding and numerical
behavior remain unchanged. Full configured native and standalone warm builds
include PostgreSQL clients/probe, warm_active_integration and viewport capture.
These builds do not qualify live database access or rendering.

C++ string/Tool layout changes require consumer rebuilding. Pinned Clang 15 with
libstdc++ x64 measures ordinary/recipe strings at 32/48 bytes, optional strings at
40/56, Tool 336 -> 368, Bundle unchanged at 456. Default inline capacity is 15;
131 characters request 132 bytes. No other-library ABI/layout claim is made.

Independent request observation covers terminators and zero separate inline
backing. Validated v1/v2 fixtures add unreferenced tools with absent, empty,
escaped short and long notation/recipe text without altering the requested letter
graph. Complete recipe and tool_body comparisons, snapshots after source death,
moves, all copy charge-boundary refusals/reclamation/exact retries, populated pools
and whole-owner failure/repeated native zero-new controls remain active. The new
standalone recipe_text_test checks exact escaped serialization and optional
states, short/long requests, detached/moved lifetime and terminator refusal/retry.

CPU results (workers 1/2, bulk/default and direct/compiler-no-unroll):

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1334 / 1584 | 1777 / 2852 |
| Selected live / peak requested bytes | 245986 / 245986 | 289004 / 289004 |
| Selected cumulative allocation work | 328356 | 618296 |
| Whole-owner allocation sites, all reclaimed | 4118 | 7761 |
| Whole-owner retained / peak requested bytes | 298433 / 299042 | 360603 / 362885 |
| Retained capacity census | 298321 | 360299 |

For both fixtures, selected totals increase by 2484 bytes: 1012 existing heap
character bytes newly selected plus 1472 larger Tool headers. Whole-owner retained/
peak increase by 1472. Census increases by 736: +1472 row headers minus 736 formerly
counted inline recipe characters. Allocation-site counts are unchanged. The census
is not a complete allocator/physical-memory bound; selected reports overlap it and
must not be summed. UUID/input/metadata and other resource domains remain separate.

Validation outcome: all 42 scoped checks passed, exit zero, with no warnings in the final build/run logs. The 42 scoped checks comprise 20
exchange processes, seven recipe/projection/viewport CTests, 12 prepared-execution/
update CTests and three standalone warm CTests. Source remained clean/pinned;
tested-to-head delta is four Markdown files plus the viewport runner pin described below. No GPU, live database, full
baseline, browser rerun, physical-residency or performance qualification.

Preserved compilation failures (neither reached runtime qualification or the
standalone warm build in its failed script):

- `3728945d`: warm_active_integration passed ordinary optional notation to the
  new type without an explicit conversion. Corrected there and in the structural
  PostgreSQL fixture. Archive `recipe-text-initial-3728945d.tar.gz`; manifest:
  `694cc64df8bf98cb5954d49c78b4ad97f0c1292ab373aad1c0e4dc7fa2b548bb`.
- `8be6d61f`: letter_projection_test pushed recipe text into an ordinary string
  vector without explicit conversion. Corrected it and the viewport capture/label
  boundaries. Archive `recipe-text-consumers-8be6d61f.tar.gz`; manifest:
  `cf824a8ad228a3119ac490a468a9d6ed7710fd1ac12b0ac7320e353d3a35c9f8`.

A rebuild invocation was not executed when automatic approval review returned an
internal Bad Request error. The same authorized invocation succeeded on retry;
this was an approval-service error, not a source/test result. Earlier parent
compilation/fixture failures and cache warning remain preserved in their records.

Final local evidence: `recipe-text-domain-fe064fcc`, with commands, raw logs,
source/configuration/toolchain, binary/runtime hashes and layout probe. Archive:
`setup-evidence/recipe-text-domain-fe064fcc.tar.gz`. Verified SHA256SUMS manifest:

    e2d00f171c9364c64c94e3baa805ebc2ac5bc87212e06cf46ba0e525a1c87286

Raw local artifacts are not independently inspected remotely; hosted checks
remain authoritative. Next: retained recipe UUID text, beginning with Uuid/str()
and serialization consumer audit. Other UUID/input payload, full setup and
combined resources remain before operation/control integration. Activity/GPU
sequencing is unchanged; #156/#145/#229 remain open.

Hosted follow-up at `dc368c594a0d4900a83facf0360c07b4147f31a5`: smoke
42934750915 passed. Viewport run 42934750737 / job 130241920643 failed during
Playwright installation, before browser execution: ubuntu-latest selected Ubuntu
26.04.1, unsupported by pinned Playwright 1.56.1. Its 15 model tests, build and
retained bundle comparison passed; browser tests were skipped. This is preserved
as a hosted failure, not a browser pass. The [job log](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/42934750737/job/130241920643)
was personally inspected. A separate follow-up commit pins only this workflow to
Ubuntu 24.04, matching the native/warm baseline and the [pinned Playwright dependency map](https://github.com/microsoft/playwright/blob/v1.56.1/packages/playwright-core/src/server/registry/nativeDeps.ts#L452).
Browser acceptance awaits the new hosted run. Native source and the 42 local CPU
results are unchanged; no accepted local matrix was rerun for this CI-only repair.
### Retained recipe UUID allocation-domain qualification (2026-11-11)

Tested source: `dcc13ea368890f34f3a2744bf7de5fafd6963b23`, stacked on
provisionally accepted #266 at `97ab2aafd4406f4d049397509de1ca4ccf54d4e1`.
Review 5851957425 reported no blocking findings and approved this bounded slice.
#266 smoke, replacement viewport and warm workflows passed; native was still
running at inspection. The replacement viewport pass supersedes the earlier
pending browser status, without erasing the preserved runner-install failure.
Integrated acceptance remains through #259.

Recipe UUID characters now select the reservation domain before heap allocation.
Normalization, malformed input, nil/non-nil values, comparison and SQL encoding
are preserved. Detached copies and moved/source-death lifetimes are covered.
Uuid/str() and serialization consumers were audited; containing rows grow even
where their UUID payload remains outside this selected scope. Other projection,
owner and input UUID storage is not claimed as admitted.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1498 / 1924 | 1934 / 3210 |
| Selected live / peak requested bytes | 280150 / 280150 | 331994 / 331994 |
| Selected cumulative allocation work | 368024 | 683606 |
| Whole-owner allocation sites, all reclaimed | 4118 | 7761 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree (workers 1/2, bulk/default and
 direct/compiler-no-unroll). Allocation-site counts are unchanged from #266.
Selected live growth is 34164 / 42990 bytes; whole-owner retained growth is
21584 / 29744 bytes. Selected reports overlap the census and must not be summed;
neither is a complete physical-memory bound.

Full configured native and standalone warm builds passed on the first attempt.
All 43 scoped checks passed: 20 exchange processes, 12 prepared-execution/update
CTests, seven supporting recipe/projection/viewport CTests and four standalone
warm CTests. Every injected owner allocation failure reclaimed its resources.
Final build/run logs have no warnings. Source remained pinned/clean; the
 tested-source-to-head delta is four Markdown files only. CPU/x64, Clang 15,
libstdc++, shared host; no GPU, live database, browser rerun, full baseline or
performance qualification. Earlier failures/warnings remain in prior records.

Evidence directory `recipe-uuid-domain-dcc13ea3` retains commands, raw logs,
configuration, compiler/source/binary/runtime hashes and layout probe. Local
archive: `setup-evidence/recipe-uuid-domain-dcc13ea3.tar.gz`. Verified SHA256SUMS:

    c34252001d6a76c061fbcd51cd07e559f874a0898eaee1b38d189e77c80b916c

Raw local artifacts have not been independently inspected remotely. Hosted
checks remain authoritative. Next bounded scope: remaining retained projection
UUID fields; other owner/input payload, complete setup, combined resources and
operation/control integration remain separate.


### Maintainer integration through #267 (2026-11-11)

Review 5858105951 accepts head `7d9a7e484d923d17edc223d95007171b273c2680`
without blocking findings. Reviewed #261–#267 are integrated at
`4c0c3c56ea83c7868a62c7dd41de10d183053ec5`, preserving #260's covenant addition.
Independently verified: local tested source dcc13ea3 to reviewed head changes
only four Markdown files. Raw local archives were not inspected or rerun.

Hosted native 42937702239 passed all 12 jobs. Inspected regression jobs
130254017744/130254017750/130254017788/130254017971 passed 29/28/29/29 tests
(115 total); database job 130254017673 reports addressed_reads=24/private_reads=0.
Warm 42937702260 passed; job 130251750153 reports four CTests and 90 Stage A
PostgreSQL acceptance checks. Viewport 42937702136 passed; job 130251750116
includes 15 model tests and successful direct-file navigation. Smoke 42937702124
passed. These supersede pending hosted status observations for the integrated
stack, without erasing earlier failures. No GPU or performance claim follows.

Next authorized slice: remaining retained projection UUID payload, as listed in
the continuation plan. Other owner/input payload, setup, combined resources and
operation/control integration remain open.

### Remaining projection UUID qualification (2026-11-11)

Base: accepted integrated main `427977d8`; #267 review 5858105951 explicitly
approves this slice. Tested source: `938d8ef507bea11d45149ee7fc4b9e569da000ae`.
The reviewed #261–#267 stack is merged; no dependent provisional stack is needed.

Explicit copies select the destination domain before allocating retained study,
occurrence/provenance owner-study, occurrence/byte/complete-character reference,
and active/spare parent study/field UUID characters. Reserving inactive parents
uses the same helpers. Recipe/address/path/table/text attachment is preserved.
The visitor independently checks these backing requests, their terminators and
allocator identity. Existing v1/v2 controls cover complete values, detached
snapshots, source-death moves, populated spare-pool copies, every byte/work
refusal boundary, reclamation and exact retry. Repeated owner exchange retains
active/spare objects rather than recreating their UUID storage.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1518 / 1980 | 2023 / 3428 |
| Selected live / peak requested bytes | 282222 / 282222 | 340060 / 340060 |
| Selected cumulative allocation work | 370096 | 691672 |
| Whole-owner allocation sites, all reclaimed | 4118 | 7761 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree. Compared with #267, selected live/peak/work
increase by 2072 / 8066 bytes. Whole-owner requests, allocation-site counts and
capacity census are unchanged. Uuid and containing-row layout remain unchanged;
this admits existing storage, not additional storage. Selected reports overlap
the census and must not be summed; neither proves complete physical-memory bounds.

Full native and standalone warm builds passed first attempt. All 43 scoped CPU
checks passed: 20 exchange processes, 12 prepared CTests, seven supporting CTests
and four warm CTests. Workers 1/2 and bulk/default or direct/compiler-no-unroll
were exercised. Tested source stayed clean/pinned; final delta is four Markdown
files only. Pinned Clang 15/libstdc++ x64 and Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no performance claim.

One warning is preserved in `2-bulk-exchange_commit.log`: line 202 reports PASS;
line 203 reports Taichi offline-cache lock failure. The process exited zero.
No cache cleanup or warning-erasing rerun was performed. No other warning/error
was found in final build/run logs. Earlier historical failures remain preserved.
A restricted Windows submodule-status read hit MSYS access denied; an authorized
retry succeeded. This was not a source/build/test failure.

Evidence directory `projection-uuid-domain-938d8ef5` contains commands, raw logs,
source/configuration/toolchain, binary/runtime hashes and layout measurement.
Verified local archive: `setup-evidence/projection-uuid-domain-938d8ef5.tar.gz`.
SHA256SUMS manifest digest:

    5468d4daca1d6ea074646121bdd4cb2bedfb342acc328d1a32239a631ea068c1

Raw artifacts are local and have not been independently inspected remotely;
hosted checks remain authoritative. No full baseline or browser rerun was made.
No GPU or live database work was requested. Declaration/catalogue/map/other owner
UUIDs and input payload remain separate from this contribution. Next bounded
scope is retained declaration UUIDs, before full setup/combined resource work
and operation/control integration.

### Retained declaration UUID qualification (2026-11-11)

Tested source `dbfd06463c25514dadc6dcdf06c666796157b52c`, stacked on #268 head
`864eabe731f3238302ac6505516fa6cad4bd4e8e`. Review 5858272490 reported no blockers
and explicitly authorized this slice. Parent smoke 43015222103 passed; native
43015222274 was running at inspection. Integrated acceptance remains through
#267, and #268's post-PASS cache warning remains preserved.

Collection directly initializes owner/recipe identities with the destination
domain instead of allocating detached defaults and then replacing them. Explicit
copies also attach before allocation. Outgoing/incoming source study IDs,
source/parent/leaf reference scopes and revisions use the same explicit copies.
Public detached declarations, complete structural values and matching are unchanged.

Both collection and copy controls observe each UUID request independently,
including its terminator and allocator; retained live requests equal domain bytes.
Zero byte/work budget refuses before any allocation attempt. Boundary sweeps,
complete-value comparison, detached snapshots, source-death moves, reclamation
and exact retry remain active. Whole-owner observations and every allocation
failure-injection site include the new selected payload.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1551 / 2040 | 2038 / 3668 |
| Selected live / peak requested bytes | 284442 / 284442 | 348940 / 348940 |
| Selected cumulative allocation work | 372316 | 700552 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree. Selected live/peak/work increase by
2220 / 8880 bytes from #268. Whole-owner retained/peak requests and capacity census
are unchanged; allocation-site counts decrease by 1 / 4 after eliminating
replaced default identity storage. UUID and containing-row layout are unchanged.
The selected report overlaps the census and must not be added to it.

Full configured native and standalone warm builds passed first attempt. All 43
scoped CPU checks passed: 20 exchange processes (workers 1/2, bulk/default and
direct/compiler-no-unroll), 12 prepared CTests, seven supporting CTests and four
warm CTests. Final build/run logs contain no warnings/errors. Source remained
clean/pinned; tested-source-to-final-head delta is four Markdown files only.
Pinned Clang 15/libstdc++ x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, shared host; no timing claim.

Evidence directory `declaration-uuid-domain-dbfd0646` retains commands, raw logs,
source/configuration/toolchain, binary/runtime hashes and layout measurements.
Verified local archive: `setup-evidence/declaration-uuid-domain-dbfd0646.tar.gz`.
SHA256SUMS manifest digest:

    6d93c65ced4843aeb9b744df3cee41856bb6b6e877b34221868546efeeb70f97

Local raw artifacts have not been independently inspected remotely; hosted
checks remain authoritative. Historical failures/warnings remain in their
original records; this run does not relabel #268's warning as warning-free.

Next: catalogue UUIDs, auditing retained fields and temporary graph keys
separately. Map/other owner UUIDs, input payload, full setup, combined accounting
and operation/control integration remain open. No GPU, live database, browser
rerun, full baseline or performance qualification is claimed.

### Retained catalogue UUID qualification (2026-11-11)

Tested source `14e34f0d606cb26c7e4a84617e9976835f3fb737`, stacked on #269 head
`f6830b9d404126cb14ee3a2a07db9896cee47936`. Review 5858547912 found no blockers
and authorized a catalogue UUID audit with retained/transient scopes distinguished.
This contribution selects retained payload only. #269 smoke 43017931238 passed;
native 43017931472 was running at inspection. #268/#269 remain provisional;
integrated acceptance is through #267. Earlier warnings/failures remain preserved.

Study/recipe UUIDs initialize directly with their destination allocator; retained
contribution occurrence study, field scope, target scope and revision attach
before character allocation. Temporary graph-key UUIDs deliberately remain
detached; their existing address/node attachment is preserved. Public detached
collection, graph semantics, per-occurrence deduplication and exact values remain
unchanged. Equivalence now also checks recipe identity and the alternatives flag.

The retained UUID visitor feeds independent request/terminator/domain checks,
exact observed retained-byte equality, snapshot/source-death/move controls,
byte/work refusal/reclamation/exact retry and whole-owner failure injection.
Zero allowance refuses before allocation. Existing diamond, duplicate graph-key
and cycle witnesses run unchanged, preserving temporary-address qualification.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1562 / 2055 | 2053 / 3719 |
| Selected live / peak requested bytes | 284997 / 284997 | 350827 / 350827 |
| Selected cumulative allocation work | 372871 | 702439 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree. Selected live/peak/work increase by 555 / 1887
bytes from #269. Whole-owner retained/peak requests, allocation-site counts and
census are unchanged, as are UUID and containing-row layouts. These reports
overlap and must not be summed; they are not physical-memory/performance bounds.

Full configured native and standalone warm builds passed first attempt. All 43
scoped CPU checks passed: 20 exchange processes across workers 1/2 and bulk/default
or direct/compiler-no-unroll, 12 prepared CTests, seven supporting CTests and four
warm CTests. Final build/run logs contain no warnings/errors. Source stayed clean
and pinned; final delta is four Markdown files only. Pinned Clang 15/libstdc++ x64,
Taichi `5ba579a8a11255b7a258cde93801070956d81d57`, shared host.

Evidence directory `catalogue-uuid-domain-14e34f0d` retains commands, raw logs,
source/configuration/toolchain, binary/runtime hashes and unchanged layout probe.
Verified local archive: `setup-evidence/catalogue-uuid-domain-14e34f0d.tar.gz`.
SHA256SUMS manifest digest:

    75146d0627b50b6ce2a0a3aa3f6696d3e106f3085de3e864c51a36073c3e23f3

Raw local artifacts have not been independently inspected remotely; hosted checks
remain authoritative. Prior #268 cache warning and earlier failures remain in
their records; this run does not supersede those historical observations.

Next: temporary catalogue graph-key UUIDs with independently counted requests,
prepayment, transient/retained overlap and graph failure/retry witnesses. Map/
other owner UUIDs, input payload, complete setup, combined resources and operation/
control integration remain separate. No GPU, live DB, browser rerun or full
baseline qualification is claimed.

### Temporary catalogue graph UUID qualification (2026-11-11)

Tested source `f4082be65778d5f262f1e21c448516ad90bca0b7`, stacked on #270 head
`065958cb068f453ecc17693850693275aede3b21`. Review 5858692092 found no blockers
and authorized this slice. Parent smoke 43020338422 passed; native 43020338513
was running at inspection. #268–#270 remain provisional; integration is through #267.

Catalogue object copies now select the UUID domain before allocation for tools,
state, reached and ancestor-target graph keys; the retained-only flag is removed.
Existing address/node/retained payload attachment and graph semantics are preserved.

The diamond fixture independently predicts 38 temporary UUID requests (1406 bytes)
and 27 retained requests (999 bytes), with peak overlap 2405 bytes. Address
requests remain separately measured: temporary 570, retained 180, overlap 750 bytes.
All 1414 graph allocation-failure sites reclaim storage. Duplicate insertion adds
and releases one UUID/address pair without changing values. Cycle rejection
reclaims all backing; repairing the cycle permits an exact-budget retry.

Important witness correction: UUID payload and ObjectKey comparison formatting
both request 37 bytes. The initial size-only observer incorrectly counted both.
The corrected pointer-aware observer classifies pre-admitted requests by allocation-
work delta, checks their count against independent graph cardinalities, and tracks
their releases. A same-sized detached negative control verifies the distinction.
It reports 626 unselected 37-byte formatting requests separately in this fixture.
Those comparison temporaries remain unselected setup work; no comparator change
or complete setup accounting is claimed. Full budget sweeps, exact retry and
catalogue snapshot/move lifetime controls remain active.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 1578 / 2083 | 2066 / 3747 |
| Selected live / peak requested bytes | 284997 / 284997 | 350827 / 350827 |
| Selected cumulative allocation work | 373907 | 703475 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree. Compared with #270, cumulative selected work
increases 1036 bytes in each fixture. Selected live/peak, whole-owner retained/
peak requests, allocation-site counts and census are unchanged. Temporary graph
UUID overlap is below the existing whole-owner peak. Reports overlap and must
not be summed; layouts/encoding/numerical behavior are unchanged.

Preserved initial run at `a4b1e3d34d45bccbc672792b97724c87e6797b30`: builds passed;
four exchange_owner lanes failed the new size-only witness, so the remaining
16 exchange processes did not run. Twelve prepared, seven supporting and four
warm CTests passed; overall exit 1. Archive
`setup-evidence/catalogue-graph-uuid-initial-a4b1e3d3.tar.gz`, SHA256SUMS:

    9af193f7a094ca1865596bdd100cc580324885d619ed9e89d448a2a001bad594

The correction is test-only; production graph attachment is unchanged. Initial
raw logs remain intact. Earlier failures/cache warnings are preserved separately.

Final corrected-source builds passed. All 43 scoped CPU checks passed: 20 exchange
processes across workers 1/2 and bulk/default or direct/compiler-no-unroll,
12 prepared CTests, seven supporting CTests and four warm CTests. Final build/run
logs contain no warnings/errors. Source stayed clean/pinned; final delta is four
Markdown files only. Pinned Clang 15/libstdc++ x64 and Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`, shared host; no timing claim.

Evidence directory `catalogue-graph-uuid-domain-f4082be6` retains commands, logs,
source/configuration/toolchain, binary/runtime hashes and unchanged layout probe.
Verified local archive: `setup-evidence/catalogue-graph-uuid-domain-f4082be6.tar.gz`.
SHA256SUMS manifest digest:

    2ee5d3002a844e47baba1677eef171f9ceb99a35d754ea96ee7318226400f15a

Raw local artifacts have not been independently inspected remotely; hosted checks
remain authoritative. Neither the initial failed run nor historical warnings are
relabeled by this final pass.

Next: retained ExchangeMap slot UUIDs, auditing default construction/reservation/
copy before allocation and preserving inactive-slot reuse. Other owner identities,
comparison-formatting/input payload, full setup, combined resources and operation/
control integration remain separate. No GPU, live DB, browser rerun, full baseline
or performance qualification is claimed.

### Retained map-slot UUID qualification (2026-11-11)

Tested source `cb177073e68355ceba50dd38d388695aa59c70a1`, stacked on #271 head
`768d530b4e140c9fc96bfc7ce13fe7a57c4b739c`. Review 5859029648 found no blockers
and explicitly authorized map-slot UUIDs. Parent smoke 43022851916 passed; native
43022851847 was running at inspection. #268–#271 remain provisional; integrated
acceptance is through #267. Historical graph witness failures and cache warnings
remain preserved. The graph witness combines allocation observations with domain
telemetry; its expected structural counts are independently derived.

Uuid gains a domain-selected nil constructor. ExchangeMap reserves outer backing,
then constructs each occurrence/field slot with its UUID and empty path/address
allocator already selected. No detached default UUID precedes attachment. Explicit
copies preserve inactive nested capacity; assignment retains the destination
domain, and ordinary snapshots detach. UUID representation/encoding is unchanged.

Both slot kinds have independent UUID request/terminator and exact retained-byte
observations. A budget admitting only the outer row rejects the first UUID with
one observed allocation attempt and complete cleanup. Full byte/work sweeps,
all-site failure injection, exact retry, inactive-slot snapshots, source-death,
empty-map later reserve and no-allocation assignment/reuse/erase/swap pass.
Owner/coordinator visitors include all active/inactive slot UUIDs. Standalone
warm checks cover domain-selected nil values, refusal and exact retry.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2731 / 3210 | 3648 / 5803 |
| Selected live / peak requested bytes | 316077 / 316077 | 381907 / 381907 |
| Selected cumulative allocation work | 415606 | 779547 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320017 / 320690 | 390347 / 392869 |
| Retained capacity census | 319905 | 390043 |

All four CPU configurations agree. Relative to #271, selected live/peak increases
31080 bytes in each fixture; cumulative work increases 41699 / 76072 bytes,
including setup copies and slot construction. Whole-owner requested storage,
allocation-site counts and census are unchanged in these fixtures. Slot growth
now reserves requested outer capacity explicitly; no general claim of identical
capacity growth is made. Selected reports overlap the census and must not be
summed; they are not physical-memory or performance bounds.

Full configured native and standalone warm builds passed. All 43 CPU checks
passed: 20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared
checks, seven supporting checks and four warm CTests. No build/run warnings or
errors were found. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no performance claim.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `map-uuid-domain-cb177073` retains commands, logs, source,
configuration/toolchain, binary/runtime hashes and unchanged layout probe.
Verified local archive: `setup-evidence/map-uuid-domain-cb177073.tar.gz`.
SHA256SUMS manifest digest:

    6a70d8accae17cf066e034a205428fc777889bbbbfba2da3e9a32a3bec7916e3

Raw local artifacts have not been independently inspected remotely. Hosted checks
remain authoritative; historical failures and warnings remain preserved.

Next: retained projection input top-level tables in StudyInput/StructuralStudyInput,
after auditing shared interfaces and consumers. Nested input arrays/text,
comparison-formatting temporaries, other setup payload, combined resources and
operation/control integration remain separate. No GPU, live DB, browser rerun
or full baseline qualification is claimed.

### Retained projection input table qualification (2026-11-11)

Tested source `19263b23821992b38dbabe566b0e05b1dde3ba8d`, stacked on #272 head
`5ea60585234fbdea12a6f4441103d82884eb9417`. Review 5859234213 found no blockers
and authorized this shared-interface/consumer audit and bounded continuation.
Parent smoke 43026470580 and warm 43026470847 passed; native 43026470568 was
running at inspection. Integrated acceptance remains through #267; #268–#272
remain provisional. Prior failures and warnings remain preserved.

StudyInput and StructuralStudyInput top-level nodes, definitions, occurrences,
selections, contacts, memberships and focal fields use InputTable, backed by the
shared reservation allocator. Explicit projection copies select the destination
domain before each table allocation. Nested arrays/text remain separate. Public
bind nodes and active-workbench focus use iterator assignment; native scale
binding and assembly fixtures consume the allocator-aware type. No model value,
recipe version or numerical change is intended. All configured native consumers,
including warm integration and viewport capture, were rebuilt successfully.

Independent table pointer/request checks, exact selected-byte equality and all
allocator identities now include fourteen input tables. Complete v1/v2 equality
covers capacity, nodes, geometry, parents, starts, selections, contacts, membership
and focus. Real recipe fixtures are supplemented with retained nodes/memberships/
focus normally filled only at public handoff. Detached snapshots after source
death, moves retaining domain lifetime, byte/work refusal sweeps, reclamation and
exact retry cover these rows. Whole-owner all-site failure injection remains active.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2735 / 3218 | 3652 / 5811 |
| Selected live / peak requested bytes | 317837 / 317837 | 385347 / 385347 |
| Selected cumulative allocation work | 417366 | 782987 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320465 / 321138 | 390795 / 393317 |
| Retained capacity census | 320353 | 390491 |

All four CPU configurations agree. Compared with #272, selected live/peak/work
increases 1760 / 3440 bytes. Whole-owner retained/peak requests and census each
increase 448 bytes, reflecting allocator state in two retained projection objects;
allocation-site counts are unchanged. The remaining selected increase accounts
for existing top-level table backing.

Pinned layouts: StudyInput 288, StructuralStudyInput 280, LetterProjection 1304
bytes. UUID/ObjectKey/ToolRef/Tool/Parent/Geometry/Bundle remain 48/88/136/432/168/
152/520 bytes; UUID payload remains 37 bytes. Allocator state increases retained
projection object size; selected accounting overlaps the existing capacity census
and must not be summed. These are requested-storage observations, not physical
memory or performance bounds.

The auxiliary layout helper initially omitted include paths and failed to find
assembly/study_input.h. Correcting only its include paths produced the above
report; the diagnostic is retained in layout-helper-initial-failure.txt. This was
not a native or warm product build failure. No source change or warning-erasing
rerun followed it.

All 45 CPU checks passed: 20 exchange processes (five cases, workers 1/2,
bulk/direct), 12 prepared checks, nine supporting checks (including assembly and
preparation) and four warm CTests. No native/warm build or test warnings/errors.
Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no performance claim.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `input-table-domain-19263b23` retains commands, logs, source,
configuration/toolchain, binary/runtime hashes and the corrected layout probe.
Verified local archive: `setup-evidence/input-table-domain-19263b23.tar.gz`.
SHA256SUMS manifest digest:

    d5c9513ea29df142de6c15b927d672df411d63408ac3870aa6f670dcd3acf8d9

Raw local artifacts have not been independently inspected remotely; hosted checks
remain authoritative. Historical failures and warnings remain preserved.

Next: nested retained input arrays (Definition components/parents and Selection
parts). Input text, other identities/comparison-formatting, full setup, combined
resources and operation/control integration remain separately inventoried. No GPU,
live database, browser rerun, full baseline or performance qualification is claimed.

### Nested retained input array qualification (2026-11-11)

Tested source `4820a18bb5f5aff6813b5707254cbbf8d2788e73`, stacked on #273 head
`8c142c3056736ba6a1a683b14ced8c79163a08ab`. Review 5859431800 found no blocking
source findings and authorized this slice. Parent smoke 43028610415 passed;
native 43028610546 was running at inspection. Integrated acceptance remains
through #267; #268–#273 remain provisional.

Definition components/parents and Selection parts use InputTable. Explicit
projection input copying reserves the outer table and constructs each row with
its nested destination allocators already selected. Text stays detached. The
input backing/domain visitor includes every nested array, including empty ones.
Existing independent request observations, exact selected-byte equality, complete
v1/v2 comparisons, refusal/reclamation/exact retry, detached snapshots/source death,
moved lifetime and all-site owner failure injection exercise this added scope.

The shared-consumer audit preserves the public std::vector enclosing-radius API
and factors the unchanged calculation into an allocator-generic internal helper.
Active-workbench local geometry and geometry overrides use iterator copies;
assembly fixtures use InputTable. Definition/Selection layout changes require
consumer rebuilds; model values and native handoff remain unchanged.

Initial source `f0e40840f0a429c85ba93fa905c2002cb343a3d4` failed native compilation
at active_workbench.cpp:133: geometry override still assigned std::vector into
InputTable. The corrected source uses iterator assignment. No qualification tests
ran at the failed source; warm build/tests were not reached. Initial build/source
archive: `setup-evidence/input-nested-initial-f0e40840.tar.gz`, manifest digest:

    c6eb73fe0e9b1d6d420528db022ad493dfe2670930b8b66fa1db109db3f26342

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2742 / 3232 | 3662 / 5831 |
| Selected live / peak requested bytes | 318861 / 318861 | 386851 / 386851 |
| Selected cumulative allocation work | 418390 | 784491 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320689 / 321362 | 391115 / 393637 |
| Retained capacity census | 320577 | 390811 |

All four CPU configurations agree. Compared with #273, selected live/peak/work
increases 1024 / 1504 bytes. Whole-owner retained/peak requests and census each
increase 224 / 320 bytes from nested allocator state; allocation-site counts are
unchanged. The remaining selected increase accounts for existing nested backing.
Pinned layouts: Definition 144, Selection 80 bytes; StudyInput 288,
StructuralStudyInput 280 and LetterProjection 1304 remain unchanged. UUID-related
layouts and encoding remain unchanged, including the 37-byte UUID payload.

Corrected native and standalone warm builds passed. All 45 CPU checks passed:
20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared checks,
nine supporting checks (including assembly/preparation/native handoff/viewport)
and four warm CTests. No corrected-source build or test warnings/errors.
Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no performance claim.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `input-nested-domain-4820a18b` retains commands, logs, source,
configuration/toolchain, binary/runtime hashes and layout probe.
Verified local archive: `setup-evidence/input-nested-domain-4820a18b.tar.gz`.
SHA256SUMS manifest digest:

    5f7f86d8996bc0b30529f11dc5c5a71a7c19fc986d900c6527e53899e06048ce

Hosted checks remain authoritative. The corrected pass does not relabel the
initial compiler failure or historical warnings.

Selected reports overlap the capacity census and must not be summed. These are
requested-storage observations, not physical-memory or performance bounds.
Next: retained input text, including inline/heap payload and terminators. Other
identities/comparison-formatting, full setup, combined resources and operation/
control integration remain separate. No GPU, live database, browser rerun, full
baseline or performance qualification is claimed. Earlier failures/warnings remain
preserved; raw local artifacts have not been independently inspected remotely.


### Maintainer queue reconciliation through #274 (2026-11-11)

**Queue reconciliation — 2026-11-11:** Reviewed #268–#274 are integrated into main at `b05db31c2e591f8fbcd5c14dc7ddff14ef46b2e9`, reviewed tip `ce3642ee463beb76948a2497c9aeb50172c3267d`. Every head in that stack has successful native and smoke workflows; #272 warm also passed. #274 native 43030713058 passed all 12 jobs; inspected regression logs total 115 tests and database reports addressed_reads=24/private_reads=0. Earlier provisional/pending labels below are historical and superseded by this acceptance.

**Current review queue:** #275 parent-value text, #276 contact/membership/focus text and #277 selection text are source-reviewed and provisionally accepted, but remain unmerged. Inspected native jobs 130564239034 (#275), 130638929459 (#276 retry) and 130648619590 (#277) failed during toolchain installation because azure.archive.ubuntu.com could not resolve, before product compilation. #276 viewport retry job 130638930090 was cancelled during dependency installation with the same DNS failures. Preserve these attempts; complete successful hosted qualification before integration. The next coding slice is definition/occurrence identities and references, per #277 review 5863228338. Other identities/comparison formatting, complete setup, combined accounting and operation/control integration remain open. #75, #111 and #123 remain deliberately held.

Inspected #274 regression jobs 130559263122/130559263206/130559263217/130559263265 passed 28/29/29/29 tests. Database job 130559263025 reports addressed_reads=24/private_reads=0. Main was an ancestor of the reviewed tip before merge, with no main-only changes to reconcile. Local archives were not independently rerun or inspected in this reconciliation; no GPU or performance claim follows. Original commits and failed/cancelled evidence are preserved.

### Retained input parent-value text qualification (2026-11-11)

Tested source `08e9ccf60d190cc99ce8d64b35e4d0a15da141ad`, stacked on #274 head
`ce3642ee463beb76948a2497c9aeb50172c3267d`. Review 5859635152 found no blockers
and authorized retained input text. The consumer audit splits that workstream:
this contribution covers parent values; other IDs/references cross additional
lookup and serialization paths and remain the next slice. Parent smoke 43030712985
passed; native 43030713058 remains pending at inspection. Integrated through #267.

ParentOccurrence.value uses InputText with the neutral shared allocator. Explicit
projection copies select its domain before copying characters. Parent ordinals,
counts, ordering and nested table attachment remain unchanged. Other input text
is excluded. Projection construction converts encoded keys explicitly; native
preparation converts parent values at existing public group/participation outputs.

The text visitor covers parent values in both input variants. Independent pointer/
request observations distinguish inline backing, check heap capacity plus terminator,
verify domain identity and exact selected bytes. Supplemented v1/v2 copy fixtures
include empty, short and 137-character parent values, alongside real encoded keys.
Full input equality, detached source-death snapshots, moved lifetime, byte/work
refusal, reclamation/exact retry and whole-owner all-site failure controls apply.
Supplementary values qualify copying, not native model validity. The census uses
inline-aware backing rather than charging inline characters twice.

Initial source `df3315e13666dbcd6abb8fb3ad73d2902732108d` failed compilation in
letter_projection_native_test.cpp:84, comparing InputText directly with std::string.
The corrected test converts explicitly; production code was unchanged. No tests
or standalone warm build ran at the initial source. Initial build/source archive:
`setup-evidence/input-parent-text-initial-df3315e1.tar.gz`, manifest digest:

    0c7f1d8e9fc0e080c8501521d018a8ede20dc617404a7ca469a57826f49b6053

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2747 / 3242 | 3667 / 5841 |
| Selected live / peak requested bytes | 319541 / 319541 | 387531 / 387531 |
| Selected cumulative allocation work | 419070 | 785171 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320849 / 321522 | 391275 / 393797 |
| Retained capacity census | 320737 | 390971 |

All four CPU configurations agree. Compared with #274, selected live/peak/work
increases 680 bytes in each fixture: 520 existing text backing and 160 allocator
state. Whole-owner retained/peak requests and census increase 160 bytes each;
allocation-site counts are unchanged. InputText is 48 bytes, ParentOccurrence 64;
StudyInput/StructuralStudyInput/LetterProjection remain 288/280/1304. UUID-related
layouts/encoding are unchanged.

Corrected native and standalone warm builds passed. All 45 CPU checks passed:
20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared checks,
nine supporting assembly/preparation/native-handoff/viewport checks and four warm
CTests. No corrected-source build or test warnings/errors. Clang 15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `input-parent-text-domain-08e9ccf6` retains commands, logs,
source/configuration/toolchain, binary/runtime hashes and layout probe.
Verified local archive: `setup-evidence/input-parent-text-domain-08e9ccf6.tar.gz`.
SHA256SUMS manifest digest:

    fe1dd68cdf84a420a45f1bbcca178be832d891c7ff980f627a54d8495ed19c49

Hosted checks remain authoritative. The corrected pass does not relabel the
initial compiler failure or earlier warnings.

Selected reports overlap the census and must not be summed. These observations
are not physical-memory/performance bounds. Remaining input text, other identities,
comparison-formatting, full setup, combined accounting and operation/control
integration remain separate. No GPU, live database, browser/full baseline rerun
or performance qualification. Earlier failures/warnings remain preserved; local
raw artifacts have not been independently inspected remotely.

### Retained relation/focus input text qualification (2026-11-11)

Tested source `6bd7ad81c8cbd86af7ae21a02f8b981a83bdd660`, stacked on #275 head
`6bbae550f6e63a53f832a34449cf14008192e966`. Review 5859805974 found no blockers
and authorized remaining input text. This slice groups contacts, memberships and
focal fields at their common preparation lookup boundary; identity/selection text
remains next. Parent smoke 43032906551 passed; native 43032906525 was pending at
review inspection. Integrated acceptance remains through #267.

Contact.a/b, Membership.occurrence/group and FocalField.value use InputText.
Explicit projection copies construct these strings with destination allocators
before character allocation. Ordinary snapshots detach and moves retain domains.
Projection/membership binding, local workbench context, native lookup boundaries,
fixture builders and viewport focus callers use explicit conversions. Output
encoding and numerical policy are unchanged. Temporary lookup conversions remain
unselected setup work, not claimed as retained input storage coverage.

The text visitor includes every added field in both variants. Supplementary copy
fixtures add empty, short and 139-character values in each field, alongside real
recipe keys; these qualify copying rather than native input validity. Independent
pointer/request/terminator/domain checks, inline/heap distinction, exact selected
bytes, complete v1/v2 equality, refusal/reclamation/exact retry, detached source-death
snapshots, moved lifetime and whole-owner all-site failures cover this backing.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2748 / 3244 | 3668 / 5843 |
| Selected live / peak requested bytes | 319677 / 319677 | 387667 / 387667 |
| Selected cumulative allocation work | 419206 | 785307 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320881 / 321554 | 391307 / 393829 |
| Retained capacity census | 320769 | 391003 |

All four CPU configurations agree. Compared with #275, selected live/peak/work
increases 136 bytes in each owner fixture: 104 existing focus-text backing and
32 allocator state. Whole-owner retained/peak requests and census increase 32
bytes each; sites are unchanged. Contact/membership attachment is additionally
exercised by the populated v1/v2 copy fixtures, not inferred from these owner totals.
Contact/Membership/FocalField layouts are 96/96/56 bytes; input containers and
LetterProjection remain 288/280/1304, with UUID-related layouts unchanged.

Native and standalone warm builds passed on the first attempt. All 45 CPU checks
passed: 20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared
checks, nine supporting assembly/preparation/native-handoff/viewport checks and
four warm CTests. No build or test warnings/errors. Clang 15/libstdc++, CPU/x64,
Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `input-relation-text-domain-6bd7ad81` retains commands, logs,
source/configuration/toolchain, binary/runtime hashes and layout probe.
Verified local archive: `setup-evidence/input-relation-text-domain-6bd7ad81.tar.gz`.
SHA256SUMS manifest digest:

    37502fc07f801f54264185ec3e1714a8e195f2b417637ffd4b08a8a3fb8a6f5c

Hosted checks remain authoritative; earlier failures/warnings remain preserved.

Selected reports overlap the capacity census and must not be summed. Requested
storage is not physical memory or performance. Next: definition/occurrence IDs
and references and selection aggregate/parts text, auditing public identity and
serialization consumers. Other identities/comparison-formatting, full setup,
combined accounting and operation/control integration remain separate. No GPU,
live DB, browser/full baseline rerun or performance qualification. Historical
failures/warnings remain preserved; raw local artifacts have not been independently
inspected remotely.

### Retained selection input text qualification (2026-11-11)

Tested source `dd98ef1dacc10c5d1b7a5622c728221c49f23e71`, stacked on #276 head
`755595b21b3193c5a236c2c3c5c5fb5a30be1cf7`. Review 5861623800 found no source
blockers and authorized continuation. Selection aggregate/parts are this bounded
subset; definition/occurrence identities remain next. Integrated through #267.

Parent hosted reconciliation, as personally inspected by the maintainer: smoke
43034798259 and projection-values job 130570694696 passed. Native 43034799670/job
130570695102 failed before product compilation because apt could not resolve
azure.archive.ubuntu.com (exit 100). Viewport 43034798156/job 130570690557 had the
same DNS failure during Playwright dependencies and was cancelled before browser
tests. Maintainer retry requests succeeded; failed/cancelled attempts remain
preserved and are neither product failures nor passes. No source workaround masks them.

Selection.aggregate and parts elements use InputText, constructed directly with
the selected domain before character allocation. Part tables remain attached.
Projection construction and native lookup adapt explicitly; values, order and
exposure are unchanged. The requested membership.group.empty() cleanup removes
a needless string copy. Other boundary conversions remain unselected setup work.

Independent text observers cover aggregate and every part in both variants,
including inline/heap distinction, capacity plus terminator, domain identity and
exact selected accounting. Empty/short/139-character supplemented copy fixtures
exercise these fields without claiming native input validity. Full v1/v2 equality,
refusal/reclamation/exact retry, detached source-death snapshots, moved lifetime
and owner all-site failures remain active.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2752 / 3250 | 3680 / 5867 |
| Selected live / peak requested bytes | 320023 / 320023 | 389051 / 389051 |
| Selected cumulative allocation work | 419552 | 786691 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 320977 / 321650 | 391691 / 394213 |
| Retained capacity census | 320865 | 391387 |

All four CPU configurations agree. Compared with #276, selected live/peak/work
increases 346 / 1384 bytes: 250 / 1000 existing character backing plus 96 / 384
allocator state. Whole-owner retained/peak requests and census increase 96 / 384;
allocation-site counts are unchanged. StudyInput/StructuralStudyInput/LetterProjection
remain 288/280/1304 bytes; UUID-related layouts and encoding are unchanged.

Native and standalone warm builds passed. All 45 CPU checks passed: 20 exchange
processes (five cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting
assembly/preparation/native-handoff/viewport checks and four warm CTests. No build
or test warnings/errors. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
The final tested-to-head delta contains only four Markdown evidence/status files.

Evidence directory `input-selection-text-domain-dd98ef1d` retains commands, logs,
source/configuration/toolchain, binary/runtime hashes and layout probe.
Verified local archive: `setup-evidence/input-selection-text-domain-dd98ef1d.tar.gz`.
SHA256SUMS manifest digest:

    4cb963ddd94760a6d7c317b843f2cc3d031ad12434dcffc52b38edd75a2efa25

Hosted checks remain authoritative. At final parent recheck, native 43034799670
was running; viewport 43034798156 still reported cancelled. Neither is claimed
as passed; prior failed/cancelled attempts remain preserved.

Selected accounting overlaps the census; do not sum the reports. No physical-memory
or performance bound is claimed. Next: definition/occurrence identity and reference
text, auditing shared lookup/public identity/serialization paths. Other identities,
comparison-formatting, full setup, combined accounting and operation integration
remain separate. No GPU, live DB, browser/full baseline rerun or performance
qualification. Raw local artifacts have not been independently inspected remotely;
earlier failures/warnings remain preserved.

### Retained input identity text qualification (2026-11-11)

Tested source `e15108abd5defa6125946467fdd61a34ba81aacb`, stacked on #277 head
`c3c89044fc9aed06b0aa18540137ab9b2f54d7e9`. #277 review 5863228338 and #278 review
5864410183 identify definition/occurrence identity text as the next coding slice.
Integrated main is accepted through #274; #275–#277 remain provisional. The
separate #278 CI repair is source-accepted with full hosted qualification and
integration pending at inspection. Carry it into this stack after integration
and qualify the updated code before merging; this source does not include it.
Earlier DNS-failed/cancelled hosted attempts remain preserved.

Definition.id and both identity/reference fields of Occurrence and
StructuralOccurrence use InputText. Explicit copies construct character backing
in the selected domain. This completes retained input text attachment. Preparation,
active binding, native fixtures and viewport snapshot consumers explicitly adapt
ordinary-string boundaries without changing identity values or output encoding.
Temporary boundary conversions and other setup costs remain separately scoped.

The independent visitor observes each new field's pointer, heap request including
terminator, inline storage and allocator domain. Supplemented v1/v2 copy fixtures
cover empty, short and 139-character identities/references; these are not valid
native-model claims. Exact complete projection backing coverage now replaces the
old requirement that some retained payload remain excluded. Full row equality,
refusal/reclamation/exact retry, detached source-death snapshots, moves and
whole-owner allocation-failure controls remain active.

Preserved attempts:

- `5ea713115707a52be96c4ad8be98e83833427329` failed compilation at
  live_hierarchy_test.cpp:2525: a viewport fixture conditional mixed InputText and
  std::string. No tests or warm build ran. Explicit conversion fixed it in
  `0c05b9b4af58dbd7e9d6d020be8047d2bae0dfe3`. Archive
  `input-identity-text-domain-5ea71311-build-failure.tar.gz`, manifest digest
  `4b853584132c05dbb49a17d632272e68440a01a18660e6465c4a9fb817f5c213`.
- `0c05b9b4` built successfully and passed 12 prepared, nine supporting and four
  warm checks. Each of the four exchange configurations stopped in exchange_owner
  at the old partial-coverage assertion; subsequent exchange cases were not run.
  With every retained projection string selected, requiring observed live bytes
  to exceed selected bytes is obsolete. The correction requires exact equality
  with independently observed requests rather than weakening the bound. Archive
  `input-identity-text-domain-0c05b9b4.tar.gz`, manifest digest
  `8a1890cc6b9474d2a68bf5985b7e37adfefff8aa350089bfcf4d501d05223deb`.
| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / allocation-work boundaries | 2761 / 3268 | 3707 / 5921 |
| Selected live / peak requested bytes | 321185 / 321185 | 392475 / 392475 |
| Selected cumulative allocation work | 420714 | 790115 |
| Whole-owner allocation sites, all reclaimed | 4117 | 7757 |
| Whole-owner retained / peak requested bytes | 321265 / 321938 | 392555 / 395077 |
| Retained capacity census | 321153 | 392251 |
| Failure-sweep peak requested bytes | 321923 | 395062 |

All four CPU configurations agree. Relative to #277, selected live/peak/work
increases 1162 / 3424 bytes: 874 / 2560 existing identity character backing plus
288 / 864 allocator state. Whole-owner retained/peak requests and census increase
288 / 864; allocation-site counts are unchanged. Definition/Occurrence/
StructuralOccurrence layouts are 160/152/104 bytes. StudyInput/StructuralStudyInput/
LetterProjection remain 288/280/1304 bytes; UUID-related layouts are unchanged.

Selected accounting overlaps the census; do not sum these reports. Complete
retained projection backing coverage does not establish complete setup work,
combined owner resources, physical-memory or performance bounds. Next: audit
remaining owner identities/comparison formatting (including ObjectKey comparison)
and membership traversal/callable costs before combined accounting and operation
integration. No GPU, live DB, browser/full baseline rerun or performance
qualification. Raw local artifacts have not been independently inspected remotely;
earlier failures/warnings remain preserved.
Final native and standalone warm builds passed. All 45 scoped CPU checks passed:
20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared checks,
nine supporting assembly/preparation/native-handoff/viewport checks and four warm
CTests. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. No build warnings/errors.
One Taichi offline-cache lock warning follows the final PASS line in
1-bulk-exchange_commit.log; it remains in the archive, not erased or relabeled.
No other build/test warnings or errors were found in the final run.

The final tested-to-head delta contains only four Markdown evidence/status files.
Evidence directory `input-identity-text-domain-e15108ab` retains commands, logs,
source/configuration/toolchain, binary/runtime hashes and the layout probe.
Verified local archive: `setup-evidence/input-identity-text-domain-e15108ab.tar.gz`.
SHA256SUMS manifest digest:

    ba4beb87ca67e045e511d1788512a0c0fe35788fb139e7c4c2c045b458d70974

Hosted checks remain authoritative. At final dependency inspection, #278 native
43087248989 had completed its compiler matrix; regression shards 3 and 4 remained
running. No full hosted pass or integration is claimed.

### Combined pending input-text stack and accepted main (2026-11-11)

Review 5864859891 provisionally accepts #279 head
`11a4813759c1e384fa6b077499c9f06998c66f58` and authorizes a normal merge from main.
#278 is accepted and integrated at `3cadbff286b0c4553808595f624f8a98add36682`;
its native 43087248989, smoke 43087248327 and viewport 43087248550 passed,
with maintainer-inspected archive traffic. These later results supersede earlier
pending labels without relabeling failed/cancelled attempts.

Tested merge `a75e34084bed5fcfc24280b54111081d6440d58d` has parents #279's reviewed
head and current main `6067c641`. The merge adds accepted CI repair/workflows and
status records; it changes no engine, warm-value or viewport product source.
Four documentation conflicts were resolved by preserving both historical evidence
sets and distinguishing current acceptance from earlier queue labels. #275–#277
and #279 remain unmerged into main pending combined qualification and disposition.

The single/four-selection accounting agrees with #279 in every CPU configuration:
selected live/peak 321185/392475 bytes, cumulative work 420714/790115,
4117/7757 failure sites fully reclaimed, retained requested 321265/392555,
peak requested 321938/395077 and capacity census 321153/392251. The independent
complete projection backing witness remains exact. No physical-memory or
performance claim follows from shared-host CPU execution.

The accompanying identity/setup audit is recorded in OWNER-RESOURCES. A local
source-pinned helper links the merged warm-values archive and records temporary
C++ new calls for equality, ordering, containment, nil and UUID copies. Its first
command named a nonexistent separate codec archive; corrected linking uses the
codec already compiled into warm_values. The original helper-only diagnostic is
retained as identity-audit-initial-build.log. No product source/build failure or
product test rerun was needed for that helper correction.
Combined native/standalone warm builds and all 45 scoped CPU checks passed:
20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared checks,
nine supporting assembly/preparation/native-handoff/viewport checks and four warm
CTests. Integrated mirror fixtures, ShellCheck and CI orchestration contracts
also passed. No product build/test warnings or errors occurred in this run;
prior source warnings and the audit-helper link diagnostic remain preserved.
Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. No GPU, live database,
browser/full accepted-baseline rerun or performance qualification.

The final tested-to-head delta contains only four Markdown evidence/status files.
Verified archive: `setup-evidence/input-stack-main-domain-a75e3408.tar.gz`.
The archive includes source/configuration, commands, logs, binary/runtime hashes,
layouts and the identity audit helper/results. SHA256SUMS manifest digest:

    462decf887b11ad9c00db49a70f6cec0ea412a795d90093af00978277da60e08

This is contributor local qualification of the combined head. Hosted qualification
of the updated head and maintainer integration remain pending; neither #279's
prior-head jobs nor #278's accepted CI run substitutes for those checks.
### Allocation-free ObjectKey ordering qualification (2026-11-11)

Tested source `86e4626e83c6f7ecd6e506571f3eaad478c73350`, stacked on provisionally
accepted #279 head `5f1ca0c3a6fdf3d4316ff861c521e92e15907dc1`. Review 5865153858
authorizes this bounded implementation while the combined stack's hosted checks
run. #279 is the main-targeted integration PR; #275–#277 remain pending until
integration. Accepted main/#278 are already included. No history is rewritten.

ObjectKey ordering now validates both addresses before comparing scopes and
couplet indices. The rendered-key oracle remains unchanged. Tests cover every
couplet transition, scopes, strict prefixes, long addresses, malformed left/right
operands even across different scopes, and independently counted zero C++ new for
valid comparisons. Invalid-input exception allocation is not claimed zero.
Nil, containment, identity copies, encoding and numerical policy are unchanged.

A negative control links the new test against values.cpp from #279's prior head
and confirms the no-allocation test rejects the old comparator. The diagnostic
wrapper initially renamed main without adding an explicit success return; its
compiler warning is preserved in negative-initial-build.log. Adding a return to
the local diagnostic copy corrected that helper warning. Product source did not
change and the corrected negative control passed. This is an expected negative
control, not a product test failure.

| Witness | One selection | Four selections |
|---|---:|---:|
| Byte / work boundaries | 2761 / 3268 | 3707 / 5921 |
| Selected live / peak requested bytes | 321185 / 321185 | 392475 / 392475 |
| Selected cumulative allocation work | 420714 | 790115 |
| Allocation sites, all reclaimed | 3289 | 6077 |
| Retained / peak requested bytes | 321265 / 321938 | 392555 / 395077 |
| Retained capacity census | 321153 | 392251 |

All four CPU configurations agree. Relative to #279, 828 / 1680 temporary
allocation sites disappear; selected reservation totals, retained storage and
peak observations are unchanged. The diamond-graph witness reports zero
unselected 37-byte requests and 162 failure sites; retained/temporary UUID and
address accounting remain exact. Requested bytes are not RSS or performance.

Next: nil/containment validation, detached temporary identity/argument copies,
then callable/recursive traversal and declaration/binding setup budgets before
combined resources and operation integration. Earlier failures, cancelled runs
and warnings remain preserved. No GPU, live DB, browser/full accepted-baseline
rerun or performance qualification is claimed.
Both builds and all 46 scoped CPU checks passed: 20 exchange processes (five
cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting native
handoff/snapshot checks and five standalone warm CTests including the new ordering
test. The prior-comparator negative control rejected the old implementation as
expected. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. Product builds have no
warnings/errors. One Taichi offline-cache lock warning follows PASS in
1-bulk-exchange_commit.log; it and the corrected diagnostic-wrapper warning are
retained. No other build/test diagnostics were found.

The final tested-to-head delta contains only four Markdown evidence/status files.
Verified archive: `setup-evidence/object-key-ordering-domain-86e4626e.tar.gz`.
Commands, source/configuration, logs, binary/runtime hashes, layouts and the
negative-control source/helper are retained. SHA256SUMS manifest digest:

    09002b6a1d4761b6d2d911eeebba23a3765036d8425f6c98c602adf3132238a6

Raw local artifacts have not been independently inspected remotely. Hosted checks
remain authoritative. At parent inspection, #279 latest-head native 43098106376,
viewport 43098106428 and smoke 43098106405 were queued; no full pass or integration
is inferred. Original commits and earlier failure/warning evidence remain intact.
### Nonallocating nil/containment qualification (2026-11-11)

Tested `bf958b8220c1b9d08d4089d9ca9b8c2ef839345d`, stacked on #280 head
`5e37be82a547dd4e88371cb927bea3effa3bb6e4`, provisionally accepted by review
5865274115. #279 remains the main-targeted integration PR for the pending text
stack, including accepted #278. Parent queued checks are not integration evidence.

Uuid::nil compares against a fixed canonical literal. Containment validates both
addresses without rendering keys, then retains the existing scope/length/element
prefix checks. Rendering and ordering share that same private full-address
validator. Empty, invalid-element and partial addresses retain the same error,
including when different scopes or lengths could otherwise short circuit.
Public values/encoding, UUID normalization and allocator lifetimes are unchanged.

The new standalone test observes zero C++ new/new[] for valid calls. Nil cases
include every UUID digit position made nonzero with uppercase input, canonical
nil and an attached nil at its exact exhausted domain budget. The containment
oracle is rendered text with delimiter-aware prefix boundaries, independent of
the production element-prefix algorithm. Multiple scopes, alphabet boundaries,
lengths 1/2/5/128, equality, strict prefixes and near misses are exercised.
Invalid operands on each side cover empty, invalid indices and partial addresses
across scope/length combinations. Exception allocations are excluded from the
zero-allocation claim. Existing ordering/address/UUID tests remain active.

Two negative controls link the new test to values.cpp from #280 head and run
nil and containment independently. Each rejects its prior allocating helper as
expected. Sources, commands and logs are retained; no product-source change is
made for those controls.

Owner metrics match #280 in all four CPU configurations: single/four-selection
selected live/peak 321185/392475, cumulative work 420714/790115, boundaries
2761/3268 and 3707/5921, retained/peak requests 321265/321938 and 392555/395077,
census 321153/392251, and 3289/6077 failure sites fully reclaimed. These owner
fixtures do not establish the helpers' allocation removal; the direct independent
controls do. No layout, physical-memory, timing or GPU claim follows.

Next: detached temporary identity/argument copies, then membership callable/
recursion and declaration/binding setup budgets, combined resources and operation
integration. Earlier failed/cancelled runs and warnings remain preserved. No GPU,
live DB, browser/full accepted-baseline rerun or performance qualification.
Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five
cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting native
handoff/snapshot checks and six standalone warm CTests. Both prior-source negative
controls rejected the allocating helpers as expected. No build/test/helper
warnings or errors occurred in this run. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host.

The final tested-to-head delta contains only four Markdown evidence/status files.
Verified archive: `setup-evidence/identity-validation-domain-bf958b82.tar.gz`.
Commands, source/configuration, logs, binary/runtime hashes, layouts and separate
nil/containment negative controls are retained. SHA256SUMS manifest digest:

    2bcfb23569e74a58316857e384018fe805a7a6f70cdfec30626932bd35a00e0a

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. At parent recheck, #280 smoke
43099228124 and warm 43099228183 reported success; native 43099228319 was running.
These workflow states do not imply stack integration or qualify this new head.

### Adoption argument and declaration identity qualification (2026-11-11)

Tested `ccd8d4d9102379b6e85ce23858456f2d4c22acb5`, stacked on #281 head
`15db4b816456fb9ff299801daac42e1624b50ead`, provisionally accepted by review
5865407113. #279 remains the main-targeted integration PR; pending stack heads
are not treated as merged or fully accepted.

Owner and binding row/field arguments now use const references through construction,
avoiding detached argument copies while retaining independent domain-owned maps.
The temporary aggregate OccurrenceKey explicitly constructs its UUID in the
reservation domain before allocation; its full-expression lifetime is unchanged.
The legacy consumed declaration argument remains by value. Logical input admission
and selected retained map copies are unchanged; this is not a complete setup gate.

The added control verifies caller vector backing and values are unchanged by
adoption, destroys those caller tables, and checks the owner's independent maps.
Existing detached snapshots, shared alias lifetime, exact reservation boundaries,
refusal/retry and exhaustive allocation failure sweeps remain active.

Initial source `8e01828dee8ea71e20d6cc0ba98744ac87407226` built successfully but all
four exchange-owner processes stopped at the older strict peak-greater-than-live
assertion. Removing transient argument copies allows retained storage to dominate
the peak. The correction permits equality while requiring observed temporary
releases and full teardown reclamation. No product behavior changed in that
correction. The failed attempt is preserved in
`setup-evidence/adoption-arguments-domain-8e01828d.tar.gz`, manifest digest:

    c0a4467703eeb1bfdd92931d62c33195180541076304e62ecdb6fab14a63e500
Single/four-selection metrics agree across workers 1/2 and bulk/direct:
selected live/peak 321185/392475; cumulative allocation work 420751/790263;
byte/work boundaries 2761/3269 and 3707/5925. Retained/peak requests are now
321265/321265 and 392555/392555, with census 321153/392251 unchanged.
Allocation sites fall from 3289/6077 to 3271/5927; every site is injected and
reclaimed, with zero remaining blocks. Failure peaks are 321250/392540.
The newly charged UUID adds 37/148 cumulative bytes for one/four selections,
without changing retained domain bytes. These are requested-allocation witnesses,
not physical memory or dedicated-host performance measurements.
Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six standalone warm CTests. No build/test/helper warnings or errors in
the corrected run. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no GPU computation.
No live database, browser, full accepted-baseline rerun or performance qualification.

Final tested-to-head changes are limited to four Markdown evidence/status records.
Verified archive: `setup-evidence/adoption-arguments-domain-ccd8d4d9.tar.gz`.
Commands, source/configuration, logs, binary/runtime hashes and layouts are retained.
SHA256SUMS manifest digest:

    a929abe8fc7d4eeb562e49a5b980c6c9d3090e524b423239d5237cbf0f1d1d90

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. At parent inspection, #281 smoke
43101300624 and warm 43101300677 succeeded; native 43101300728 was queued.
Next: remaining identity copies outside these paths, membership callable/recursion
and declaration/binding setup budgets, then combined resources and operations.

### Byte-expansion temporary identity qualification (2026-11-11)

Tested `22c4699bac19d4961d241d8efbfb7aff8115a3ad`, stacked on #282 head
`29b82129cd040b3bbe3fda33d2b85a12e181f904`, provisionally accepted by review
5865626048. #279 remains main-targeted integration; the dependent stack is unmerged.

The private ByteRef character/byte identities are borrowed from stable source
tools during normalization/projection. The source tool table is never mutated by
those consumers, and no pointer is stored in their published output. Definition
and provenance values still copy their own identities. Legacy and structural
projection tests now destroy a local source bundle before checking output values,
including exact UTF-8 provenance, repeated bytes and pinned revisions. Existing
malformed/reordered/stale input rejection remains active.

A focused local probe includes the production translation unit and observes C++
new requests of 37 bytes during expansion only, after fixture/index construction.
Five letters yield 13 byte rows and zero such requests. Rebuilding the same probe
against #282's letter_projection.cpp yields 52 requests and rejects that previous
implementation. This supports removal of the four UUID character copies per row;
it does not claim zero total allocation or account for addresses/path/table/string
backing. Both helper sources, commands, build logs and results are retained.

Owner metrics match #282 in all four CPU configurations: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251 and 3271/5927 failure sites fully reclaimed. Those unchanged
owner totals do not establish the expansion removal; initial projection is outside
adoption. No physical-memory or dedicated-host performance claim follows.

Remaining initial projection indexes, provenance/equivalence copies and rendering
temporaries are still distinct from independent published outputs. Membership
callable/recursion and declaration/binding setup budgets remain open, followed by
combined resources and operation integration. No GPU, live DB or browser work.
Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five
cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting native
handoff/snapshot checks and six standalone warm CTests. The prior-source expansion
probe failed the zero-copy predicate as expected; the current probe passed.
No build/test/helper warnings or errors. Clang 15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. No performance claim.

Final tested-to-head changes contain only four Markdown evidence/status records.
Verified archive: `setup-evidence/projection-temporaries-domain-22c4699b.tar.gz`.
Commands, source/configuration, logs, binary/runtime hashes, layouts and both
expansion probe sources/results are retained. SHA256SUMS manifest digest:

    e913ad7f9b704e8bf3813d8b7438d648dee065e491d007a386078d899632fafe

Raw artifacts have not been independently inspected remotely. Hosted qualification
and review remain required. Parent #282 smoke 43104595184 succeeded; native
43104595076 was still running at recheck. Prior stack failures remain preserved.

### Occurrence rendering argument qualification (2026-11-11)

Tested `fa3bef8fde62df7cc18bb02c4822c430bdc71e09`, stacked on #283 reviewed head
`38e81a957066e85294a0afe2555fb8dcc27090df`. Review 5866277807 reports no blocking
finding and green native 43106388266/smoke 43106388225, permitting continuation.

Selection, part and contact rendering in project_letters now passes existing
study/OccurrenceId components to a private helper rather than constructing an
owning temporary OccurrenceKey. Public occurrence_text delegates to the same
helper; nil-study/path-length rejection, error message and output format are
unchanged. The output still owns its text. No public API, domain attachment,
independent snapshots or owner refusal/exact-retry behavior changes.

Tests explicitly cover normalized uppercase UUID input, UINT64_MAX entry,
UINT32_MAX byte ordinal, zero ordinals, separators, nil study and invalid path
lengths 1/3. Existing source-death and projection/structural controls remain active.
A focused local probe compares the old aggregate-key call shape with the new
component call and requires identical text. For aggregate/byte cases, UUID-sized
37-byte requests fall from 2 to 1; the byte case's 8-byte path request falls from
1 to 0. These size-filtered observations support the bounded source change; output
string allocation remains and zero total allocation is not claimed. Probe source,
commands, binary and build/output logs are archived.

#279 remediation is separate from product validation: original combined-head native
43098106376, viewport 43098106428 and smoke 43098106405 remained queued without
jobs. Both cancellation endpoints returned 409 (not queued yet); rerun returned
403 (already running). Close/reopen preserved reviewed head `5f1ca0c3` and triggered
fresh native 43115507493, viewport 43115507480 and smoke 43115507510. Old attempts
remain preserved, never relabeled. The original API snapshots are retained locally
as setup-evidence/pr279-run-*-before.json. No empty commit, rewrite or descendant
suite rerun was used to repair the queue. Integration requires maintainer disposition.

Owner metrics remain unchanged in all four CPU configurations: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251, with 3271/5927 failure sites fully reclaimed. These owner
observations do not prove the initial-rendering allocation removal.

The 2-direct-exchange_commit log contains one Taichi ticache.lock warning after
its final case_failures=0 PASS marker. The verifier explicitly checks that order
and permits only that inspected diagnostic; it remains in the archive. No cache
cleanup or rerun was used to erase it. No build/probe warning or product error.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five
cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting native
handoff/snapshot checks and six warm CTests. Clang15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no GPU/DB/browser-model
or performance qualification. The browser was used only to inspect the CI queue.
Final tested-to-head delta is four Markdown records. Verified archive
`setup-evidence/occurrence-rendering-domain-fa3bef8f.tar.gz`; manifest digest:

    70798865e9d3a83f485c7ece145ba9f61e721b9f1cb1051f69b0ee86bb0ab5b4

Raw local artifacts have not been independently inspected remotely. Hosted
qualification/review remain required. At final recheck, #279 fresh smoke
43115507510 and viewport 43115507480 passed; native 43115507493 had passed build
and projection-values and was running its matrix. No merge is inferred.


### Maintainer integration through #284

**Maintainer integration — 2026-11-11:** Reviewed #275–#277 and #279–#284 are integrated into main at `5beeb2cacf28c75cbecef385a0de1e877154bbc0`, preserving original commits through reviewed tip `09bafe48869ddf239e6e245d766c0c0e41a43364`. #279's fresh native 43115507493, viewport 43115507480 and smoke 43115507510 succeeded; #280–#284 completed their applicable hosted checks, including #284 native 43116769390 and smoke 43116769371. Original failed and stuck queued attempts remain historical, not relabeled. Earlier pending/integration labels below are superseded by this record. #285 is provisionally source-accepted at `7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` (review 5866915489), targets main and awaits native qualification; its smoke passed. Next: initial tool/occurrence indexes and publication/setup costs, then membership callable/recursion, declaration/binding budgets, combined resource accounting and operation integration. No blanket GPU or performance acceptance.


### Temporary provenance/equivalence qualification (2026-11-11)

Tested `f81dd2e383bff17188033c0e9108e7da80a39186`, stacked on provisionally accepted
#284 head `09bafe48869ddf239e6e245d766c0c0e41a43364` under review 5866703067.
Current main remains `6067c641`; #279 is still the pending integration PR.

Temporary initial-projection lookup values now retain owner entry/ordinals and
borrow character/equivalence ToolRefs from the const input's stable tool/parent
tables. They do not point into returned expansion vectors. Published occurrence
provenance/equivalence construct independently owned identities; no pointer escapes.
Lookup keys, order and published record copying remain unchanged. The strengthened
source-death test compares full ToolRefs against original request/parent values,
including pinned revisions, for all five letters after the actual source dies.
Existing structural source-death, repeated/multibyte and malformed-input tests apply.

A local source-to-source probe constructs the same five-letter recipe before
observing C++ new requests of 37 bytes during project_letters. Prior #284 source
reports 840, current source 795, both producing 13 byte rows and three equivalence
entries. The script requires exactly 45 fewer requests: three UUID character
copies per provenance entry and two per equivalence entry. This size-filtered
observation supports the bounded source change, not zero allocation, physical
memory or performance. Commands, both source versions, builds and results are retained.

The output strings and all published identities remain owned. Owner adoption begins
after initial projection, so unchanged adoption counters cannot establish this
removal. Initial tool/occurrence indexes and publication/setup costs remain separate;
membership callable/recursion and declaration/binding budgets precede combined
resources and remaining-operation integration. No GPU, live DB or browser work.

All four owner configurations preserve #284 metrics: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251, and 3271/5927 failure sites fully reclaimed. These reports
overlap and must not be summed. They qualify owner behavior, not initial projection
allocation removal or performance.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five
cases, workers 1/2, bulk/direct), 12 prepared checks, nine supporting native
handoff/snapshot tests and six standalone warm CTests. No build/test/probe warnings
or errors. Clang15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. No GPU or performance claim.
Final tested-to-head delta contains four Markdown records. Verified archive:
`setup-evidence/projection-provenance-domain-f81dd2e3.tar.gz`; manifest digest:

    6713a423634b9f96b3de7c84295a906c7ec71ab19c0e6910d3c23359a2468968

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. At final #279 check, fresh smoke
43115507510 and viewport 43115507480 passed. Native 43115507493 had passed build,
projection-values, all four compiler jobs, database and regression shards 2/3;
shards 1/4 remained running. The original stuck attempts are preserved. No merge
or final qualification is inferred from the partial matrix.

### Initial tool-index qualification (2026-11-11)

Tested `fc9d870b85182ee272d9e09b0c96c665d41b3a90`, continuing #285 reviewed head
`7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` under review 5866915489. Accepted main
`c2c168b3` was included by normal merge `9f88ce31a450895cf1df3df77244285c323f907d`.
That merge changes only four Markdown records relative to #285, preserving both
integration-through-#284 and #285 evidence histories; no product source changed
in reconciliation. This qualification covers the resulting combined product head.

Temporary Tools keys now use reference_wrapper<const ObjectKey> into the stable
source tool table, like their existing mapped Tool pointers. The transparent
comparator calls ObjectKey's value comparator, preserving ordering/validation and
value-based lookup. Neither normalization nor projection mutates tools/parents
before index consumption finishes; published identities remain independent.
Normalization duplicate tests cover original/reversed source order. Existing
stale-revision rejection and both recipe versions' source-death controls apply.

A focused source-to-source index probe builds 22 tools before observation. Current
index construction has zero 37-byte requests; prior #285 source has 22 and fails
the zero-request predicate. Lookup through separately copied ToolRefs/ObjectKeys
must return the correct source tool for both versions, guarding against accidental
pointer-identity lookup. This size-filtered witness supports the source change;
map nodes still allocate, and no zero-total/setup-budget/performance claim follows.
Both source versions, commands, binaries and logs are retained in the archive.

Initial occurrence indexes and publication/setup costs remain open, followed by
membership callable/recursion, declaration/binding budgets, combined resources and
remaining operations. No GPU, live database or browser work. Earlier failure and
warning records remain preserved.

All four owner configurations retain the prior metrics: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251 and 3271/5927 failure sites fully reclaimed. Initial indexing
is outside adoption; these unchanged totals do not establish the index reduction.
Reports overlap and must not be summed.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six standalone warm CTests. One ticache.lock warning follows the final
PASS marker in 1-bulk-exchange_commit.log; it is retained, with its order explicitly
verified. No build/probe warnings or product errors. No rerun/cache cleanup was
used to suppress the warning. Clang15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host, no performance claim.

Final tested-to-head delta is four Markdown records. Verified archive:
`setup-evidence/projection-tool-index-domain-fc9d870b.tar.gz`; manifest digest:

    8b7e28eabe22bf123531ec70f114717f85f488c02cdbcef64644443d8d90a5ae

Raw artifacts have not been independently inspected remotely. Hosted qualification
and review remain required for this contribution. Parent #285 exact-head native
43121173964 and smoke 43121173792 now report success, and Patrick has accepted
that contribution; the PR remains open at inspection. Earlier pending/failed/stuck
attempts remain historical. #279 and the stack through #284 are integrated, not
still pending. Initial occurrence indexes/publication setup are the next scope.


### Maintainer acceptance through #286 — 2026-11-12

**Maintainer acceptance — 2026-11-12:** #285 and #286 are integrated into main at `087b6aff14d454c9ebafcda28c9e9243f596e4d2`, reviewed tip `49643f2a22bbb0076584e01d990a32ff7fe0acda`. Exact-head #286 native 43130677310 and smoke 43130677322 passed; #285 native 43121173964 and smoke 43121173792 also passed. All 12 #286 native jobs succeeded. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 130894721312 reports addressed_reads=24/private_reads=0. The active review queue is clear; #75, #111 and #123 remain deliberately held. Earlier provisional/pending labels below are historical and superseded by this acceptance. No GPU or performance qualification is implied.

**Next bounded task:** start from current main and audit initial occurrence indexes in `project_letters` and their publication/setup costs. Preserve value ordering, duplicate/revision rejection, source stability, exact provenance and independent published outputs; distinguish removable temporary copies from necessary owned publication storage. Keep the implementation bounded with source-death and independent allocation evidence. Membership callable/recursion, declaration/binding setup budgets, combined resource accounting and remaining operation/control integration follow. Do not repeat accepted baseline tests merely to onboard.


### Ordered occurrence-value qualification (2026-11-12)

Tested `ca10e9ed8b50db66a9d4d7fb8de8dc6ac6160095`, based directly on accepted main
`35814207e736506812bb2a5aec57a387cbc5b4ba`. #285/#286 were integrated at
`087b6aff14d454c9ebafcda28c9e9243f596e4d2`; the maintainer records all 12 #286 native
jobs passed, 115 inspected regression tests and database addressed_reads=24/
private_reads=0. Those are prior accepted evidence, not rerun for onboarding.

The ordered index retains its owned OccurrenceId key but stores only a borrowed
source definition pointer and node as value. It no longer duplicates the ID/path
or owns another ObjectKey inside a temporary StructuralOccurrence. Both legacy and
structural input rows remain const/stable until consumption; all published keys,
definitions, revisions, text and recipe copies retain independent ownership.
Iteration order and existing duplicate/revision validation are unchanged.

Both recipe-version tests now compare every published occurrence's full ToolRef and
node to the normalized input after the actual projection source copy dies. Existing
provenance, byte order, exact mass, stale/duplicate and owner lifetime/refusal/retry
controls remain active. The new checks short-circuit before dereferencing a missing
source row.

A size-filtered source-to-source project_letters probe over 18 occurrences (13 byte
paths) observes main/current UUID-sized requests 773/755 and path-sized requests
223/210. The script requires deltas 18 and 13 respectively. Output still allocates;
these observations are not total backing, physical-memory or performance evidence.
An initial helper retained a stale #284 source pin and compared 840/755 UUID-sized
requests, failing the expected delta. That helper/source/binaries/logs are preserved
with initial- prefixes. Correcting the helper baseline to current main passes;
product code/builds were unchanged and no product test failed. The corrected and
initial commands/results are both archived.

Owner metrics stay unchanged across four CPU configurations: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251 and 3271/5927 failure sites fully reclaimed. These overlapping
reports must not be summed and cannot establish initial-projection savings.

Remaining owned occurrence keys, legacy/used-definition lookups and publication/
setup costs remain separate, followed by membership callable/recursion and declaration/
binding budgets before combined resources and operations. No GPU, live DB or browser.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/test/probe warnings or product test failures;
the initial helper comparison failure is separately preserved above. Clang15/
libstdc++, CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/projection-occurrence-index-domain-ca10e9ed.tar.gz`; manifest digest:

    47b3bce6b2c632395ced430c42521af146ec0ba1d7ada3def6183a61a53d3ef9

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required for this new contribution. There is no
pending predecessor implementation PR; this branch targets current main directly.

### Legacy occurrence lookup qualification (2026-11-12)

Tested `7bebf877bdd52f48964a3ace99a1da898562322f`, continuing from #287 reviewed head
`291e721b5ab5685cc17f8adc1c75213b0b466bb5` under review 5874475520 (no blocker,
provisional continuation permitted). Main remains `35814207`; #287 is unmerged.

The ordered OccurrenceSource carries the const source legacy row pointer, replacing
the second legacy OccurrenceId map. Structural rows carry null and never enter the
v1 state branch. Initial warm::validate enforces recipe-version separation. Input
rows remain stable/const; publication copies numerical state and identities into
owned output. No key ordering, revision validation or public ownership change.
The ordered value gains a pointer; eliminated allocation calls are not total bytes.

The new legacy source-death control assigns every occurrence distinct finite xyz
positions/velocities, projects from a local source copy, destroys it, binds the
projection and verifies every component, node and encoded occurrence association.
Both recipe versions' definition/provenance source-death controls remain active,
including structural no-slot behavior and rejection of legacy binding for v2.

A focused source-to-source v1 projection probe against #287 observes total C++ new
calls 2092→2061, path-sized requests 210→197 and UUID-sized requests unchanged at
755. The script requires deltas 31/13/0 over 18 occurrences and 13 byte paths.
The removed 18 map nodes and 13 copied paths agree with source inspection; this is
not zero-total allocation, physical-memory or performance qualification. Commands,
source versions, binaries and results are retained. No helper failure or warning.

Remaining owned occurrence keys, used-definition lookups and publication/setup costs
remain separate before membership callable/recursion, declaration/binding budgets,
combined resources and operation integration. No GPU, live DB or browser work.

Owner metrics remain unchanged in all four CPU configurations: single/four selection
selected live/peak 321185/392475, work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251, with 3271/5927 failure sites fully reclaimed. These overlapping
reports must not be summed; initial legacy indexing is outside owner adoption.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/test/probe warnings or errors. Clang15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Final tested-to-head delta contains four Markdown records. Verified archive:
`setup-evidence/projection-legacy-index-domain-7bebf877.tar.gz`; manifest digest:

    f85a12b245d94fe8d4d6661675a43ef253c268d09c5f13c7c39d408d9541b07d

Raw artifacts have not been independently inspected remotely. Hosted qualification
and review remain required. Parent #287 smoke 43218258474 passed; native 43218258553
reported queued in its summary at recheck, but its jobs showed successful build,
projection-values and database plus running/queued matrix jobs. This is active
qualification, not the former no-job queue failure; no retry or merge was made.


### Used-definition lookup qualification (2026-11-12)

Tested `dfcd04f47182ac8b4ef5762ec522b5aa2467bdc4`, continuing from #288 reviewed head
`8772552d523f7b3239ef53705f1a630da2536817` under review 5874675639 (no blocker,
provisional continuation permitted). Main remains `35814207`; #287/#288 are unmerged.

The temporary used-definition set borrows const ObjectKeys from stable input
occurrence rows. The existing ToolKeyLess comparator compares values, preserving
ordering and deduplication across distinct source objects. No loop-local synthetic
key is borrowed. Source validation, revision rejection and independently owned
published definition text, geometry and parent records remain unchanged.

Both recipe versions project from a local source copy that is then destroyed.
New controls compare bound definition IDs against an independent std::set of encoded
source keys, checking exact unique membership and order. Existing full reference,
revision, provenance, numerical-state and v2 no-slot controls remain active.

A source-to-source v1 projection probe against #288 uses 18 occurrences, 13 byte
alternatives and 12 distinct definitions. UUID-sized requests fall 755→743; C++ new
calls fall 2061→2037; path-sized requests remain 197. The script requires one fewer
UUID-sized request per distinct definition, unchanged path counts and fewer total
calls. This supports the temporary-copy removal, not total-byte, physical-memory
or performance claims. Commands, both sources, binaries and results are retained.
Public layouts are unchanged: StudyInput 288, StructuralStudyInput 280,
LetterProjection 1304, Uuid 48, ObjectKey 88, ToolRef 136, Tool 432, Parent 168,
Geometry 152 and Bundle 520 on this toolchain; UUID payload is 37 bytes.

Owner metrics are unchanged across all four CPU configurations: single/four
selection live/peak 321185/392475, allocation work 420751/790263, byte/work
boundaries 2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555,
census 321153/392251. All 3271/5927 failure sites reclaim every block; failure peaks
are 321250/392540. These overlapping reports must not be summed. Initial projection
occurs before adoption, so its savings are not a change to the selected owner quota.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/test/probe warnings or errors. Clang15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/projection-used-definitions-domain-dfcd04f4.tar.gz`; manifest digest:

    4a6718968f304d9732e27552d00669b0da029890d66da3ff1ba299d4e9adc930

Raw artifacts have not been independently inspected remotely. Hosted qualification
and review remain required. #288 smoke 43220088002 passed and native 43220088562
was in progress at inspection; no duplicate retry or merge was made. No GPU, live
DB or browser work. Remaining occurrence-index keys and publication/setup costs
precede membership callable/recursion, declaration/binding budgets, combined
resources and remaining-operation integration.


### Stable occurrence-index key qualification (2026-11-12)

Tested `644373efc54c4d1e12461c23cea9d8afdfcaa65b`, continuing from #289 reviewed head
`34db84f180eaa4b8e33cae313fb56e0962cc0a29` under review 5874861380 (no blocker,
provisional continuation permitted). Main remains `35814207`; #287–#289 are unmerged.

The ordered-occurrence map borrows IDs from const input occurrence rows. The
byte-provenance map borrows IDs from const selection part rows. Both tables remain
stable for the call. OccurrenceIdLess delegates to the existing value comparator;
lookups compare values across distinct source objects. The aggregate passed to
ordered.at is only a transient search key, never inserted. Active/equivalence indexes
retain owned keys and their separate synthetic-key lifetime problem is deferred.
Published occurrence keys, provenance, input text and recipe copies remain owned.

V1/v2 source-death tests now reverse the actual projection source occurrence rows,
destroy that source, then compare published IDs with independent source copies
sorted by occurrence value. Existing complete definition/node, ordinal/provenance,
state and v2 no-slot checks remain. Aggregate lookup uses a separately constructed
ID; provenance lookup uses an occurrence-row ID distinct from the selection-part
object stored as its key. These exercise value lookup rather than pointer identity.

The source-to-source v1 probe against #289 covers 18 occurrences and 13 byte paths.
Path-sized requests fall 197→171 and C++ new calls 2037→2011; UUID-sized requests
remain 743. The script requires deltas 26/26/0, consistent with removing two copied
paths per byte occurrence. Public layouts are unchanged (StudyInput 288,
StructuralStudyInput 280, LetterProjection 1304; ObjectKey 88, ToolRef 136).
Commands, sources, binaries and logs are retained. This is temporary-copy evidence,
not a total-byte, physical-memory or performance claim.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/test/probe warnings or errors. Clang15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Owner metrics remain unchanged in all four CPU configurations: single/four selection
live/peak 321185/392475, allocation work 420751/790263, byte/work boundaries
2761/3269 and 3707/5925, retained/peak 321265/321265 and 392555/392555, census
321153/392251. All 3271/5927 failure sites fully reclaim their blocks, with failure
peaks 321250/392540. These overlapping reports must not be summed; initial
projection occurs before adoption and its savings are outside the owner quota.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/projection-borrowed-occurrence-keys-domain-644373ef.tar.gz`;
SHA256SUMS digest:

    f728680cb034dbe60b7c5020d7da0e8703ea1459d33d7dd9392b626c6ae4d199

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. #289 smoke 43222858266 passed. Its native
43222858150 summary reported queued, but inspected build/projection-values jobs
passed and regression jobs were active. No duplicate retry or merge was made.
No GPU, live DB or browser work. Remaining active/equivalence synthetic-key lifetime
and publication/setup costs precede membership callable/recursion, declaration/
binding budgets, combined resources and remaining-operation integration.


### Selection metadata consolidation qualification (2026-11-12)

Tested `d1c61a5eab9164c1b9db54209a601c8d961090dc`, continuing from #290 reviewed head
`070f5a6c523fcdbc3e627dad6691f74eeb6e489a` under review 5875081581 (no blocker,
provisional continuation permitted). Main remains `35814207`; #287–#290 are unmerged.

OccurrenceSource now contains an initialized activity flag and optional borrowed
complete-character pointer. Selection processing resolves aggregate/part values in
the ordered map, sets activity and attaches the source single-parent ToolRef even
for inactive aggregates. No synthesized key is stored. Publication copies the pinned
reference into owned output; the provenance map is unchanged. The separate active
set and equivalence map are removed. V1/v2 source-death controls verify exact
Aggregate/Parts activity and complete-reference presence/revision, including shuffled
mixed input, inactive aggregates and bytes. Existing repeated/multibyte, ordering,
v1 initial-state, v2 no-slot and malformed/revision controls remain active.

The focused v1 source comparison against #290 measures 18 occurrences, 13 byte paths
and three complete-character aggregates in both exposure cases:

| Observation | Aggregate prior/current | Mixed prior/current |
|---|---|---|
| Active occurrences / active byte paths | 5/0 in both | 13/12 in both |
| C++ new calls | 2011 / 2003 | 2124 / 2096 |
| Path-sized requests | 171 / 171 | 218 / 206 |
| UUID-sized requests | 743 / 743 | 775 / 775 |
| Cumulative requested bytes | 181845 / 181325 | 191597 / 190341 |

The script requires removed calls = active rows + equivalent rows + active byte
paths, removed path requests = active byte paths, and unchanged UUID counts.
Probe-only sizeof instrumentation records metadata 24→32 and actual libstdc++
ordered-node type 64→72, adding 144 requested node bytes across 18 rows. Original
and instrumented sources, commands, binaries and logs are retained. The observed
cumulative byte differences (520/1256) include this growth; they are not retained/
peak memory, allocator overhead, other allocation APIs or performance evidence.
Public layouts remain unchanged. No probe failure or diagnostic was observed.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/probe errors or product test failures.
Two Taichi cache-lock warnings occurred after the final PASS in the one-worker bulk
and direct exchange_commit logs. Both are preserved; verification permits exactly
those two inspected post-PASS diagnostics and rejects other warnings/errors. No
cache cleanup or test rerun was used to erase them. Clang15/libstdc++, CPU/x64,
Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Owner metrics remain unchanged across all four configurations: single/four selection
live/peak 321185/392475, work 420751/790263, byte/work boundaries 2761/3269 and
3707/5925, retained/peak 321265/321265 and 392555/392555, census 321153/392251.
All 3271/5927 failure sites fully reclaim blocks; failure peaks 321250/392540.
These overlapping reports must not be summed; initial projection precedes adoption.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/projection-selection-metadata-domain-d1c61a5e.tar.gz`;
SHA256SUMS digest:

    28fcd7ab7c80dceb7b653d268eb4e37f67fcec6b6bcf5843e5f600ad7d4fdb6e

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. #290 smoke 43225419276 passed and native
43225419105 was running at review; no duplicate retry or merge was made. No GPU,
live DB or browser work.

Next bounded audit: local ProjectedOccurrence record copying and OccurrenceKey
staging versus necessary owned outputs. Definition/Selection staging already moves;
retained recipe and public input/parent/identity stores remain owned. Stop initial
projection cleanup at that explicit boundary, then proceed to membership callable/
recursion and declaration/binding setup budgets, combined resources and operation/
control integration. This contribution does not complete the owner setup gate.


### Publication staging qualification and audit stop (2026-11-12)

Tested `c55398a0c88b3bc3f753a9598bc139ef26547977`, continuing from #291 reviewed head
`f340c720e9ea99428576613fd53ef7676cf87bdf` under review 5875329810. Accepted main
`b234514df310f0f1dedebb3f8a2110a73b884f37` (through #288) is included by normal merge
`7cb3117329626318f0caff7d199c95a52ec13430`, which changed no files relative to #291.
Live PR state confirms #287/#288 merged; #289–#291 remain unmerged.

The local ProjectedOccurrence constructs its owned key directly. A const reference
to that local key serves all text and parent consumers; record.active is read before
the completed record moves into occurrences_. No moved-from reads or output-vector
references cross reallocation. Output recipe, keys, full references, provenance,
parent values and v1/v2 input remain independently owned. New v1/v2 source-death
checks verify every active parent's full occurrence and field, ordinal and count,
plus no missing/extra rows. Existing shuffled/mixed exposure, repeated/multibyte,
pinned references, slots, v1 initial-state and v2 no-slot controls remain active.

The v1 comparison against #291 covers 18 occurrences, 13 byte alternatives and three
complete-character aggregates in both exposure modes:

| Observation | Aggregate prior/current | Mixed prior/current |
|---|---|---|
| C++ new calls | 2003 / 1826 | 2096 / 1919 |
| UUID-sized requests | 743 / 626 | 775 / 658 |
| Path-sized requests | 171 / 145 | 206 / 180 |
| Cumulative requested bytes | 181325 / 176278 | 190341 / 185294 |

Required deltas are 177 calls, 117 UUID requests and 26 path requests in each mode.
Source inspection accounts for 4 UUID copies per occurrence, 3 per byte and 2 per
complete-character reference, plus 34 address and 26 path copies. Request totals
include all observed ordinary C++ new calls during projection; they are not retained/
peak memory, allocator overhead, other allocation APIs or performance evidence.
Public layouts and metadata/node sizes (32/72) are unchanged. Original sources,
probe-only sizeof-instrumented copies, commands, binaries and logs are retained.

Both builds and all 47 scoped CPU checks passed: 20 exchange processes (five cases,
workers 1/2, bulk/direct), 12 prepared checks, nine supporting native handoff/snapshot
checks and six warm CTests. No build/test/probe warnings or errors. Clang15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; shared host.
Owner metrics remain unchanged across all four configurations: single/four selection
live/peak 321185/392475, work 420751/790263, byte/work boundaries 2761/3269 and
3707/5925, retained/peak 321265/321265 and 392555/392555, census 321153/392251.
All 3271/5927 failure sites fully reclaim blocks; failure peaks 321250/392540.
These overlapping reports must not be summed; initial projection precedes adoption.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/projection-publication-move-domain-c55398a0.tar.gz`;
SHA256SUMS digest:

    9158b079b30951cbe046b5fbcb9518ba1e309d2b0a3d8d020e7bd8e6fc0eb08d

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. #291 smoke 43228339158 passed and native
43228339116 was running at review. No duplicate retry or merge of pending slices.
No GPU, live DB or browser work.

The bounded initial-projection copy audit stops here. Definition/Selection staging
already moves; retained recipe, identity/provenance, parent and v1/v2 input remain
owned. Next: MembershipFields::collect recursive follow callable storage and
traversal/stack work, preserving pinned references, cycles, memoized diamonds and
exact contribution ordering/ownership. Declaration/binding setup budgets, combined
resources and remaining operation/control integration follow. Initial-projection
savings do not establish a complete owner setup gate.


### Membership traversal setup qualification (2026-11-12)

Tested `8d1e1802faa84e855677d7efcf5b0ff4483d0e81`, continuing from #292 reviewed head
`ced6c2d891b5c8d0497bfd9d10e3edc1a473e8f1` under review 5875540117. Accepted main
`a4de248e18d07284198c2a963af4ee458b629ffb` through #289 is included by normal merge
`7be5b888c735284b26fefd4cad492dbcddef1879` with no file delta relative to #292.
#290–#292 remain unmerged; the initial-projection copy audit remains closed.

Membership collection uses nonrecursive local lambdas and domain-charged explicit
DFS frames. Frame depth cannot exceed the input tool inventory; allocation-work
and live-byte admission bound backing growth, including old/new buffers. Child
visits may grow the vector, so parent frames are reacquired by index. Tool/state/
reached pointers refer to stable source/map nodes. Pins are validated on every visit,
including memoized visits. Visiting-state cycle rejection, deduplicated diamond
ancestors, ordered contributions and active-only/alternatives behavior remain intact.

PreparationWork charges before catalogue copies, key-payload comparisons, visits,
pinned equality, ancestor insertions, owner lookup, output construction and
conservative frame/contribution relocation (including nested output payloads).
Owner adoption passes its existing setup allowance and reports Membership phase
refusals through the established unwind boundary. The phase sweep now covers all
six incomplete phases. Standalone collection/ExchangeBindings use an unrestricted
logical-work allowance; they are not claimed as owner admission. Domain overhead,
declaration/binding constructor work and combined borrowed accounting remain open.

The focused mixed-diamond test records 56744 work units, 3151 work-refusal boundaries
and 17967 peak domain bytes. Every work and allocation boundary refuses and releases
all blocks; exact allowance retries reproduce ordered full-reference contributions.
It checks one-short work refusal, stale pins on memoized nodes, cycles and a 192-level
public chain with repeated terminal reachability, deterministic order and complete
teardown. Existing 32 exposure combinations, revision and ownership controls remain.
The focused executable passes 3793 checks; it is also part of the supporting suite.

Initial tested `2438647a91f7a22d445d0116c917fffb93e62f19` passed both builds and its
budget/retry tests, then the deep-chain fixture failed with private dependency escape
before collection. The fixture improperly connected a public letter to independently
scoped private groups. Only test identities changed to a separate public address
range at final tested source. The original source, binary and logs are preserved in
`setup-evidence/membership-traversal-budget-initial-2438647a.tar.gz`, manifest:

    ed3e112124fe1718a18acc399b8003417d28b982014330dd5c3f34eb88bc59f7

All four CPU modes report these single/four-selection owner changes compared with
the retained #292 evidence (not rerun):

| Observation | Single selection | Four selections |
|---|---|---|
| Setup work | 58925 → 85804 | 150127 → 211363 |
| Setup refusal boundaries | 1556 → 3072 | 4628 → 7904 |
| Selected allocation work | 420751 → 420871 | 790263 → 790383 |
| Selected byte/work boundaries | 2761/3269 → 2759/3271 | 3707/5925 → 3706/5927 |
| Independent failure sites | 3271 → 3272 | 5927 → 5928 |

All failure sites reclaim every block. Selected live/peak remain 321185/392475;
retained/peak remain 321265/321265 and 392555/392555; census 321153/392251 and
failure peaks 321250/392540 remain unchanged. Allocation-work growth includes the
new charged frame backing; fewer implicit callable allocations is not a total-memory
claim. These overlapping reports must not be summed. Work units are logical admitted
operations/payload, not elapsed time or all allocator overhead. Public projection
layouts remain unchanged (StudyInput288, StructuralStudyInput280, LetterProjection1304).

Both builds and all 47 scoped CPU checks passed on the corrected source: 20 exchange
processes (five cases, workers1/2, bulk/direct), 12 prepared checks, nine supporting
native handoff/snapshot checks and six warm CTests. No corrected-head build/test
warnings or errors. Clang15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; shared host. The focused run above is
also included in the supporting suite, not counted as a new distinct suite.
Final tested-to-head changes are four Markdown records. Verified archive:
`setup-evidence/membership-traversal-budget-domain-8d1e1802.tar.gz`;
SHA256SUMS digest:

    089fe2f0d085b9207ab5e861e47b807aae406ff0383e511876cd164dd85dd218

Raw local artifacts have not been independently inspected remotely. Hosted
qualification and review remain required. #292 smoke 43230769945 passed and native
43230770028 is in progress at inspection; no duplicate retry or merge of pending
slices. No GPU, live DB or browser work.

Next: declaration collection and ExchangeBindings constructor work budgets, then
combined resources and operation/control integration. This contribution bounds the
membership portion of owner setup, not the complete owner constructor.

### Declaration setup admission and GCC 11 cleanup (2026-11-12)

Tested source `a867c98b27bfba55226c8820a053b890d6b58018`, continuing from #293
`b0f2f289071def1b6f65e69fba56549cfc59d0db` under review 5875958258. Accepted main
remains `a4de248e18d07284198c2a963af4ee458b629ffb` through #289, already included
in this stack. #290-#293 remain unmerged at inspection; no acceptance is inferred
from local tests. Initial-projection copy cleanup remains closed.

Declaration collection now receives the owner's shared PreparationWork meter and
reports a distinct Declarations phase. Selection, occurrence and tool searches,
identity comparisons/copies, source/parent/leaf accumulation, vector growth and
conservative payload relocation are admitted before execution. Reverse copying
and shared declaration publication use the same meter. Exact pinned identities,
ordered contributions, mass and later matches() allowance remain unchanged.
Standalone declarations remain unrestricted. ExchangeBindings construction,
staging and capture still need their own setup-meter integration.

The four-owner focused fixture covers aggregate and parts declarations, including
repeated leaves and maximum entry ID. It compares every ordered source, parent,
leaf, revision, mass and matching allowance after source destruction. Collection
and reverse copying each sweep every work, live-byte and allocation-work refusal,
require full domain reclamation, then retry with exact allowances; one-short work
also refuses. All 4012 structural checks pass. Collection work/refusal-boundary
pairs by owner are 7445/276, 7156/218, 35116/1009 and 25008/819; reverse-copy pairs
are 2214/49, 2214/49, 8446/157 and 7016/132. Owner phase checks cover all seven
incomplete setup phases and retain caller/native state on refusal.

#### Hosted failure and correction

#293 smoke 43234968370 passed, but native run 43234968384's projection-values job
131239777540 failed under Ubuntu 22.04/GCC 11.4: 232 membership cleanup assertions
failed. It is not a qualified parent merely because its Clang/native build passed.
The local GCC 11.5 standalone build reproduced the same 232 failures and exposed
a declaration work-refusal cleanup failure at initial source
`10df7d7b458f0290ba1d818d961c0efabcb68eca`. Local GCC 13.3 and Clang 15 passed that
source. This compiler-dependent failure remains preserved, not relabeled.

Allocating nested aggregate expressions did not unwind every earlier identity
allocation under GCC 11. The correction gives each copied scope/address/revision,
recipe pin and source payload its own local owner before assembling rows with
moves. Membership contributions also have an explicit local lifetime before
vector insertion. No failure assertion or work/allocation limit was relaxed.
The membership sweep now passes all 3793 checks on GCC 11.5 and GCC 13.3, with
work=56744, boundaries=3151 and peak domain bytes=17967 on each compiler.

Both standalone compiler configurations pass all three CTests at the final pin;
the focused membership/declaration reruns provide detailed counts and are not
counted as additional distinct suites. New LOCAL-CI guidance includes GCC 11
alongside the newer compiler. These Ubuntu 24.04 local results do not replace the
hosted Ubuntu 22.04 job. Verified standalone archive:
`setup-evidence/declaration-standalone-a867c98b.tar.gz`, SHA256SUMS digest:

    74c16f8ad2f722a583cb3033cf66e14d156680b22f5086591bf14c50c89dc82a

The archive includes failed GCC 11 and passing GCC 13 logs from the initial pin.
Its final packaging command first failed because of a Windows line ending; the
verified evidence was subsequently archived without rerunning or relabeling tests.
The initial Clang 47-check pass is separately retained in
`setup-evidence/declaration-setup-budget-domain-10df7d7b.tar.gz`, manifest:

    f7d46838d0af9b08a0477b74e4496dd6b2dfd0922703ead7ca06ee8a7d237cdc

Final native CPU qualification results follow below. Raw local artifacts have
not been independently inspected remotely. No GPU, live database or browser work;
all measurements are on a shared host and are not performance qualification.

All four completed owner modes report single/four-selection setup work
85804/211363 -> 104737/287803 and refusal boundaries 3072/7904 -> 3592/10115.
Selected allocation work stays 420871/790383; selected live/peak stays
321185/321185 and 392475/392475. Byte/work boundaries stay 2759/3271 and 3706/5927.
Independent failure sites remain 3272/5928 with zero surviving blocks; retained/
peak remain 321265/321265 and 392555/392555, capacity census 321153/392251 and
failure peaks 321250/392540. These overlapping requested-resource observations
must not be summed or interpreted as physical memory or timing guarantees.

Both final builds and all 47 scoped CPU checks passed: 20 exchange processes
(five cases, workers 1/2, bulk/direct), 12 prepared checks, nine native supporting
checks and six warm CTests. The six standalone compiler CTests above are additional.
Clang15/libstdc++, CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`;
accelerator backends disabled. One offline-cache lock warning appears after the
successful matrix result in `1-bulk-exchange_commit.log`; the verifier checks its
location/count and rejects any other warning/error. No build warnings occurred.
The warning and raw logs remain intact. Layouts are unchanged: study/structural
input 288/280, projection 1304, UUID/ObjectKey/ToolRef 48/88/136, Tool/Parent/
Geometry/Bundle 432/168/152/520, UUID payload 37 bytes on this toolchain.

Verified final archive:
`setup-evidence/declaration-setup-budget-domain-a867c98b.tar.gz`, SHA256SUMS:

    4390daafb8e2364c2c113da94eb905baf695df3f9d75f4323e05a493fd89546f

Final tested-to-head changes are five Markdown records only. Hosted qualification
and review remain required, including the corrected combined head for the #293
failure. Next: ExchangeBindings constructor/staging/capture under the shared setup
allowance, then complete combined resource accounting and operation integration.

### Binding setup admission qualification (2026-11-12)

Tested source `803437be28971c9efcd80ded72c2eae3a05eecae`, continuing from #294
`32fe9928cdbed11b5092724d81f3bfbff07f0b9d` under review 5876572990. Current main
`7bf08034` updates the maintainer handoff while runtime integration remains through
#289. Normal merge `cdcf28e0` includes those notes; conflicts were limited to the
two status documents and retain the current maintainer disposition and prior
contribution records. #290-#294 remain unmerged at inspection.

Temporary declaration-validation bindings and the retained coordinator/binding
now receive the owner's existing PreparationWork meter. Bindings is a distinct
phase. Work admission precedes mass/handle/edge and packed-mask reserves, trusted/
staged map copies, map inventory traversal, outer slot growth, nil UUID creation,
nested active/inactive identity reservation/copy, and staging/capture. Retained
coordinator command/hierarchy/field-index reserves are included before allocation.
Packed masks conservatively charge a byte per requested bit plus a rounding word;
these are logical work units, not measured physical bytes or elapsed time.

The adoption path no longer resets staging to UINT64_MAX or supplies an independent
unrestricted capture allowance. Shared capture logic throws setup refusal to owner
adoption; the operational wrapper retains its separate bounded CaptureWork result.
Existing source/pin/handle checks, exact participation, stale-generation rejection,
matching budgets and later operational staging/capture/command limits are unchanged.
Standalone construction/prepare and ordinary map snapshots remain explicitly
unrestricted conveniences. The initial snapshot-copy gate remains separate.

New focused map controls exercise construction, outer growth, inactive nested
reservation, explicit-domain copying and slot shrink. Each map kind traverses
44 refusal boundaries with complete reclamation, unchanged source identity and
one-short/exact retry. Occurrence/field work totals are 2733/2703. The full owner
setup sweep requires all eight incomplete phases, logs binding-phase refusals,
checks native readiness/generation/tick preservation, complete C++ allocation
teardown and exact/one-short retry. Existing independent byte/allocation failure
and later operational preparation/commit/retry controls remain active.

Final CPU results and archive follow below. No new standalone GCC projection run
is claimed: this slice changes the native binding/coordinator path and its owner
controls, not the standalone projection implementation. #294's hosted GNU 11.4
projection correction remains separately qualified under job 131259756048.
#294 smoke 43240639817 passed; native 43240639938 remains in progress at inspection.
#290's maintainer-requested failed-job retry is separate; its initial compiler
failure and unavailable logs remain historical, with no inferred root cause.
No duplicate hosted retry, merge, GPU, live database or browser work was performed.

All four owner configurations report these single/four-selection setup changes
relative to the retained #294 evidence (not rerun):

| Measurement | Single selection | Four selections |
|---|---|---|
| Setup work | 104737 -> 581709 | 287803 -> 1287811 |
| Setup refusal boundaries | 3592 -> 7402 | 10115 -> 22443 |
| Binding-phase refusal boundaries | 3810 | 12328 |

Higher totals are expected because setup now admits previously unrestricted
binding work; lowering them is not an acceptance requirement. Selected allocation
work remains 420871/790383; selected live/peak remains 321185/321185 and
392475/392475. Byte/work allocation boundaries remain 2759/3271 and 3706/5927.
All 3272/5928 independent failure sites reclaim every block. Retained/peak stays
321265/321265 and 392555/392555, capacity census 321153/392251, failure peaks
321250/392540. Overlapping requested-resource reports must not be summed.

Both native/warm builds and all 47 scoped CPU checks passed at the tested pin:
20 exchange processes (five cases, workers 1/2, bulk/direct), 12 prepared CTests,
nine supporting native handoff/snapshot tests and six warm CTests. No build or
test warnings/errors. Shared-host Clang15/libstdc++, CPU/x64, Taichi
`5ba579a8a11255b7a258cde93801070956d81d57`; accelerator backends disabled.
Object layouts remain unchanged: study/structural input 288/280, projection 1304,
UUID/ObjectKey/ToolRef 48/88/136, Tool/Parent/Geometry/Bundle 432/168/152/520,
UUID payload 37 on this toolchain. No performance or GPU qualification is claimed.

Verified archive `setup-evidence/binding-setup-budget-domain-803437be.tar.gz`;
SHA256SUMS digest:

    0edbcc0fe70d37e1305462ff9d1c70eb40dce4ea21d41e1e1e0ab1384c03b18c

Raw machine-local artifacts have not been independently inspected remotely.
Final tested-to-head changes are four Markdown records only. Hosted checks and
review remain required. Next: finish the setup/resource inventory and combined
caller/owner/borrowed accounting, then remaining operation/control integration.
Domain/control-block overhead and combined resource acceptance are not claimed
complete by this binding gate. The projection copy audit remains closed.

### Initial map copy-work component qualification (2026-11-12)

Tested source `763b9b6e8574efd1b91efc3c5a6d910a1b9d29eb`, continuing from #296
`c43de24155d9852f9d202abe3360cec739e1c014` under review 5877302279. Current main
`a67255c6` integrates reviewed #290-#294 and is included by normal merge
`9acdd5fb`. Two status-document conflicts retain current maintainer integration
and historical contribution records. #295/#296 remain unmerged at inspection.

The input copy-work gate now charges P+M_occurrences+M_fields, with checked fresh
map demand M=1+n*(3*sizeof(T)+74)+K. It does not form an unchecked 3*sizeof(T),
subtract unsigned M-S or charge the old snapshot a second time. The byte gate
still uses C+S. Result fields expose each work component and the required sum,
published only after successful inspection; byte refusal still precedes one
copy-work charge. Initial maps omit the setup meter because their work is prepaid.

The existing fixed initial/membership admissions explicitly cover bounded
header/scalar/control initialization, with bounded checked metadata arithmetic in
inspection charges. These logical units do not replace payload work, selected
request quotas or opaque allocator/driver accounting. Domain bootstrap request
tracking and a trusted disjoint borrowed-resource ledger remain next.

Extended owner input controls compare fresh-map demand with actual metered map
construction for empty and unequal occurrence/field counts with different nested
payload sizes. They require zero selected live bytes after each measured map,
zero/one-short/exact work admission without C++ allocation, intact caller/native
state and successful actual-owner retry. Empty maps preserve the two header byte
charges while replacing their work with one unit each. Existing capacity-only
slack, populated payload and recipe text controls remain; path growth must affect
only the corresponding map work component. Checked row/UUID products and sums
exercise overflow, and an exact UINT64_MAX sum is accepted by the arithmetic helper.
Inspection refusal must leave all four work-breakdown fields unpublished.

Initial code `a4e53a5b` built successfully. Before runtime qualification, inspection
found an overflow-test input with zero row extent, which is not a valid sizeof(T)
for this helper. Only that test input changed to one at the final pin; no product
failure was observed or relabeled. Final source, builds and test results are
recorded below. The earlier build log remains locally preserved.

GCC 11.5 and GCC 13.3 each pass all three standalone projection CTests after the
shared admission header changed. This checks shared-header/compiler compatibility;
map adoption behavior is exercised by the native owner controls, not claimed from
the standalone suites. Verified `setup-evidence/map-copy-work-standalone-763b9b6e.tar.gz`;
SHA256SUMS:

    79feefc61db9cf4a6d7a686a24ae33671cc83cf2b02fe85a5a6491cbae0e6ad7

All four owner configurations agree on this new input breakdown (single/four
selection fixtures):

| Input measurement | Single selection | Four selections |
|---|---:|---:|
| Caller bytes C | 34107 | 56164 |
| Snapshot bytes S | 33083 | 50700 |
| Required input bytes C+S | 67190 | 106864 |
| Projection copy work P | 32362 | 48130 |
| Occurrence-map copy work | 435 | 2637 |
| Field-map copy work | 1348 | 4042 |
| Required copy work (old -> new) | 33083 -> 34145 | 50700 -> 54809 |

Inspection work/boundaries remain 1905/1161 and 2923/1713. Separate owner setup
work/boundaries remain 581709/7402 and 1287811/22443, including 3810/12328 binding
boundaries. The map change is prepaid in input copy work, not duplicated in the setup meter.
Selected allocation work remains 420871/790383 and live/peak 321185/321185 and
392475/392475. All 3272/5928 independent failure sites reclaim every block;
retained/peak remains 321265/321265 and 392555/392555, census 321153/392251 and
failure peaks 321250/392540. These overlapping counters are not summed.

Both final native/warm builds and all 47 scoped CPU checks passed: 20 exchange
processes (five cases, workers 1/2, bulk/direct), 12 prepared CTests, nine native
supporting tests and six warm CTests. The six GCC standalone CTests above are
additional. No final build/test warnings or errors. Shared-host Clang15/libstdc++,
CPU/x64, Taichi `5ba579a8a11255b7a258cde93801070956d81d57`; accelerator backends
disabled. Retained projection/value layouts are unchanged; AdoptionInputResult
adds four scalar work counters, not new owner heap backing.

Verified final archive `setup-evidence/map-copy-work-domain-763b9b6e.tar.gz`;
SHA256SUMS:

    d5fc37a945ca6fd22106667de361852663d9465c0327f291f9508f9c47384168

Final tested-to-head changes are five Markdown records only. Local documentation
file targets and whitespace checks pass. Raw machine-local artifacts have not
been independently inspected remotely. No GPU, live database, browser or timing
qualification. #296 smoke 43249326530 passed; native 43249326537 remains running.
No duplicate hosted retry or merge was initiated. Hosted checks and review remain
required for this contribution. Next: domain bootstrap request tracking and the
trusted disjoint borrowed ledger, then bounded combined accounted-byte admission.

## Self-accounted bootstrap tracker prerequisite — 2026-11-12

Continues #297 review 5878815959 from integrated main
`f20acbb844d28d4d6721cb11248d057f44750af2`. Tested source:
`5d753d867798516493f092f5b37ff6e8633c9ba5`. Subsequent changes in this contribution
are documentation only. [Shared support](../../../common/README.md#domain-bootstrap-tracker-awaiting-review)
defines the lifetime/accounting contract and remaining integration boundary.

The new helper prepays its own outer tracker request and uses an allocator rebound
by `allocate_shared` to measure the actual domain/control-block request. Intrusive
handles prevent recursive unaccounted tracker control blocks. Bootstrap H and
selected D are separate ledgers. `ExchangeOwner::adopt` is unchanged and does not
use this helper yet; no combined owner-resource enforcement is claimed.

Local Ubuntu/WSL, x64 CPU-only, shared personal host:

- GCC 11, GCC 13 and Clang 15 standalone builds passed without warnings/errors.
- Four CTests passed for each compiler (12 total): letter projection, membership,
  structural recipe and the new bootstrap reservation controls. New test compiles
  with `-Wall -Wextra -Werror` and is included by the existing hosted standalone job.
- Independently observed C++ allocation requests equal tracker reports: outer
  tracker 72 bytes, outer plus domain/control block 160 bytes on these toolchains.
  These are measured layouts, not portable fixed budget constants.
- Zero/one-short/exact byte and work budgets cover both allocation boundaries;
  refusal performs zero underlying allocation attempts. Fresh exact retry succeeds.
- Domain weak references retain the charged control block; selected shared/weak
  objects retain domain and tracker after external handles disappear. Final weak
  release leaves zero blocks/live bytes, without a lifetime cycle.
- Each of the two actual bootstrap allocation failures is injected independently;
  acquired blocks unwind and prepaid request work is retained. Constructor failure,
  aligned rebound allocation, checked product/sum overflow and handle self-move
  controls pass. Selected and bootstrap observed request sums are disjoint.

Verified local archive `setup-evidence/bootstrap-reservations-5d753d86.tar.gz`;
SHA256SUMS manifest digest:

    3e706d77983b6fe37464a7eb3eb5f54d99205c303f254af2eea8aae7c7bbab9b

The archive retains commands, source pin, compiler versions, build/test logs and
binary hashes. This is contributor-local evidence, not independently inspected
remote evidence. No source build or test failure occurred. An inspection-only WSL
shell quoting error after successful qualification was corrected; it did not rerun
or alter tests. No native model/CPU matrix, GPU, PostgreSQL, browser or performance
qualification was run. The already accepted #297 baseline was not repeated.

Next: integrate the helper at the owner adoption boundary and add native refusal,
unchanged-state/retry witnesses, then the trusted disjoint borrowed-resource ledger
and combined accounted-byte policy. H does not cover allocator metadata, physical
residency, process allocations or borrowed runtime backing.

## Owner bootstrap admission — 2026-11-12 (awaiting review)

Continues #298 review 5879152987 on `386be0a03a1de22092d848f711c4264bb46995d5`.
Main remains `f20acbb8`, integrated through #297. #298's smoke/warm runs passed;
its native run 43268613665 remains active at this contribution's source review.
No duplicate hosted run or merge was initiated. Tested source:
`f1cabfd5d9aa44d4d73ef8e4fefad2c93025caf9`; subsequent changes are documentation.

Owner adoption now uses the bootstrap tracker and reserve_domain factory with a
separate budget/result. It keeps H separate from selected D. Input/setup gates
precede bootstrap; byte/work refusal at either bootstrap boundary precedes selected
allocation. Later setup/selected refusals and unexpected exceptions unwind both
surfaces, retain prepaid work and report zero live bootstrap bytes. Successful
owners and selected allocator lifetimes retain the tracker through final weak
release. [The contract](../../docs/ADOPTION-ACCOUNTING.md#owner-bootstrap-integration-awaiting-review)
records the census overlap and remaining borrowed/combined policy boundary.

New native owner controls exercise zero/one-short/exact bootstrap byte/work limits,
both allocation failures plus the first owner request failure, initial setup and
later constructor/setup/selected refusals, independent H+D observation, unchanged
native snapshots and exact retry. Weak catalogue release after owner/domain/tracker
handles disappear reclaims all blocks. Existing complete selected-reservation,
setup and every-allocation-site failure sweeps remain enabled.

Measured single/four-selection results, identical across one/two CPU workers and
bulk/direct-no-unroll modes:

| Counter | Single | Four |
|---|---:|---:|
| Bootstrap outer tracker request | 72 | 72 |
| Bootstrap live/peak/prepaid work H | 160 | 160 |
| Selected live/peak D | 321193 | 392483 |
| Selected cumulative allocation work | 420879 | 790391 |
| Observed retained/peak owner requests | 321353 | 392643 |
| Retained-capacity census | 321161 | 392259 |
| Independent allocation failure sites | 3273 | 5929 |
| Failure-sweep peak requests | 321338 | 392628 |

The owner tracker handle adds eight selected object bytes on this toolchain. H
replaces the formerly excluded 80-byte domain request with measured 72+88 bytes;
observed retained/peak totals increase by 88 and allocation sites by one. These
are layout measurements, not hardcoded budget fees. C+S and P+M_occurrences+M_fields
remain unchanged (67190/106864 bytes and 34145/54809 copy work). Counters overlap
and must not be summed into a physical-memory claim.

The initial build at `6576d4de` passed; the final code adds explicit later-refusal
witnesses before runtime qualification. Local commands/logs are retained under
owner-bootstrap-domain-f1cabfd5. Full qualification and archive verification are recorded below. CPU-only Clang15/libstdc++ on shared Ubuntu/WSL x64;
no GPU/database/browser or performance qualification. No standalone helper baseline
rerun is required: that source is unchanged from reviewed #298. Its prior GCC11/
GCC13/Clang15 evidence does not imply a new GCC qualification of owner integration.

Next: establish the trusted disjoint borrowed-resource ledger, then enforce the
combined accounted-byte policy and continue remaining operation/control integration.
The projection-copy audit stays closed. No complete owner-resource acceptance is
claimed by this scoped change.

Final qualification: native and warm builds passed; all 47 scoped CPU checks
passed (20 exchange processes, 12 prepared CTests, nine supporting native CTests,
six warm CTests). No build/test warnings or errors. All owner measurements above
were compared across the four configurations and match. The tracked Taichi tree
is `5ba579a8a11255b7a258cde93801070956d81d57`, not a separate submodule commit;
its source is clean. Runtime/binary hashes and configuration are in the archive.

Verified local archive `setup-evidence/owner-bootstrap-domain-f1cabfd5.tar.gz`;
SHA256SUMS manifest:

    6f0919f6ea624d557745cc73eb9711b14f6af19f21089da42a57d396b51d5726

Raw local evidence remains contributor-reported, not independently inspected
remotely. Final changes after the tested code are six Markdown files only.
Documentation file links resolve and whitespace validation passes. #298's native
run remains active at final publication; the new PR also awaits hosted review.

## Native allocation-record contract and CPU implementation — 2026-11-12

Continues #300 review 5879635043, which accepts the bounded CPU-first slice and
explicit Unavailable behavior elsewhere. Current main `9025c301` includes accepted
#298; normal merge `2a919e22` reconciles its two handoff documents into the reviewed
#299/#300 stack. Tested code is `48236f411b46eda54bf1f23b50d30e4ca3fc1a0c`;
tracked Taichi tree is `e74df27a42357efc43bc9f08d3b8048f91b4cf29`.

Common Device now supplies allocation record/status/ownership, value identity and
retained metadata queries, with CPU implementation and default Unavailable on
other backends. Tokens contain no Device pointer or lifetime lease; inspection
requires a known-live native owner. CPU reset epochs and process-local device
instances reject stale IDs and same-address reconstructed devices. Imports report
registration extent but no owned request, and release never frees imported backing.
Zero allocations have explicit live/retired state. Allocation metadata registration
failure clears the unpublished handle and unwinds acquired payload. The new native
CTest is included automatically by the hosted round-robin test selection.

The common Device layout/vtable changed: native static libraries and the dependent
engine were rebuilt together. The first command mistakenly selected the object-only
`taichi_core` target. It was intentionally interrupted before engine linking/tests;
its log and command are retained. Corrected `taichi_core_static` rebuilt the archive
and dependencies, then the engine rebuilt successfully. No source build/test failure
occurred. Source `2068ad7c` contains the final runtime changes; later code commits
add registration-failure/import-alias/invalid-token test controls. Final qualification
runs those controls at the tested pin above.

The five focused CPU CTests passed: allocation_record_test, engine_smoke_test and
record_map_padded/direct/working_set. The new test covers owned direct/runtime
requests, view aliases, distinct equal-size allocations, foreign handles/tokens,
zero live/retired requests, import ownership and alias release, stale reset IDs,
invalid token extents, same-address device reconstruction, base Unavailable
fallback, allocation-free inspection, metadata storage and registration failure/retry.
Measured test metadata: CPU device object 96 bytes; retained record table 192 bytes
for its fixture capacity. These are accounted layout measurements, not physical
residency or portable fixed fees. No new per-token heap or allocation registry is
introduced. The closed projection-copy audit and existing H/D/input work gates
remain unchanged.

Broader CPU qualification passed: 20 exchange processes (five cases across threads 1/2 and bulk/direct-no-unroll), 12 prepared execution/update tests, nine supporting native tests and six warm recipe tests. Together with the five focused checks, all 52 scoped CPU checks passed. Completed build and matrix logs contain no warnings/errors; source remained clean at the tested pin. No GPU,
PostgreSQL, browser or performance qualification is implied. This does not implement
all borrowed backing exports, trusted Harness coverage/lifetime, or combined B
admission; those remain the next work after this native prerequisite.

Verified contributor-local archive: native-record-domain-48236f41.tar.gz.
SHA256SUMS manifest: 34889eecc83fda5c15c173c5b2c042ab4d6bb175569a59c37e476bd8323944cd.
Archive SHA256: 6c056f9d04a1faec386c98e96588b8df165e7ea659de8e01c6429ca5c482460d.
Includes source/configuration/binary pins, commands, full corrected native/engine
build logs, preserved interrupted-target history and all scoped result logs.
Raw local artifacts have not been independently inspected remotely. The final
publication changes only five Markdown files after the tested code pin.

## Array/root native backing exports — 2026-11-12

Continues #301 review 5880096613 on reviewed 63a46aab. Current main b2eb86c7
integrates #299; normal merge ca24735f reconciles it into this stack. #300/#301
remain source-accepted but unmerged while hosted native qualification continues.

The common Ndarray export captures allocation identity at construction and inspects
through a supplied known-live Device. Native LLVM tree roots capture their records
at registration and expose them through common ProgramImpl. Equal-size allocations
remain distinct, partial views report complete allocation extents, and imports remain
unresolved registrations. Unknown/Retired/Unavailable are not accepted zero backing.
Root record payload is reported once per runtime; embedded array records belong to
the array object census. See the owning BORROWED-RESOURCES coverage table: prepared
invocation and node/list families, complete host metadata and trusted Harness
coverage/lifetime remain outstanding. No complete B or combined gate is implemented.

Initial source 398e2238 completed the native static library build. Its intermediate
engine build was intentionally interrupted before qualification to use the final
metadata additions, common-program-accessor test correction and constructor cleanup.
Both command/source/build histories are retained. The intermediate build is not a
pass and the uncompiled accessor typo is not described as a successful test.

Final source and scoped qualification results are recorded below. CPU/shared-host
scope only: no GPU, database, browser or performance qualification is implied.

Tested source: 6a741e4f6fc24e0f703825ba1d796bc48614ff83.
Tracked Taichi tree: 0a23a7a8b587b2105abbf9d67f5435ad1320224a.
The final native static library and engine rebuilt together successfully. All six
focused CTests passed: owner_backing_test, allocation_record_test, engine_smoke_test
and record_map_padded/direct/working_set. New owner controls cover partial views,
foreign devices, unresolved imported aliases, independent import retirement,
release/reset/reuse, owned-array destruction, distinct equal-sized roots,
missing/destroyed roots, default unavailable coverage, allocation-free inspection
and root metadata removal. CPU fixture observations: logical array request 28 bytes,
root device request 4096 bytes after native page rounding, Ndarray object 224 bytes.
These are scoped request/layout measurements, not physical residency or portable fees.

The broader CPU matrix passed: 20 exchange processes (five cases across threads 1/2
and bulk/direct-no-unroll), 12 prepared execution/update tests, nine supporting
native tests and six warm recipe tests. Including the focused six, all 53 scoped
checks passed. Tested source stayed clean. Completed builds had no compiler
warnings/errors. One passing 2-direct-exchange_commit process reported a shared
Taichi offline-cache lock warning while tests ran in parallel. The first archive
verifier rejected that diagnostic; the final verifier requires and preserves that
exact sole warning. No cache cleanup or test rerun was performed, and no source
build/test failure occurred. Both verifier versions and the diagnostic are retained.

Verified contributor-local archive: owner-backing-domain-6a741e4f.tar.gz.
SHA256SUMS manifest: f9e058236bafcbc5cad8a4b9b54d24d3244c2483715c776318cd830582f9b57d.
Archive SHA256: 68a576162397c51e7ca58c225870e5c9f25c60304e0a4da18b96b32b6b31ec8a.
Includes source/configuration/compiler/binary pins, commands, complete final builds,
intermediate build history and all scoped result logs. Raw local artifacts have not
been independently inspected remotely. Final publication changes only four Markdown
files after the tested source. At publication #300 native 43276619607 and smoke
passed; #301 smoke passed and native 43282494612 remains active. No acceptance or
merge is inferred from those statuses.

## Prepared CPU backing exports — 2026-11-12

Continues #302 review 5880581671 at reviewed aa48a2e4. Main 0e82ebe0 integrates the
accepted #300 audit; normal merge f0e1e4b8 reconciles its handoff notes. Two Markdown
conflicts were resolved preserving the maintainer disposition and scoped contribution
history. #301/#302 remain provisionally source-accepted and unmerged.

Common launch-context queries expose actual CPU prepared argument/result/owned
RuntimeContext buffers, captured array records and separate family coverage. Array
records originate from Ndarray at binding time rather than recapturing numeric IDs
at inspection. External/raw rebinding clears the record; imports remain unresolved.
The records are embedded in existing ArrayBinding rows, so the existing sizeof-based
capacity accounting includes them. Incremental record bytes are a subset, not an
additional charge. Full host metadata, other backend launch buffers and native
node/list/ambient reservations remain incomplete/unavailable. No complete B ledger
or combined admission is introduced. Returned views require a live quiescent native
owner and do not extend lifetime across destruction or address reuse.

Initial d906a73e failed native compilation because the new capture used CallableBase
instead of the originating Kernel to access Program. Correction a380d5be uses the
originating Kernel/Program, leaving originless contexts unavailable. The original
source, commands and compiler diagnostic are retained; that failed build is not
qualification. Corrected code also keeps exhausted device identity unavailable.

Final tested source: a380d5be40468ec45962bea1989212722d145ff4.
Tracked Taichi tree: 561a2dec0dbbd08f07354f3d1718de6cdd173b83.
Qualification results and verified archive follow below. CPU/shared-host scope only;
no GPU, database, browser or performance qualification is implied.

Corrected native static and dependent engine builds passed. All eight focused CTests
passed: prepared_backing_test, prepared_launch_test, owner_backing_test,
allocation_record_test, engine_smoke_test and record_map_padded/direct/working_set.
The new test covers full backing through alias views, distinct coverage flags,
metadata capacities, foreign/scalar/out-of-range/default-unavailable queries,
allocation-free inspection, prepared execution before/after context move, external
rebinding, unresolved imports and independent import retirement, array destruction
and unusable state. CPU fixture: argument request 40 bytes, result request zero with
available coverage, incremental embedded record capacity 144 bytes (a subset of full
binding-row capacity). No per-query allocation was observed. These are request/capacity
measurements, not physical memory or universal fixed fees.

The broader matrix passed: 20 exchange processes (five cases across threads 1/2 and
bulk/direct-no-unroll), 12 prepared execution/update tests, nine supporting native
tests and six warm recipe tests. Including the eight focused tests, all 55 scoped CPU
checks passed. Corrected build and captured final runtime logs contain no warnings
or errors; the tested source remained clean. The original compiler failure remains
in the history directory rather than being relabeled as qualification.

Verified contributor-local archive: prepared-backing-domain-a380d5be.tar.gz.
SHA256SUMS manifest: c6d2344669bd6d5caf924fc1880fc3d886a086ce6a21c2fd17b2969d6ba29eb8.
Archive SHA256: 7b9f2c561e81e8069d374662745f83accb0e0147dd8aa7c4eca0ad2d2e4f4324.
Includes exact source/configuration/compiler/binary pins, commands, corrected builds,
failed initial build history and all scoped results. Raw local artifacts have not
been independently inspected remotely. Final publication differs from the tested
pin only in four Markdown files. Documentation whitespace and 116 local file links
passed. Parent hosted qualification was not duplicated; no acceptance or merge is
inferred from local results.

## Native node/list/ambient request exports — 2026-11-12

Continues #303 review 5880964608 at reviewed bb0dfd28. Main e11525a0 integrates
accepted #301; normal merge b86533e5 reconciles the handoff. #302/#303 remain
provisionally accepted and unmerged. No acceptance is inferred from CI.

Common ProgramImpl queries expose bounded CPU native request views for NodeManager,
ambient storage, list objects, allocated directory pages and allocated chunks.
Untouched slots are Absent; invalid selectors are Invalid; unsupported owner/backend
states are Unavailable. Pool backing identity and rounded extents remain explicitly
unresolved. These are current reachable requests, not allocation-history records,
Device tokens, lifetime leases or a complete B ledger. Query through a known-live,
quiescent owner with its pool intact. Tree removal retains these native requests even
though its root allocation is released. See [the ownership contract](../../docs/BORROWED-RESOURCES.md).

The ambient request-size table is retained and exported once per runtime. Its pointer
and the cached query callback are embedded metadata subsets, not extra charges when
whole owner objects are counted. Queries have fixed depth and allocate nothing;
future Harness enumeration/validation must still be prepaid and metered. Imported
alias resolution, underlying pool identity, remaining metadata and combined C+B+H+D
admission remain outstanding.

Tested source: 3832ed917b603ef938959c6bad93db7212231dd1.
Tracked Taichi tree: 62b11a6811ec7e9fe51991cdff28a2499a4ad0fe.
Full native static and engine builds passed. Nine focused tests passed, including
reservation_request_test, prepared_backing_test, prepared_launch_test,
owner_backing_test, allocation_record_test, engine_smoke_test and three record-map
variants. The new fixture verifies statuses, selector bounds including UINT64_MAX,
all four list roles, distinct addresses, first/repeated allocation-free queries,
actual activation allocation deltas, stable repeated activation, retained requests
after tree destruction and distinct runtime instances without querying dead owners.
Fixture requests: ambient32, page4096, chunk524288, metadata1024 bytes. These are
allocator requests on this fixture, not rounded/physical totals or universal fees.
CPU/shared-host scope only; no GPU, database, browser or performance qualification.

The broader matrix passed: 20 exchange processes (five cases across threads 1/2 and
bulk/direct-no-unroll), 12 prepared execution/update tests, nine supporting native
tests and six warm recipe tests. Including nine focused tests, all 56 scoped CPU
checks passed. Full final build and captured runtime logs contain no warnings/errors;
the tested source remained clean. There were no source build/test failures in this run.

Verified contributor-local archive: native-reservation-domain-3832ed91.tar.gz.
SHA256SUMS manifest: fd700d864c8c13316a02a17ccd70552df2a7e9ca3e3d0830a16fad7481134507.
Archive SHA256: ae8fd2e9c93b99a4ce265223fe6df853f47029c3fd45c57d44cbc978be41d936.
Includes source/configuration/compiler/binary pins, commands, complete builds and
all scoped results. Raw artifacts have not been independently inspected remotely.
Final publication differs from the tested pin only in four Markdown files. All 117
local file links and whitespace checks passed. Parent #303 smoke 43292870875 passed;
native 43292871002 remains active. No duplicate hosted run was requested and no
acceptance or merge is inferred from these local results.

### #304 review correction: explicit CTest runtime environment

Review5881347735 identifies inherited configuration defects in #302/#303/#304.
Hosted #302 native43287680849/job131433550369 failed owner_backing_test at
runtime_lib_dir@43 before its assertions; the other28 shard tests passed (maintainer
inspection). That failed attempt remains preserved and #302 is not green. Earlier
local qualification sourced engine-env.sh, masking the absent CTest property.

Corrected source e0cc9e72b1be9640fbb1becfc580744b093aa08d changes only CMake test
properties from reviewed48d1a2b5. All three Runtime-using tests now share the existing
TI_LIB_DIR environment and TIMEOUT900 group. Native tree remains
62b11a6811ec7e9fe51991cdff28a2499a4ad0fe; no native implementation changes.
CMake regenerated successfully; engine build reported no work needed. With the
parent TI_LIB_DIR unset, verbose CTest confirms the explicit runtime path and
900-second timeout for owner_backing_test, prepared_backing_test and
reservation_request_test:3/3passed. The combined nine-test backing/reservation
subset also passed with the variable unset. These nine include the three, so this
is nine distinct tests, not12. Tested source stayed clean. CPU only, no GPU.
The earlier56-check result remains scoped to its original source/environment;
this focused correction does not relabel it as clean-parent-shell qualification.

Evidence: runtime-ctest-fix-e0cc9e72.tar.gz, contributor-local, not independently
inspected. Manifest SHA256 da0734563c111a389c182e272a4dd0df606d85a90442758e80aa3a5a832757d4.
Archive SHA256 22ea2c02df67f12adeddc7b083019c0b6f8a15371c758d55e305c82f5381b56f.
Contains commands, source/binary pins, generated CTest properties, build and verbose
results. The final archive checksum command initially included a trailing CR and
failed after successful tests/archive creation; a separate corrected checksum read
succeeded without rerunning tests or changing evidence. No test failure occurred.
Correction is carried at combined #304; unchanged parent runs are not retriggered.
Hosted corrected qualification and review remain pending. Final documentation-only
delta does not change tested code or configuration.

## Explicit owned-import provenance — 2026-11-12

Continues #304 review5881459672 at d40daac0: source/configuration blocker corrected,
provisional source acceptance restored, integration still pending corrected combined
hosted qualification. Main remains e11525a0 through #301 and is already an ancestor.
The failed #302 run remains failed; unchanged parents are not rerun.

Common Device import_owned_memory validates a same-device live Owned token and
in-bounds range before capturing that owner in the imported registration. The separate
allocation_backing_record query returns the live canonical owned token/full extent.
Raw pointer imports remain unresolved, with no late address scan or pointer matching.
Registration identity/liveness and backing identity/liveness remain separate.
Unknown/default-unavailable paths cannot supply usable backing sizes. Existing
map/transfer/launch APIs do not gain a lifetime lease or automatic use-after-owner
protection; callers retain the live/quiescent owner obligation. See the scoped
[ownership contract](../../docs/BORROWED-RESOURCES.md).

The additional owner token is embedded in every existing CPU AllocInfo row, and the
existing retained-capacity times sizeof(AllocInfo) report includes it once per device.
No second registry or extra additive token charge. Query depth is bounded by two
indexed validations; future input-sized enumeration still needs work accounting.
This slice does not implement pool identity/rounded extents, raw-import resolution,
prepared-binding adoption, remaining metadata or combined admission.

Tested implementation source: d8c752ccb9407e8f2daf4e595aba2b7afc12f694.
Tracked Taichi tree: 89ba8451f7446a2dc6a5175c03d50c05a517e25e.
The extended allocation_record_test covers partial views/full owner backing,
separate registration identity, exact mapped offset, allocation-free first query,
raw matching-pointer refusal, invalid/overflow/foreign/nested/null-output inputs,
default backend, zero/end views, independent import retirement, owner retirement,
replacement/reset non-revival and failed registration with exact retry. Existing
allocation-record coverage still exercises same-address device reconstruction.
CPU/shared-host scope only; no GPU, database, browser or performance qualification.

Full native static and dependent engine rebuilds passed without compiler warnings
or errors. All56 scopedCPU checks passed:9focused,20exchange (five cases across
threads1/2 and bulk/direct-no-unroll),12prepared execution/update,9supporting native,
6warm recipe. Focused/prepared/supporting CTests ran with parent TI_LIB_DIR unset;
direct exchange executables used the explicit engine environment. The standalone
allocation-record fixture reports device_object_bytes96 and record_capacity_bytes384;
these are fixture metadata sizes, not portable constants or physical memory totals.
Tested source stayed clean. No source build/test failure occurred.

Two passing exchange_commit cases (1-bulk and2-direct) emitted offline-cache lock
warnings during concurrent runs. Both exact diagnostics remain in runtime-warnings.txt
and the original logs. No cache cleanup or test rerun was performed. Build logs have
no warnings/errors; runtime logs are not claimed warning-free.

Verified contributor-local archive: owned-import-domain-d8c752cc.tar.gz.
SHA256SUMS manifest: f1bda6862c6c7931d08255ee8267ca786d3c2bd696c0cf3ef9ee5f273a18a7be.
Archive SHA256: 18224edddb59895bfbb1b4762e3e69a6d5e98b8e2f67bc6d503562da632947cc.
Includes source/configuration/compiler/binary pins, exact commands, builds, scoped
results and retained diagnostics. Raw artifacts have not been independently inspected
remotely. Published head differs from tested source only in four Markdown files;
118 local file links and whitespace checks passed. Parent #304 smoke43299289118
passed; native43299289153 remains active at inspection. No duplicate qualification
or self-merge. Local results do not replace authoritative hosted CI or acceptance.

## Prepared owned-import backing adoption — 2026-11-12

Continues #305 review5881732238 at44c890b7. Provisional source acceptance is recorded;
#302–#305 integration remains pending corrected parent/hosted qualification. Main
e11525a0 is already an ancestor. No duplicate hosted run or unchanged-parent rerun.

The new prepared_array_backing_record preserves the separate registration query and
resolves canonical live Owned backing through common Device. Array coverage now
accepts explicit owned imports only while both registration and owner validate.
Raw imports remain unresolved. Bound elements must fit the registration extent before
resolving full owner backing; a larger owner cannot certify an oversized import view.
This is accounting coverage, not an added execution guard or lifetime lease. Existing
prepare type/rank/shape checks remain unchanged. No new retained metadata is added;
summary scans remain input-sized work to meter before eventual combined admission.
See [the scoped ownership contract](../../docs/BORROWED-RESOURCES.md).

Initial implementation11f4cf86 began native rebuild; it was deliberately stopped to
add the registration-extent boundary and negative test. The source pin, commands and
interrupted build log are retained as history, not reported as qualification.
Corrected tested source:684a1687ce5dffbf11592f063c9ed5f52f376ce8.
Tracked Taichi tree:15d7d5bda369ad8faa73bbc8b23fb9b3aa2ff815.

Extended prepared_backing_test covers canonical full owner vs Imported view identity,
nonzero-offset actual execution, context move/execution, allocation-free first queries,
oversized-view refusal without launch, foreign/scalar/out-of-range inputs, raw/external
refusal, independent import retirement, and a Live registration whose backing owner
has retired. Existing tests continue to exercise owned views, context buffers, metadata
capacity, unusable/default states and captured array destruction. CPU/shared-host only;
no GPU, database, browser or performance qualification is implied.

Corrected native static and dependent engine builds passed. All56 scopedCPU checks
passed:9focused,20exchange (five cases across threads1/2 and bulk/direct-no-unroll),
12prepared execution/update,9supporting native,6warm recipe. Focused/prepared/supporting
CTests ran with parent TI_LIB_DIR unset; direct executables used the explicit runtime
environment. Corrected build and captured final runtime logs contain no warnings or
errors. Tested source remained clean. The interrupted intermediate build is retained
without a pass claim; no source compiler/test failure occurred in final qualification.
Prepared fixture requests remain arguments40/result0 bytes and embedded record
capacity144 bytes, unchanged by this slice and not portable fixed fees.

Verified contributor-local archive:prepared-import-domain-684a1687.tar.gz.
SHA256SUMS manifest:391688cfa350a074473b5ff6bab1665f555f22d57363fa90b2a685e9a2bada81.
Archive SHA256:49b59c0348e35e04ea40cc79ff2b1f1fa465f452a4eb97ab7e8fc448b3e53138.
Includes source/configuration/compiler/binary pins, commands, corrected build/results,
and interrupted-source history. Raw artifacts are not independently inspected remotely.
Final publication differs from the tested source only in four Markdown files;119local
file links and whitespace checks passed. Parent #305 smoke43302942605 passed;
native43302942903 remains active at inspection. No duplicate hosted qualification,
self-merge or inference of final acceptance from local results.

## Native host-pool backing identity — 2026-11-12

Continues #306 review5881942652 at52696f12. Provisional source acceptance recorded;
stack integration awaits hosted qualification. Main e11525a0 is already an ancestor.
No unchanged-parent rerun, duplicate hosted qualification or self-merge.

Native raw-allocation records now carry nonwrapping pool/allocation identities and
exact raw request sizes. Token validation rejects release/reset/address reuse without
dereferencing saved pointers; allocator vector indexes are not used as identity.
Range containment uses an ordered integer-address map and subtraction bounds. Common
Device/Program bridges first validate supported live owner/request provenance. Raw
imports remain unresolved; zero owned CPU payload has no raw mapping (Absent).
Rounded extents remain unavailable. Shared raw backing must not be summed with its
contained requests. No full B or physical-memory accounting is claimed.

Metadata object/value payload belongs once to the shared pool. Incremental value
payload is a subset, not extra storage; full pool/node/allocator metadata remains
incomplete. Failed map registration releases its acquired OS mapping. Lookup is
logarithmic, not the request API's fixed-depth unit, and future Harness work metering
must cover lookup and enumeration. See [the ownership contract](../../docs/BORROWED-RESOURCES.md).

Corrected tested source:34432410b75fb53d03190494f0bb0cb35dd9e8f9.
Tracked Taichi tree:bb2ced8560731a914dab4ecb6914d7959a8c4593.
Extended native_pool_test exercises shared/distinct raw identity, exact raw size,
explicit unknown rounding, allocation-free capture/validation/metadata, invalid and
cross-boundary/oversized/null/zero ranges, foreign pool identity, release/reallocation,
allocator swap-erase movement, reset, failed metadata registration and exact retry.
Extended allocation_record_test covers owner/import shared pool identity, raw-import
refusal, zero backing, retired owner and default-unavailable API. Extended
reservation_request_test covers live native metadata requests, allocation-free query,
Absent/invalid/unsupported cases. CPU/shared-host scope only; no GPU, database, browser
or performance qualification is implied. Pool reset is confined to the isolated test
process after its allocations are no longer used; retained databases are untouched.

Initial d5d59ddd native build was deliberately interrupted to move lazy shared-pool
bookkeeping initialization into CPU device construction. Corrected first-device
metadata inspection is allocation-free before any raw backing exists. Initial source,
commands and interrupted log remain in history without a qualification claim.

Corrected native static and dependent engine builds passed. All57 scoped CPU checks
passed:10focused (including native_pool_test),20exchange processes (five cases across
threads1/2 and bulk/direct-no-unroll),12prepared execution/update,9supporting native,
6warm recipe. Focused/prepared/supporting CTests ran with parent TI_LIB_DIR unset;
direct executables used the explicit runtime environment. Corrected build and captured
final runtime logs contain no warnings or errors. Tested source remained clean. No
source compiler/test failure occurred; the interrupted intermediate build remains
unqualified history. No GPU or performance claim is inferred from this shared host.

Verified contributor-local archive:host-pool-domain-34432410.tar.gz.
SHA256SUMS manifest:6ab07a7832ec069cea9786a0c8b62c053d480fb587d269225192782dac7de115.
Archive SHA256:908cce66995a326b929f5b927c167f653a247518cf592f3ef2d3c7580235170e.
Includes source/configuration/compiler/binary pins, exact commands, corrected results
and interrupted-source history. Raw artifacts have not been independently inspected
remotely. Final publication differs from tested source only in four Markdown files;
120 local file links and whitespace checks passed. Parent #306 smoke43305748766
passed; native43305748693 remains active at inspection. Hosted results and review,
not local qualification alone, govern integration; #302's failed attempt is preserved.

## Rounded host-pool extents — 2026-11-12

Continues #307 review5882509096 at88aa2cd0. Fetched main a139c21b, which integrates
qualified #302–#305. Maintainer inspected119 passing regressions and database
addressed_reads24/private_reads0 on #305; the original #302 initialization failure
remains preserved. Normal merge ba723dd0 resolves two handoff-document conflicts by
preserving current maintainer integration plus historical contribution records.
#306/#307 remain source-accepted with native qualification pending; no duplicate run.

Host pool setup captures OS page size; nonzero raw requests are rounded with checked
padding arithmetic before the OS call. Successful mappings retain original and rounded
lengths separately. Live pool records report rounded virtual extent through existing
owner/request bridges. Request containment, allocator tails, Device records and prepared
view bounds remain based on original sizes. Padding is not usable view capacity.
Rounded extents are not physical residency; full B and remaining metadata are not built.
See [the ownership contract and OS references](../../docs/BORROWED-RESOURCES.md).

Tested source:afa366af4202939491330576f09ab93e40a8369a.
Tracked Taichi tree:a106dd6d1f94ccbf8d6c33de6decbf67f1f72ea3.
Native pool tests now independently obtain the host page size and cover1,page-1,page,
page+1 requests, retained/query-time extent agreement, actual final-page access,
padding exclusion from request containment, release invalidation, zero/overflow
rejection before accounting publication, and existing allocation-failure cleanup.
Device and runtime request tests require rounded coverage without changing raw requests
or owned-import identity. Added pool field/map-value length is included once through
existing sizeof-based reports; full metadata remains incomplete. Query work unchanged.
Execution qualification is Linux/WSL CPU/shared-host only. Native Windows source uses
GetSystemInfo/VirtualAlloc semantics but Windows execution remains unqualified; no GPU,
database, browser or performance qualification is implied.

Parent status refresh: #306 native43305748693/smoke43305748766 and #307
native43310337611/smoke43310337168 now report success. These workflow conclusions
were checked without duplicating runs; raw hosted job logs were not independently
re-inspected here. Maintainer integration remains separate from these status checks.

Native static and dependent engine builds passed without compiler warnings/errors.
All57 scoped CPU checks passed:10focused,20exchange processes (five cases across
threads1/2 and bulk/direct-no-unroll),12prepared execution/update,9supporting native,
6warm recipe. Focused/prepared/supporting CTests ran with parent TI_LIB_DIR unset;
direct executables used the explicit runtime environment. The fixture reports
os_page_bytes4096 and four passing rounded-boundary cases. Tested source stayed clean;
no source build/test failure occurred.

One passing 1-bulk-exchange_commit case emitted an offline-cache lock warning during
parallel execution. The exact diagnostic remains in runtime-warnings.txt and its
original log. No cache cleanup or test rerun; runtime logs are not claimed warning-free.

Verified contributor-local archive:rounded-pool-domain-afa366af.tar.gz.
SHA256SUMS manifest:ad830f5ac85a8dbd6fe641e391b98e9000f881fa1510660d6b4cf4a29874ac5c.
Archive SHA256:10db420ff3a757cad8c51292ee0fb7c105939b321b3bc0928e90e7c3f29f4dca.
Includes source/configuration/compiler/binary pins, commands, complete build/results
and preserved diagnostics. Raw artifacts have not been independently inspected remotely.
Final publication differs from tested source only in four Markdown files;121local
file links and whitespace checks passed. No self-merge or duplicate hosted run.

## Native pool container metadata — 2026-11-12

Continues #308 review5883275400 at10b3ed7f. Main e62924fd integrates qualified
#306–#307; normal merge488a17bd resolves two Markdown conflicts preserving maintainer
integration and contribution history. Maintainer inspected119 passing #307 regressions
and database addressed_reads24/private_reads0. #308 remains source-accepted with native
qualification pending. No unchanged-parent rerun or duplicate hosted qualification.

A native rebinding allocator counts successful live raw-map allocation requests,
including implementation node storage; failed requests do not advance the counter.
The counter outlives map/rebound copies and stays under pool synchronization. Map value
payload remains a subset. The report separately exposes UnifiedAllocator object bytes
and retained chunk-vector capacity, once per shared pool identity. Empty-map library
storage is preserved rather than presumed zero. Reset's temporary map copies share
counter lifetime; this is retained-state reporting, not transient-peak prepayment.
Full metadata_complete remains false; global/host-heap overhead and complete runtime/
prepared/Harness coverage are still outside scope. See [the contract](../../docs/BORROWED-RESOURCES.md).

Tested source:9a249a7db484eb260c4bd10ed49740123313ce6e.
Tracked Taichi tree:d24296773894d995459abf44e1b3a72c8c8a4bb1.
Native pool tests compare retained map/vector growth with live C++ allocation bytes,
check actual-node payload inclusion, allocator object size, map release with retained
vector capacity, reset/copy cleanup, failed registration/exact retry and arithmetic
overflow refusal. Existing allocation-free query, identity, rounding and owner/request
bridge tests remain in scope. CPU/Linux/WSL shared-host qualification only; no GPU,
native Windows execution, database, browser or performance qualification is implied.

Initial source4d3d81fa and diagnostic sourcec3adc4d6 both built successfully but
failed native_pool_test (9/10 focused checks passed each): the test incorrectly
equated cumulative C++ allocation bytes267 with retained map64+vector32 bytes.
The correction uses a live-allocation witness, without changing production code:
the final witness observes cumulative261/live96/reported retained96 bytes.
Allocator object24 bytes already existed before that observation. Node payload32
is included in the map64 request, not added again. These are fixture observations,
not portable hard-coded sizes. Both failed runs, pins, commands and logs are retained
under the final archive's history directory and are not relabelled as passes.
Corrected native static/dependent engine builds and all57 scoped CPU checks passed:
10focused,20exchange,12prepared,9supporting native,6warm recipe. Focused/prepared/
supporting CTests ran with parent TI_LIB_DIR unset; direct executables used the explicit
runtime environment. Corrected build and captured runtime logs contain no warnings/
errors. Tested WSL source is clean. Final publication differs only in four Markdown
files;122local file links and whitespace checks passed.

Contributor-local archive:pool-metadata-domain-9a249a7d.tar.gz.
Manifest SHA256:191a37e868e11cb56fc8ffb6828aa580dc4f5d9a579afb92839989434dae2d59.
Archive SHA256:111c48a710e7d388205da6e18effc1b4a9d1ff36ca12eda2aec4fbd0b5870952.
Includes source/configuration/compiler/binary pins, commands, complete results and
both failed observations. Raw artifacts have not been independently inspected remotely.
Hosted qualification and maintainer acceptance remain separate; no self-merge.
## Prepared scalar and binding-key metadata — 2026-11-12

Continues #309 review5883627408 at6b3180b5, source provisionally accepted with no
blocking finding. Main cee73d53 integrates qualified #308; normal merge992d54a0
preserves maintainer handoff and contribution history in two Markdown conflicts.
Maintainer inspected #308's119 passing regressions and database131549865939
addressed_reads24/private_reads0. #309's hosted native qualification remains separate.
No duplicate parent CI; historical failures remain preserved.

PreparedBacking exposes scalar vector capacity separately from its containing object.
A separate budgeted query enumerates four binding maps' owned key capacities and value
payload, with entry/bucket counts. Required entries are checked before traversal;
insufficient budget visits none. Exact retry reads capacities without allocation,
hashing or following pointers. Node/bucket request bytes remain unmeasured, and both
metadata completeness flags remain false. The native owner/lifetime guard is shared
with existing prepared-buffer inspection. See [the contract](../../docs/BORROWED-RESOURCES.md).

Tested source:83c1d74d36a0edfe2a33ba8f07b7d0999edcbf33.
Tracked Taichi tree:9a1d7b6da2896ce585fa1bcfd0a6107a84380598.
Expanded prepared_backing_test checks spare scalar capacity, zero/one-short/exact
entry budgets, allocation-free refusal/retry, map family counts, a moved-in key with
spare capacity, cleared key/value storage without a completeness claim, foreign
device refusal and moved-owner behavior. Existing execution/import/retirement tests
remain in scope. No retained context state, binding reserve or launch behavior changes.
CPU/Linux/WSL shared host only; no GPU/native Windows/database/browser/performance claim.
Initial sourcee6cdc209 built successfully but prepared_backing_test failed, with9/10
focused tests passing. The fixture expected only two allocation-type entries; native
set_arg also registers the scalar argument as kNone, so the correct count is three.
Test-only correction83c1d74d preserves production code and emits observed family counts.
The failed source, build/test commands and logs remain under final archive history;
that attempt is not relabelled as passing.
Corrected native static/dependent engine builds and all57 scoped CPU checks passed:
10focused,20exchange,12prepared,9supporting native,6warm recipe. Focused/prepared/
supporting CTests ran with parent TI_LIB_DIR unset; direct executables used explicit
runtime setup. Build logs contain no compiler warnings/errors. One passing
1-bulk-exchange_commit emitted an offline-cache lock warning; exact diagnostic is
retained without cleanup/rerun. No warning-free-runtime claim is made.
Fixture observations:map entries2/3/4/0, required9, scalar capacity160 bytes,
moved-in key capacity92 bytes; these are observations, not portable constants.
Existing argument40/result0/recordcapacity144 values remain unchanged.

Tested WSL source is clean. Final publication differs only in four Markdown files;
123local file links and whitespace checks passed. Contributor-local archive:
prepared-metadata-domain-83c1d74d.tar.gz.
Manifest SHA256:8a58debaf890b9a23ad6400cd8f0f78047e3cd920c0835bde4191944ccdb771e.
Archive SHA256:9c09a1302f26038699d1666b06ec3ef092c1a9cf82963e866317ae78da148ad1.
Includes source/configuration/compiler/binary pins, commands, results and failed
initial observation. Raw artifacts have not been independently inspected remotely.
Hosted qualification/maintainer acceptance remain separate; no self-merge.
## Prepared map allocation requests — 2026-11-12

Continues #310 review5883920961 at75025c10, source provisionally accepted without
blocking findings. Main cee73d53 remains integrated through #308 and is already in
this stack; #309/#310 hosted integration stays pending. Open queue and remote refs
were rechecked; no duplicate parent qualification or self-merge.

Four native binding maps now use stable shared request counters. Actual rebound
node/bucket requests are reported separately from key capacity and value-payload
subsets. Counter object and actual shared_ptr control-block request are separate
reported allocations. Moves/rebound allocator copies retain domain lifetime; ordinary
container copies select fresh domains. Public allocator sharing requires domain-wide
quiescence/deduplication; identities are synchronous, not durable tokens or leases.
Entry-budget preflight is unchanged; full host/runtime metadata remains incomplete.
See [the contract](../../docs/BORROWED-RESOURCES.md).

Tested source:8001358dc47449a6c4e4df3d7a99b7058e0c1f7d.
Tracked Taichi tree:ca93f571203ffe232beaba0dd23142060bf0e24e.
Expanded prepared_backing_test compares all retained map/counter/control-block/key
requests against an independent live-allocation witness after source destruction,
rehash and clear. It checks copy-domain independence, move assignment with a displaced
destination, reuse/destruction of a moved-from map and zero retained allocations after
teardown. Every observed allocation site in that scenario is failed individually,
including counter/control-block construction; each must reject and reclaim all acquired
storage. Existing budget/refusal/retry/move/import/execution checks remain in scope.
No GPU/native Windows/database/browser/performance qualification is claimed.
Native static/dependent engine builds and all57 scoped CPU checks passed:
10focused,20exchange,12prepared,9supporting native,6warm recipe. The allocator scenario
exercised16 failure sites, each with zero retained allocations after unwind. Live
request equality passed after source destruction, rehash and clear. Existing observed
map counts2/3/4/0, required entries9, scalar capacity160/key capacity92 and
argument40/result0/recordcapacity144 remain unchanged.

Build logs have no compiler warnings/errors. One passing2-bulk-exchange_commit emitted
an offline-cache lock warning; exact diagnostic is preserved without cleanup/rerun.
No warning-free-runtime claim. No source build/test failure occurred.
Focused/prepared/supporting CTests ran with parent TI_LIB_DIR unset; direct executables
used explicit runtime setup. Tested WSL source is clean. Final publication changes
four Markdown files and one explanatory header comment from tested source; executable
source is unchanged. 124local file links and whitespace checks passed.

Contributor-local archive:prepared-map-domain-domain-8001358d.tar.gz.
Manifest SHA256:785a1ac9acaf2078f166619638ce3d75a5436c8e0e04ada7758dded23005965f.
Archive SHA256:1934785f84441f35416100771058bc398a06ef3bce7140ea1fa8e09bcf760cf3.
Source/configuration/compiler/binary pins, commands, full results and diagnostics are
retained. Raw artifacts have not been independently inspected remotely. Parent310
smoke43332423712 passed/native43332423596 remains active; no duplicate run or self-merge.
## Runtime bookkeeping tables — 2026-11-13

Starts from mainc5645a28 after qualified #309–#311 integration. #311 integration note
6850662795 records all12jobs passed for #310/#311 and personally inspected119 #311
regressions plus addressed_reads24/private_reads0. Main differs from qualified311 only
in two handoff documents. Remote refs/open queue checked; only held75/111/123 remained.
No accepted-baseline rerun or self-merge.

Common ProgramImpl::runtime_bookkeeping exposes LLVM executor base/root-manager object
storage and three retained map families. Actual node/bucket/counter/control-block
requests use reviewed shared allocator domains; value and incremental record payloads
remain subsets. Fixed reads allocate/enumerate nothing. These tables may retain retired
descriptors and cannot certify live payload backing. Full runtime completeness is false;
remaining shared/compile/thread/backend surfaces are explicit exclusions in
[the contract](../../docs/BORROWED-RESOURCES.md).

Tested source:e94e8c03059daa0b54ab73457c7104269e142f0a.
Tracked Taichi tree:c2c0c512b5b7aa2ed05e30fbb1c3a9df7b40b7d8.
Expanded owner_backing_test checks unsupported versus supported-empty maps, separate
domains/counter storage, root insertion, payload subsets, allocation-free stable queries,
retained ndarray descriptors after Device retirement, root-map release and retained tree
descriptors after tree destruction. Existing prepared allocator live-witness/16failure
controls remain in the focused subset. CPU/Linux/WSL shared host only; no GPU/native
Windows/database/browser/performance qualification is claimed.
Native static/dependent engine builds and all57 scoped CPU checks passed:
10focused,20exchange,12prepared,9supporting native,6warm recipe. No source build/test
failure; captured build/runtime logs contain no warnings/errors. Focused/prepared/
supporting CTests used parent TI_LIB_DIR unset; direct executables used explicit setup.
Fixture observations:root-map requests208 bytes, tree-map requests168 bytes, one retained
runtime descriptor after array retirement; root backing4096/array request28/Ndarray
object224 bytes. These are observed values, not portable constants.

Tested WSL source is clean. Final publication changes only four Markdown files;
125local file links and whitespace checks passed. Contributor-local archive:
runtime-metadata-domain-e94e8c03.tar.gz.
Manifest SHA256:ca446301d2e6e9b4ba65c55fec6fb5ea12a01bcd0ffc620eae85a032104b9e06.
Archive SHA256:1d14d90cd45de7a36d08167b7171c93ff0b431f4b532170c2f4ce7178dc2bc9e.
Retains source/configuration/compiler/binary pins, commands and complete results.
Raw local artifacts have not been independently inspected remotely. Hosted qualification
and maintainer acceptance remain separate; no duplicate accepted run or self-merge.
## Runtime coverage/exclusion source audit — 2026-11-13

Inspected reviewed312 source9d459141c854e923e5eefd6bac776713cd5bb08d/native tree
c2c0c512b5b7aa2ed05e30fbb1c3a9df7b40b7d8 after review5890043642. Main remains
c5645a28 through311;312 is provisionally source-accepted and unmerged. No duplicate
native build/CI run. [Coverage policy audit](../../docs/RUNTIME-COVERAGE.md) traces
Program/ProgramImpl, LLVMRuntime initialization, executor materialization, CPU launcher
registration, ThreadPool and HCP owner contracts. It separates required-family gaps
from metric exclusions and proposed compiler/JIT scope. No new policy is accepted here.

Concrete next code scope is native runtime-global object/table/temporary/random/result
request export, preserving ambient-metadata coverage and canonical backing deduplication.
Shared launch registry and Program/thread ownership remain explicit subsequent coverage
work. Source inspection is the evidence for this documentation-only slice; no executable
code, completeness flag or admission behavior changed. Prior CPU results remain prior
evidence, not rerun results. No GPU/database/browser/native-Windows qualification.
Validation:144 local file links and whitespace checks passed across the five changed Markdown files. No executable files changed.
