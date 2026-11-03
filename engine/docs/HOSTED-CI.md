# Hosted CPU CI

The native workflow builds once, then executes independent test groups on
separate Ubuntu 24.04 runners. The `native-cpu` check remains the final acceptance
gate: the build, all four regression shards, all four compiler-mode jobs and
the database job must succeed. Failed, cancelled or skipped jobs fail the gate.
`projection-values` and the repository smoke checks remain separate checks.

## Reuse and provenance

Only C/C++ compiler outputs use a cross-run `ccache` cache. Both builds still
configure and build the checked-out revision. Compiler content, source/header
inputs and compiler options determine object reuse; no relaxed `sloppiness`
settings are enabled. Cache storage is separated by runner OS/architecture and
toolchain package versions. Main push builds publish caches; PRs only restore
them. Cache misses are ordinary builds. Cache statistics are retained in the
build evidence and job summary. The first PR run may be entirely cold because
there is not yet a main-branch cache.

The wrapper explicitly binds Taichi source, build and generated runtime paths
to this checkout. No CMake build directory is restored from another run.
Generated runtime bitcode is rebuilt, and runtime JIT/compiler audits and all
tests execute afresh. A manual workflow dispatch with `cold_build: true` disables
object reuse and provides a clean-build witness without changing the test gates.

Within one workflow run, the build job packs the executables, staged runtime,
CTest definitions and configuration into a tar artifact. Object files, static
archives, CMakeFiles and Testing output are omitted. Tar preserves executable
permissions. Consumers download the producer's named artifact from this run,
verify its SHA-256 against the producer output, then verify the exact checked-out
commit, Taichi subtree and absolute workspace path. They do not rebuild or
reconfigure. A mismatched path fails rather than rewriting CTest paths. The
actual CPU-only backend check runs in the producer and each consumer.

The build artifact expires after seven days. Rerunning a failed consumer within
that window can reuse its successful producer's artifact, including across run
attempts. After expiry, rerun the whole workflow to produce a new build. Missing
artifacts or failed provenance checks cannot be treated as successful tests.

## Test coverage and evidence

| Job | Coverage and retained evidence |
|---|---|
| `native-build` | Same CPU-only fork/wrapper configurations, stale-CUDA archive probe and backend check; source/tree/submodule pins, both CMake configurations, compiler version, complete CTest inventory and cache statistics. |
| `native-regression` (four shards) | All configured CTest tests exactly once, assigned by index modulo four. Each shard runs with `-j 1`, checks its selected names against the expected slice and the producer's inventory, and retains test lists, CTest output, JUnit results and LastTest.log. New registered tests are included automatically. |
| `native-compiler` (one/two workers × bulk/direct) | Each job collects both default and no-unroll C3, selection, contact and placement compiler evidence, then runs all five extra no-unroll functional cases. Each worker count also runs its no-unroll placement arithmetic test once, in the bulk job. Full output directories upload even on failure. |
| `native-database` | Unchanged pinned cold fixtures and PostgreSQL image, newly created retained fixture volume, native warm-to-active integration and cold fingerprints. |

Each compiler/regression artifact name includes its matrix coordinates. This
avoids overwriting evidence between jobs. Matrix fail-fast is disabled so a
failure does not cancel other evidence-producing jobs. Test invocations remain
serial inside each runner; two-worker tests do not compete with other tests on
that runner. No existing mode or assertion is removed to reduce elapsed time.

## Validation and expected benefit

`bash engine/scripts/check_hosted_cpu_ci.sh` exercises real CTest partitioning
with an uneven nine-test inventory, test failure propagation, inventory drift,
archive exclusions, executable/runtime preservation and rejection of wrong
digest, source and workspace. It does not claim physics validation. Hosted
builds run this contract check and ShellCheck before building the native code.

The pre-change reference is [run 41550647756](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41550647756):
about 12 minutes building, 13 collecting compiler evidence, 63 running CTest,
38 running extra no-unroll cases and four for setup/integration. Compiler caching
targets build time. Independent runners shorten the much larger testing portion;
they do not reduce total numerical work. Runner queues, artifact transfer and
uneven test lengths affect the result. Compare actual cold/warm build statistics,
job timings and full test coverage before claiming a speedup. Hosted CPU success
does not qualify local GPU behavior.
