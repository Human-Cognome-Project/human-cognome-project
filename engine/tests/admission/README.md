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
above remains separate. Integration with the new main CI workflow is pending;
complete B3 admission and GPU qualification remain outstanding.
