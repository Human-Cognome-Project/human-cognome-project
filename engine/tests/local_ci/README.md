# Local CI tooling validation — 2026-11-02

Provisional tooling evidence, not engine acceptance or hosted CI parity.
Tested script/source commit: `062e7dfd8c3f5c73dc2b8b4663ae844a9e96864b`.
Parent implementation is PR #203 at `175a45ce`.

Environment: Ubuntu 24.04 under WSL2, Clang/LLVM 15, CMake 3.28.3,
PostgreSQL 16.15, CPU/x64; two build workers and serial CTest. Shared personal
host; no GPU execution. Native libraries and runtime were incrementally rebuilt
with all accelerator backends explicitly disabled. No timing qualification.

The [local runner](../../scripts/run_local_ci.sh) completed:

| Profile/check | Result |
|---|---|
| Shell syntax and ShellCheck | Both scripts pass |
| Invalid profile / existing evidence directory | Rejected before building |
| Concurrent runner | Rejected while the first runner held the build lock |
| `quick` | Exit 0; 3/3 CTest checks, 632 interval arithmetic samples, placement-capacity matrix with zero failures |
| `database` | Exit 0; 180 integration checks, 24 addressed reads, zero private reads; cold fingerprints unchanged |

Quick CTest selection was `prepared_capacity_test`, `engine_smoke_test`, and
`strict_math_cpu`. The database run created a fresh retained `hcp_ci_*` warm
fixture and left the operational warm database untouched. Both evidence
directories passed `sha256sum -c SHA256SUMS` after completion. Earlier runs at
`d9e25700` also passed, but the table above refers to the corrected final scripts.

Raw logs, configuration, binaries' hashes and source pins are retained locally
under the evidence names `quick-062e7dfd` and `database-062e7dfd`. SHA-256 of each
directory's `SHA256SUMS` file:

```text
quick:    a805efb12f2df416100f106da2964191d14547bd9354273595cb272ff74970c3
database: 64e55dab7cbda97eb4de5a3bf0d1deaec2be4a3a133e540d9154f351628079b6
```

These hashes identify local evidence; they do not make it remotely available.
The raw bundle remains machine-local because it includes local build paths and
host configuration. Supply it separately for detailed review when appropriate.
No full `regression` or `matrix` run is claimed for this tooling validation.
Those profiles mirror the named hosted native stages but do not implement every
other repository workflow. Follow [LOCAL-CI.md](../../docs/LOCAL-CI.md) for scope,
commands, reporting and the distinction between provisional and final acceptance.
