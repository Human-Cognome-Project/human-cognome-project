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
