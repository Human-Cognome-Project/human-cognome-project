# Validation and evidence

Read when building, testing or reviewing a contribution. [Choose another path](README.md).

## Build, recovery and evidence discipline

The native CPU baseline uses Ubuntu 24.04, LLVM/Clang 15, CMake and Ninja;
the checked-in [workflow](../../.github/workflows/native-engine-cpu.yml) records
dependency packages, configuration and the full acceptance matrix. Build the
repository's fork first, then its matching HCP wrapper. A pip/system Taichi is
not interchangeable. Keep exact submodule commits; use the documented Eigen
mirror fallback if needed. Do not copy build caches, headers, libraries or runtime
bitcode from an unknown previous environment.

Start with the one-test smoke check in First run. Enable PostgreSQL/viewport
clients only when needed. A native smoke run requires neither a GPU nor database
credentials. If source-only work is possible but compilation is blocked, record
the exact missing dependency or resource and preserve the completed work.
Do not report missing/skipped tests as passes.

For changed behavior, run focused tests followed by the owning plan's required
matrix. Check existing jobs before triggering CI; do not duplicate a running
suite or rerun an accepted baseline merely because the environment changed.
Record source SHA, actual backend, toolchain/configuration, commands, results,
compiler artifacts where required, and unchanged-state/retry witnesses. Separate
local contributor evidence, personally executed checks and hosted CI.

CPU success or CUDA-enabled compilation is not GPU execution. #196 has bounded
local GTX 1070 evidence; full integrated GPU qualification remains open. The
CUDA f32-to-f64 `0x1p-149` promotion failure is still a separate task. Keep the
C3 CUDA qualification guard and supported-backend limits explicit.

## Local evidence and provisional continuation — 2026-11-02

Patrick confirmed that contributions from the app on his local machine will
include local CI evidence for provisional maintainer acceptance and continued
work. Review the source and evidence appropriate to the changed scope; do not
hold otherwise accepted follow-on work solely for hosted CI latency.

Record the tested commit, actual backend, configuration, commands, results,
retained evidence and the profiles not run. A focused local pass supports only
its tested scope. Distinguish contributor-reported results from raw evidence
personally inspected, and local execution from hosted execution. Machine-local
hashes identify evidence but do not make the raw bundle remotely available.

Hosted CI remains authoritative for the checks it can perform. Provisional
acceptance is permission to continue the reviewed scope, not a claim that
pending hosted checks passed or that untested behavior is qualified. Reconcile
later hosted findings before final acceptance/merge; do not duplicate a running
suite merely to permit local progress. Keep dependent work in reviewable
commits or stacked PRs, with its provisional dependencies explicit. GPU behavior
requires actual local hardware evidence and is not inferred from hosted CPU CI.

