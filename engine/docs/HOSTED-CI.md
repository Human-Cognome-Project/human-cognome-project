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

## Ubuntu archive resolution repair (2026-11-11, awaiting review)

Reviews of #275–#277 identified repeated DNS resolution failures for
azure.archive.ubuntu.com during apt/toolchain installation, before product
compilation. #276 viewport attempts failed during Playwright dependency installation
and were cancelled before browser tests. Repeated identical retries did not restore
qualification. Those attempts remain failed/cancelled; skipped suites are not passes.

`hosted_ubuntu_mirror.sh` replaces only the exact Azure Ubuntu archive URL with
`archive.ubuntu.com` in legacy `.list` and deb822 `.sources` files. It preserves
protocol, suites, components, Signed-By keys and security.ubuntu.com; unrelated
repositories are untouched. This is Ubuntu's primary archive, documented in the
[Ubuntu package archive reference](https://ubuntu.com/project/docs/how-ubuntu-is-made/concepts/package-archive/).
No package authentication bypass, toolchain-version change, numerical expectation
change or weakened test gate is introduced. A remaining download failure still
fails the existing dependency command.

All four native job definitions that install APT packages normalize sources before
apt update. The viewport job does so before Playwright installs its dependencies.
The viewport path filter includes the shared helper/test. Native paths already
include engine/**. Local validation never changes this machine's APT configuration;
the optional script argument points fixture tests at a temporary source root.

Tested source `464e4410984f98b4bacff566c4bfc96e1f12a814`: ShellCheck passed for
existing orchestration and new scripts; mirror fixtures passed legacy/deb822 exact
URL replacement, signature/suite/security preservation, unrelated-host protection,
idempotence and absent sources. Existing orchestration contracts passed complete
shards, failure propagation, inventory and bundle provenance guards. Workflow wiring
was inspected for all four native install sites and the viewport working directory.
No product baseline or browser suite was rerun locally for this script-only change.

Verified local archive `setup-evidence/ci-ubuntu-mirror-464e4410.tar.gz`; manifest:

    14003deee241e272684696b970f235840e4f1927ddb0c77f375d6c9443aebd0e

Final tested-to-head changes are documentation only. Local fixtures prove rewrite
behavior, not remote mirror availability or hosted qualification. Raw local evidence
has not been independently inspected remotely. Hosted runs must qualify this repair
before the blocked allocation stack can integrate; no self-merge or automatic
relabeling of earlier attempts follows. The identity-text coding slice remains queued.

### Mirror-list correction after review

Review #278/5863935970 found the initial helper did not cover the runner's actual
`mirror+file:/etc/apt/apt-mirrors.txt` indirection. Hosted native job 130734677651
still downloaded from Azure after the old normalization message. Earlier local
direct-source passes did not prove the workaround effective; preserve this finding
even where installation happened to succeed.

The helper now also examines `apt-mirrors.txt` within the supplied APT root. It
changes only matching archive URLs, preserving mirror metadata, unrelated endpoints
and the source's mirror+file/Signed-By configuration. Diagnostics list changed files
and count them, or explicitly report no matching URL and no files changed.
The isolated fixture includes mirror+file sources and a mirror list with priorities,
architecture/suite metadata, security and unrelated endpoints; exact-content and
second-run assertions verify preservation and idempotence.

Correction source f137938f52a9361f479697f975353a5209324d62 first stopped at ShellCheck
SC2129 on repeated fixture redirects; fixture/orchestration tests did not run at
that source. The retained local log is ci-ubuntu-mirror-f137938f/shellcheck.log.
Grouping fixture writes fixed it without changing production behavior.
Tested correction `f87fa4baef9ad9eebe9e8bc37f0ddb6c3c6b1282`: ShellCheck, direct and
mirror-list fixtures and existing orchestration contracts passed. Local APT sources
were untouched. Verified archive `setup-evidence/ci-ubuntu-mirror-f87fa4ba.tar.gz`;
manifest digest:

    35f463cb94e42385e4ef18902e3b468b513cbc21ce58e87444bef7af8e567a39

Hosted installation logs must additionally demonstrate archive.ubuntu.com traffic;
fixture success alone does not establish that the runner's failing endpoint changed.
