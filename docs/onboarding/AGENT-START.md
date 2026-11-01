# Fresh-instance onboarding and implementation brief

This is the entry point for a new coding environment or agent with no chat
history. Read it, the full [AGENTS](../../AGENTS.md), and the owning contract
before changing code. This brief summarizes accepted requirements; it does not
replace their detailed acceptance tests.

## Establish the actual starting point

Baseline checked **2026-11-01**: main
`939632dedb9df0234d7e828e5c0555313ee14cec`, merge of #201.
Always fetch current refs and inspect the open PR queue: these pins describe
the handoff, not a permanent instruction to stay on an old commit.

1. Use a fresh clone or inspect `git status --short` before touching an existing
   checkout. Preserve uncommitted work; do not reset or clean it away.
2. Run `git fetch origin`, `git log -5 --oneline origin/main`, and
   `git submodule status --recursive`. Record the selected source SHA.
3. Check [open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls),
   their full descriptions, latest comments/reviews, changed files, head SHA and
   checks. Existing contributions take precedence over starting duplicate work.
4. Select either current main for new work or the explicitly chosen existing
   contribution branch. Read its diff against main before continuing it.
5. Build the matching fork and wrapper using [First run](FIRST-RUN.md).
   Environment recovery does not require a new physics implementation, database
   population or an accepted-baseline CI rerun.

