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

## Reused-cache provenance correction — 2026-11-03

At `d4adfd7047aabf8e94988b977bc43a0ef4d5af4c`, the focused
`check_local_ci_cache.sh` witness passed. It first configured and verified three
valid alternate CPU cache paths (source/build aliases and a separate runtime
copy). The real quick runner replaced all three with this checkout's absolute
paths. Runtime bytes matched the rebuilt source; compiled archive paths, sizes
and modification times remained identical across another incremental build.
Taichi's expected runtime-bitcode regeneration was allowed.

Bash syntax and ShellCheck passed. The quick profile again passed all three
CTest checks, 632 arithmetic samples and the placement-capacity cases. No full
physics regression, compiler matrix, database or GPU rerun is claimed for this
path-binding correction. The prior database evidence retains its original pin.
Local evidence is retained as `cache-d4adfd70`, including before/after CMake
caches, archive inventories, the quick run and its verified checksum manifest.
The outer evidence `SHA256SUMS` digest is
`692a2633c74bf3a8b29e22d54d71c231b64fa73304b0c2d8b178d4ee07e9e764`.

## Numerical-kernel extraction — 2026-11-07

First mechanical slice of [#229](https://github.com/Human-Cognome-Project/human-cognome-project/issues/229),
explicitly based on provisionally accepted #228 at
`4596c3535bba92efa533616f074d702ede85939c`.
Tested code: `44ff31fef653f1afa42b129c3db81ad8d978b007`;
Taichi: `5ba579a8a11255b7a258cde93801070956d81d57`.
The subsequent contribution changes documentation only.

Native CPU/x64 under WSL Ubuntu, Clang/LLVM 15; CUDA disabled in the build and
hidden from execution. The shared host ran CTest with two jobs alongside four
independent compiler-matrix processes. No timing, GPU or database qualification
is claimed. No test expectations, compiler auditors or negative controls changed.

| Check | Result |
|---|---|
| Full native build, `cmake --build engine/build -j 2` | Passed |
| `ctest --test-dir engine/build -j 2 --no-tests=error --output-on-failure` | 115/115 passed |
| Unchanged `engine/scripts/hosted_cpu_compiler.sh`, `HCP_WORKERS=1/2` × `HCP_SOURCE_PATH=bulk/direct` | All four jobs passed |
| Default/no-unroll compiler collection and audits in each job | All eight configurations passed, including placement negative controls |
| No-unroll hierarchy, refresh, selection, contact ticks and placement | All four matrices passed, including all 22 placement cases each |
| Bulk placement arithmetic | Both worker configurations passed, 632 samples each |
| Mechanical source comparison against #228 | Entire builder and all seven helper definitions identical after LF normalization |

The unchanged builder's SHA-256 is
`bd073e2871b8bc704bf3eb2387a8018b5c11debffd3d19e7bb79ba24ceb1ac5a`.
Its private declaration, callers and ownership remain unchanged; only the
compilation-unit boundary and CMake source list change.

Raw logs, emitted LLVM, audit output, build configuration, binary/toolchain
identities, source pins, exact commands and mechanical comparison remain local
in `numerical-extraction-44ff31fe`. All manifest entries verified successfully.
SHA-256 of `SHA256SUMS`:

```text
ae6ea7093056219f7230d6653621306557f8907ce62f94ca76b529626eace09b
```

This identifies contributor-local evidence, not remotely available or independent
maintainer evidence. #228 hosted native run 42370220296 and smoke 42370220331
passed; merge/disposition remains separate. Complete owner resource/lifetime/work
accounting remains next and should guide subsequent #229 factoring. This first
slice does not close the broader issue.
