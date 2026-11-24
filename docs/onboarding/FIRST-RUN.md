# First local native check

For a first contribution to composition, identity or focus, start with the [standalone value suite](#optional-check-logical-assembly-without-the-native-runtime). For work on native execution, use the matched runtime build below.

For environment recovery and existing contribution selection, read the
[fresh-instance brief](AGENT-START.md) first. A clean smoke check is the setup
gate; it is not a request to duplicate an already running full CI suite.

Goal: build the matched native runtime and pass `engine_smoke_test`, a test
included in the hosted [native CPU workflow](../../.github/workflows/native-engine-cpu.yml).
This checks local native execution; it is not the full acceptance suite or proof
of live insertion, language settling or GPU performance.

## 1. Start from current main

Use a fresh checkout for this walkthrough:

```sh
git clone https://github.com/Human-Cognome-Project/human-cognome-project.git
cd human-cognome-project
git switch main
git submodule update --init --recursive
```

Keep the pinned submodule revisions. If Eigen's GitLab host is unavailable, use
the exact-commit mirror fallback in the [engine build guidance](../../engine/README.md#local-cuda-build);
that dependency-fetch fallback also applies to the CPU build.

## 2. Build the matching native stack

Follow [engine Building](../../engine/README.md#building): build the modified
Taichi fork first, then configure the HCP wrapper against that build. Do not
substitute a system or pip Taichi package.

The hosted baseline is Ubuntu 24.04, Clang/LLVM 15, CMake and Ninja with the
native C API enabled and the Python frontend/accelerator backends disabled.
The workflow records exact dependency installation and configuration flags.
Use the engine README as the build recipe; report any discrepancy with CI.

For this first check, keep `HCP_WITH_WARM_INTEGRATION` and
`HCP_WITH_VIEWPORT` off (their defaults). No PostgreSQL server, cold-data restore,
GPU or database credentials are needed. CI enables optional clients for its
broader coverage; those are not prerequisites for this smoke test.
Allow time and memory for compiling the native fork; this is not a prebuilt demo.

## 3. Run the named check

From the repository root, after building:

```sh
ctest --test-dir engine/build -N -R '^engine_smoke_test$'
ctest --test-dir engine/build -R '^engine_smoke_test$' --output-on-failure
```

The first command must list exactly one test. The second must report that test
passed with zero failures. CTest supplies its runtime environment. For direct
executables, source the generated environment first:

```sh
. ./engine/build/engine-env.sh
./engine/build/engine_devices --cpu-only
```

For implementation qualification, run the required configured suite with
`ctest --test-dir engine/build --output-on-failure`.
Its test count depends on enabled optional targets; a smoke pass is not a claim
that the broader suite or PostgreSQL integration has run.

If a check fails, retain the commit SHA, OS/toolchain versions, configure command,
matched Taichi build path, failing command and CTest output
(`engine/build/Testing/Temporary/LastTest.log`). A missing test is a setup or
documentation problem, not a passing run. Do not fix runtime mismatch by mixing
headers/libraries/bitcode from different builds.

## Optional: check logical assembly without the native runtime

The independent value suite includes `active_assembly_test`, which compiles,
links and exercises logical shared/private input, identity, focus and geometry
validation without Taichi or PostgreSQL. This optional check does not execute
native physics or replace `engine_smoke_test` above. It runs in the hosted
native CPU workflow's `projection-values` job.

```sh
cmake -S engine/tests/letter_projection -B /tmp/hcp-logical-values
cmake --build /tmp/hcp-logical-values -j 2
ctest --test-dir /tmp/hcp-logical-values --output-on-failure
```

All five value CTests must pass. For the source-pinned implementation and native
compatibility evidence, see the [logical boundary](../../engine/docs/ACTIVE-WORKBENCH.md#logical-header-and-standalone-target--2026-11-23).

## Optional: look at a retained capture

Open [the bundled viewport](../../tools/viewport/index.html) locally in a
WebGL2-capable desktop browser. Choose **Open snapshot** and select
[letter-study.json](../../tools/viewport/examples/letter-study.json).
Orbit, pan and zoom to inspect the 16 exposed occurrences.

No build, npm install, server, database or engine connection is needed for this
retained capture. It shows a supplied fixture after eight CPU ticks, not a
settled language model. Camera navigation does not select engine focus or LoD.
See the [viewer guide](../../tools/viewport/README.md) for browser constraints
and separately generating new captures.

## Later paths

- PostgreSQL integration: use the owning component's documented isolated
  fixtures and retained-data verification. Never reset/drop persistent project
  databases. Database identity is independent of snapshot filename.
- GPU: follow [local build guidance](../../engine/README.md#local-cuda-build)
  and [#145](https://github.com/Human-Cognome-Project/human-cognome-project/issues/145).
  Verify the actual backend; successful execution alone is not pinning or
  performance qualification.
- Pick implementation work through [current state](../../AGENTS.md#current-state)
  and its accepted issue/plan, not through this first-run recipe.