At this handoff, [#202](https://github.com/Human-Cognome-Project/human-cognome-project/pull/202)
is an **open draft** for B2 certified native nearest-free candidate placement,
head `1214e9a0265eb693e4d605add7c496f16e124d36`. Its author reports focused
qualification and outstanding full CPU/compiler/PostgreSQL evidence. Those are
contributor claims, not maintainer acceptance or independently verified hosted
results. The next contributing-agent action is to inspect and continue that
branch's remaining qualification; maintainer review follows the actual ready
source and evidence. Do not implement B2 again on a separate branch or assume
the draft supplies B3. Recheck its live disposition before acting.

## Authority and reading order

| Question | Governing source |
|---|---|
| What is accepted, built, held or pending? | [AGENTS current state](../../AGENTS.md#current-state), current PR dispositions and source-pinned evidence |
| What does the project mean by objects, fields and ownership? | [Orientation](README.md), [model contract](../../AGENTS.md#model-contract), [architecture](../../engine/ARCHITECTURE.md) |
| What is the next core work sequence? | [Core continuation](../core-local-study-continuation-plan.md), slices B–E and independent P |
| What must operational insertion preserve? | [Core operation specification](../core-operation-readiness-plan.md), accepted D1–D7 and full operation matrix |
| How are B1/B2/B3 implemented and qualified? | [Native admission/contact transaction plan](../native-admission-contact-transaction-plan.md), accepted #199 |
| What numerical behavior is current? | [Cumulative publication](../../engine/docs/CUMULATIVE-PUBLICATION.md), [live hierarchy](../../engine/docs/LIVE-HIERARCHY.md), [accepted motion](../../engine/docs/ACCEPTED-MOTION.md) |
| How do I build and report results? | [First run](FIRST-RUN.md), [engine build](../../engine/README.md#building), [contribution/evidence rules](../../CONTRIBUTING.md) |

Code and executable evidence establish built behavior; accepted decisions govern
required behavior. If they disagree, identify the discrepancy. A historical
"next action" is not today's queue. Research and archived notes preserve
derivations, not authorization to replace the accepted model. See the
[documentation map](../documentation-map.md) for other components. Read the
[Covenant](../../covenant.md) and [Charter](../../charter.md) before contributing.

## Accepted baseline and remaining work

| Stage | Accepted scope / remaining boundary |
|---|---|
| Cold and warm data | Retained letter/core snapshots (#136); shared native PostgreSQL toolbox and coordinate-free v2 recipes (#171). Preserve existing stores. |
| Prepared native execution | Mapping, backing and coherent structural edits (#164/#166/#169). Fixture support is not operational admission. |
| C3-1 / C3-2 | Fixed installed CPU hierarchy (#195), accepted wide motion and scoped local CUDA corrections (#196), combined CPU source/hierarchy refresh (#198). New-body starts remain private fixtures. |
| B1 | Native selection gate (#200) and complete CPU contact-safe candidate ticks (#201), including unchanged rejection and one completed publication. |
| B2 | Existing draft #202; certified nearest-free candidate placement awaits qualification/review. Not accepted by this documentation change. |
| B3 | Combined bootstrap/admission remains pending after B2. A placement query alone cannot publish a structural admission. |
| C / D | Enduring prepared owner, occurrence bindings and live operations; then observable same-execution commands. Legacy `ActiveSession`/`FixedStudy` refresh is not live Add. |
| P / E | Separate CUDA mixed-precision repair and integrated actual-hardware qualification. Docker packaging follows the supported core. |

#201's accepted hosted [run 41220868948, job 124594948773](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41220868948/job/124594948773)
at code `3cabc70f1812fceaaf759460fe9328279bfa9d23` passed 109/109 CPU suites
in 4746.01 seconds, compiler collection/audit and extra no-unroll modes,
180 PostgreSQL integration checks with zero private reads, and CPU backend
verification. All eight checks passed. [Contributor evidence](../../engine/tests/contact_ticks/v1/README.md)
is separately pinned; its runtime is not the hosted runtime. The
[acceptance history](../history/acceptance-through-201.md) retains earlier pins.

#156 remains the complete operation owner; #172/#177 own numerical/hierarchy
work and #145 hardware qualification. A bounded slice does not close them.
Held #123 (general identity), #111 (command atomicity/restart) and deferred #75
(old monitor) are not prerequisites for separately accepted work.

## Non-negotiable implementation specification

- **One physical model:** native C++ over the curated Taichi fork. Reuse shared
  native builders and binary activity facilities. Do not add host physics replay,
  a custom scheduler, potential-capacity scans or a second persistent solver.
  Project Python is limited to early bulk-data bootstrap preparation/population/
  validation; upstream Taichi tooling is a separate boundary.
- **One auditable publication gate per destination:** accumulate applicable
  contributions at calculation precision, filter the net once at the expressed
  positional publication boundary, and compare with the stored position. Do not
  discard each small contribution independently, recover discarded child tails,
  or carry an unpublished positional remainder across ticks. Preserve pending
  input, source/mass invalidation and required published history.
- **Accepted wide motion:** positive dt retains accepted displacement/dt;
  zero dt retains incoming velocity. Quiet positive-dt publication retains zero
  velocity. This supersedes older untrimmed-velocity intent for that profile;
  legacy arithmetic remains separately supported.
- **Minimal universal numerical controls:** precision is calibrated, not a fixed
  f60 rule. Density-based f32/f64 selection is an optional future experiment,
  not an instruction to add regional switches now. No arbitrary physical epsilon
  or new damping rule to make small fixtures stop.
- **Distinct identity and mass:** a reusable definition is not a live occurrence.
  Copies have independent state and mass; deduplicate membership paths per
  occurrence and field. Primary conceptual LoD changes scale; secondary naming/
  compression stays 1:1. Storage depth and slot indices are not physical scale
  or coordinates. Potential, present and awake are distinct; sleepers remain
  occupants and contributors.
- **Admission:** use actual extents and sleeping occupants, canonical batch
  order, current combined focus and certified nearest-free placement. Tangency
  is allowed; missing geometry is rejected. New bodies start awake with zero
  motion. Validate quantized stored coordinates and the derived distance bound;
  exhaustion rejects without claims. D7 aggregate/parts exchange targets the
  outgoing logical centre, not study focus. Read D1–D7 rather than inferring
  bootstrap, restart or rebase behavior from this summary.
- **Transactions:** prepare old-plus-new peak capacity. Preserve survivor state,
  identity, dirty history, pending work and generation semantics. Ordinary
  rejection leaves the committed execution and last successful report unchanged;
  unexpected failure after live writes makes the execution unready. Do not
  uninstall/reseed the hierarchy or reset an owner to simulate an edit.
- **Data boundaries:** cold definitions → shared warm PostgreSQL toolbox →
  owner-local hot workbench → native execution. The cache manager rejects private
  namespaces before lookup; private inputs enter only the active owner boundary.
  Never reset/drop persistent stores or repopulate retained seeds as setup.
- **Observation:** snapshots read the actual active calculation stream, including
  GPU execution. A separate CPU replay is not a GPU snapshot. Keep captures,
  source/generation and completed tick identity coherent.

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

## Intended local study and release

The purpose is to observe mathematically derived organization and its changes
when new language material is added. Small letter universes need not crystallize
or reach exact stillness. Report motion and shapes separately from invariant
failures; do not inject language-family clustering as an expected answer.
Earlier max-tick observations do not establish a runtime timeout or large-scale
performance. More bodies/fields may amortize overhead; measure before claiming it.

Local development uses the native build and NAS PostgreSQL; secrets stay outside
Git. The release snapshot means a **ready-to-load-and-run Docker setup**:
application image(s), separate retained-fixture PostgreSQL service, commands,
notation and source/image/fixture identities. Prior model states are optional.
Choose CPU/GPU image separation during packaging. Use
`docker/snapshots/<date>-<source-revision>/` for release bundles with appropriate
large-artifact handling. A command-line operator or coding agent can drive it;
a new GUI is not required. The bounded letter-physics mini-release must state
supported operations and backends and does not imply completion of #156/#145.

## Leave the next instance a usable handoff

End each slice with the branch and exact source, accepted versus pending scope,
tests actually run, remaining findings, and the next action's owner. Update
[AGENTS current state](../../AGENTS.md#current-state) and the owning plan when
acceptance changes. Keep historical evidence pinned rather than rewriting it
to describe later results. No prior chat should be needed to continue.
