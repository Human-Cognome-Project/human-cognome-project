# Local CPU evidence for provisional review

`bash engine/scripts/run_local_ci.sh PROFILE /absolute/new/evidence-directory`
runs the existing native tests from a clean, committed Linux checkout, including
WSL2. It rebuilds incrementally with LLVM 15 and explicitly disables GPU backends.
Wrapper configuration explicitly binds `ENGINE_TAICHI_ROOT`,
`ENGINE_TAICHI_BUILD` and `ENGINE_TAICHI_RUNTIME_SOURCE` to absolute paths in
that checkout, replacing inherited cache overrides. Incremental reuse remains
enabled for this matched build.
No Python frontend, Docker or GPU is needed. It does not install packages, fetch
source, reset databases or change hosted CI. Use a separate checkout while a run
is active: edits or a changed HEAD invalidate its final result.

Prerequisites are the exact initialized recursive submodules, Bash, CMake, Ninja,
Clang/LLVM 15 at `/usr/lib/llvm-15`, a C++ toolchain, libx11-dev, libpq-dev,
zlib1g-dev and libzstd-dev. Database runs also need PostgreSQL client tools and a
running server. Build output uses `engine/build` and `engine/taichi/build-review`;
do not share these directories with a GPU build or another concurrent run.
`HCP_LOCAL_BUILD_JOBS` defaults to 2. CTest runs serially to avoid overlapping
native runtime memory demands; explicit matrix workers are still tested.

| Profile | Evidence supplied | Not established by this profile |
|---|---|---|
| `quick` | Smoke, capacity, strict arithmetic, 632-sample placement math and placement-capacity checks | Full regression, compiler matrix, DB integration |
| `regression` | All configured CPU CTest suites and complete CTest log | Extra no-unroll matrix, DB integration |
| `matrix` | 1/2 workers × bulk/direct × default/no-unroll emitted modules and audits; no-unroll hierarchy/refresh/selection/contact/placement runs and interval math | Full default CTest suite, DB integration |
| `database` | Native warm-active integration on a new retained warm fixture; cold before/after data fingerprints | Hosted container reproducibility, all other database/WAL tests |

For example, after creating an evidence parent directory outside the checkout:

```sh
bash engine/scripts/run_local_ci.sh quick /work/evidence/quick-001
bash engine/scripts/run_local_ci.sh regression /work/evidence/regression-001
bash engine/scripts/run_local_ci.sh matrix /work/evidence/matrix-001
bash engine/scripts/run_local_ci.sh database /work/evidence/database-001
```

Each destination must be new. Logs include both stdout and stderr; failures retain
their output. `result.txt` reports the exit code and last step. A missing result
means incomplete/interrupted evidence, never success. `commands.txt`, `head.txt`,
submodule pins, both CMake configurations, binary/runtime hashes, environment
and load snapshots, raw logs and emitted modules remain together. `SHA256SUMS`
covers all retained files; check it from the evidence directory with
`sha256sum -c SHA256SUMS`. Do not publish machine evidence without reviewing it
for hostnames, paths or identifiers; the runner never dumps the environment or
connection secrets. Build configuration may contain local paths.

To check that a reused CPU cache cannot redirect these source pins, run
`bash engine/scripts/check_local_ci_cache.sh /absolute/new/cache-evidence` after
the first CPU build. It configures valid alternate source/build path aliases
and a copied runtime, verifies those cache entries, runs the quick profile,
then verifies all three corrected paths, runtime bytes and unchanged compiled
archives on an incremental substrate build (runtime bitcode is regenerated).
It retains before/after caches and does not require another
fork compilation. Do not run this witness concurrently with other build work.

## Local PostgreSQL

Use libpq service/password files outside Git, with owner-only permissions.
The default services are `hcp-core`, `hcp-linguistics`, and `hcp-warm`; override
them with `HCP_LOCAL_CORE_SERVICE`, `HCP_LOCAL_LING_SERVICE`, and
`HCP_LOCAL_WARM_SERVICE`. The application role defaults to `hcp_local`, overridden
by `HCP_LOCAL_PG_ROLE`. Cold services must have read-only credentials. The warm
service's database name is overridden only for the new fixture.

Creating that fixture requires a database administrator. Supply
`HCP_LOCAL_PG_ADMIN_SERVICE` for a configured libpq administrator service, or run
locally as root to use the OS `postgres` account through peer authentication.
All services must address the same PostgreSQL server. The application role must
already exist. The script creates a unique `hcp_ci_*` database owned by that role,
loads the v1 schema expected by the existing integration executable and runs the
native client. It does not migrate or reuse the operational warm database.
The fixture is retained for inspection; its name is recorded. No drop, truncate,
reset or automatic cleanup operation is included. Cold data is fingerprinted
before and after; concurrent cold-data writes cause verification to fail.

## Reporting for review

Report the commit, profile, final exit code, actual backend, host/toolchain,
test counts, evidence location and checksum manifest. State which profiles have
not run. A passing quick profile is useful provisional evidence for a bounded
change; it is not full acceptance. Keep local results separate from hosted runs
and from predecessor-commit evidence. Even all four profiles do not reproduce
every hosted workflow (repository smoke, retained snapshot checks, viewport,
stale-CUDA archive probe and container setup are separate). Shared-host timing
is not a dedicated-hardware benchmark. GPU and GTX 1070 qualification remain
separate and require the user's advance GPU-use notice.

Final acceptance remains a maintainer decision after the applicable source,
evidence and hosted checks have been reviewed. Do not close review findings or
mark an implementation accepted merely because a local command exits zero.
