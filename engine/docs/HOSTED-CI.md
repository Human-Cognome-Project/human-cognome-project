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

The four `placement_{1,2}_{bulk,direct}` CTest registrations have a bounded
**2400-second** timeout. The combined matrix has grown to 22 placement, admission,
exchange, owner, focus and failure cases. In #223's hosted run 42122775107,
`placement_2_bulk` passed both owner witnesses before its previous 1200-second
limit killed the remaining matrix at 1200.23 seconds; the shard finished 28/29.
Direct inspection of job 127581880062 confirms 21 of 22 case-result PASS lines
and no case-failure lines before the final admission-contact remap case timed out.
[Review 5793702801](https://github.com/Human-Cognome-Project/human-cognome-project/pull/223#pullrequestreview-5793702801)
requires enough bounded time to finish the unchanged coverage. The 40-minute
budget gives headroom over that observed limit without removing cases, splitting
test indexes or changing worker/path/compiler configurations. Other CTest limits,
the 90-minute consumer-job limits and the all-jobs acceptance gate are unchanged.
This does not cure unrelated package-installation delays: #222's cancelled run
42094921159 remains incomplete evidence, while the review records #221's run
42086483712 as green. Parent integration/status reconciliation belongs to the
maintainer; no parent reruns are implied by this adjustment.

The corrected #223 head `2286950ae07180c5a23b6d0de38792f064a570f5` passed
run 42134552198. All 115 tests passed, including all four placement registrations;
`placement_2_bulk` completed in 1150.36 seconds. All compiler and database jobs
and the aggregate gate passed. This clears the integrated stack's timeout blocker;
the earlier failed/cancelled runs remain historical evidence. See
[final acceptance](../tests/admission/README.md#maintainer-integrated-acceptance--221223-2026-11-06).

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

## Initial hosted acceptance — 2026-11-03

PR #206 at `9f78602f96859d461370213ae7770372278e617b` passed all
18 checks in [run 41667320655](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41667320655).
Inspected regression logs cover all indexes 1–115 exactly once across shards
of 29, 29, 29 and 28 tests. All four compiler jobs completed and retained their
mode evidence; database integration passed 180 checks with 24 addressed reads
and zero private reads. Provenance/backend and artifact checks passed.

The native critical path ran from 15:46:21 to 16:31:02 UTC: **44m41s**, compared
with approximately **129m48s** for the reference run above. This is an observed
cross-run comparison, not a controlled benchmark. The inspected build artifact
(13247902226) reports 260 cacheable compilations, zero hits and 260 misses;
cache restore also explicitly missed. The elapsed-time improvement therefore
establishes the benefit of the job layout without a warm-cache benefit claim.

Merged as `943e0549c62af92a4aefc0c7b9f78a2e2fb214e8`, followed by #205
at `ef0ede690900e6b3cea4bdfce7fc0e33b7a6021b`. Integrated main run
[41720357523](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41720357523)
passed, including the final native-cpu gate (job 126238016942). The separate
[main smoke run 41720357550](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41720357550)
also passed.

### Integrated main verification

The native critical path was **51m21s**, from 21:16:59 to 22:08:20 UTC on
2026-11-03. Build job 126211185168 took 25m48s including checkout/submodule
setup. Independently inspected shard logs cover indexes 1–115 exactly once:

| Shard | Job | Tests | CTest seconds |
|---|---|---:|---:|
| 1 | 126224780479 | 29 | 1477.49 |
| 2 | 126224780703 | 29 | 765.34 |
| 3 | 126224780592 | 29 | 732.26 |
| 4 | 126224780719 | 28 | 1048.40 |

Compiler jobs 126224780344, 126224780486, 126224780212 and 126224780046
passed. Their independently inspected artifacts (13274111031, 13275117418,
13275530538 and 13275770327) retain all eight default/no-unroll configurations,
passing audits and five extra no-unroll cases per worker/source-path job.
Both bulk artifacts also report 632 passing placement arithmetic samples.
Database job 126224780021 passed 180 checks, with 24 addressed reads and zero
private reads. Consumer digest/source/path/backend checks and artifact uploads
succeeded.

Build evidence artifact 13271479740 reports **260 cacheable calls, zero hits,
260 misses**, using 2.99% of the 1 GB cache limit. Restore explicitly missed;
the main build successfully saved its toolchain-qualified cache under source
`ef0ede690900e6b3cea4bdfce7fc0e33b7a6021b` at 21:42:44 UTC.
This verifies cache publication, not warm-cache reuse or a warm-cache speedup.
The rollout is complete. No additional benchmark rerun was needed, and these
hosted CPU results do not qualify contributor-local or GPU execution.
