# Engine documentation status

For a new instance, read the [onboarding/spec brief](../../docs/onboarding/AGENT-START.md).
Current CPU contracts include [live hierarchy](LIVE-HIERARCHY.md),
[combined refresh](LIVE-REFRESH.md), [accepted motion](ACCEPTED-MOTION.md),
[native selection](NATIVE-SELECTION.md) and [contact-safe ticks](CONTACT-SAFE-TICKS.md).
Use [AGENTS current state](../../AGENTS.md#current-state) for acceptance and
[core continuation](../../docs/core-local-study-continuation-plan.md) for remaining work.
The recovered record below is not a complete current implementation inventory.

This directory was recovered on 2026-09-24 from the native C++ engine workspace
that had remained local while the repository was reorganized.

The record is intentionally preserved because it contains the model decisions,
vetting evidence, measured engine behaviour, drift corrections, and native
Taichi integration work that produced the current C++ implementation.

## How to read it now

Current repository paths supersede old workstation paths:

- historical `/opt/project/repo/engine` → repository `engine/`
- historical `/opt/project/taichi` → repository `engine/taichi/`
- historical `/opt/project/taichi/build-review` → a matching local build of
  `engine/taichi/` (normally `engine/taichi/build-review/`, ignored by Git)

Absolute paths inside recovered notes are provenance from the development
station, not required installation locations.

Likewise, statements such as "nothing here is committed", agent/model routing
instructions, or restrictions on what an orchestrator was allowed to inspect
describe the context in which the note was written. They are not current Git
or contribution instructions. Current contributor policy is in
`/AGENTS.md` and `/CONTRIBUTING.md`.

## Current realignment

The accepted 2026-10-10 object-support slice is implemented and tested:
[OBJECT-SUPPORT-RESULTS.md](OBJECT-SUPPORT-RESULTS.md). It includes proportional
centre response, supplied-geometry enclosure, ordinary contact and fixed-study
publication. The broader roadmap below still contains future work.

Start model/bridge changes with the [conversion and continuation plan](../../docs/model-realignment-plan.md)
(2026-10-09). It distinguishes conceptual primary LoD from attached 1:1
compression, records component-derived extent and sequences the response,
scale, whitespace and contact conversions. It supersedes conflicting historical
requirements without claiming the corresponding code has changed. Track
#106, #115, #116, #118, #119 and #120 alongside the plan.

## Document roles

- **[ASSEMBLY-BOOTSTRAP.md](ASSEMBLY-BOOTSTRAP.md)** — preserved encoding-floor
  translation guide with a current pointer to the built fixed-study slice.
- **[OBJECT-SUPPORT-RESULTS.md](OBJECT-SUPPORT-RESULTS.md)** — executed native
  object-support checks, fixture settings and payload migration boundaries.
- **[ACTIVE-FIELD-MODEL.md](ACTIVE-FIELD-MODEL.md)** — current integrated field
  and tick explanation, including #91's implemented selective-motion proposal
  and the open destination/brake questions. Read with the
  [NAPIER system guide](../../docs/napier-system-guide.md).
- **[EFFECTIVE-MOTION.md](EFFECTIVE-MOTION.md)** — CPU fixtures, reproduction,
  demand definitions and the review/integration boundary before local GPU tests.
- **OPERATIONAL-PLAN.md** — most complete consolidation of the native field
  model and build state as of 2026-09-14. Treat explicit RESOLVED/BUILT entries
  as design/build evidence, but check current code and later decisions before
  assuming unfinished/open status is still current.
- **HARNESS-NOTES.md** — literal development record preserving distinctions
  that were repeatedly lost through summarization. Valuable provenance; not a
  replacement for current repository instructions.
- **HARNESS-STRUCTURE.md** — provisional system-level harness decomposition.
  It predates the later repository distinction between the recovered
  `field::Harness` class and the wider analyst-facing engine harness.
- **DRIFT-AUDIT.md** — model-anchored audit of the native field implementation
  and corrections made during September development.
- **VETTING-PLAN.md** — evidence trail for the modified Taichi runtime,
  capacities, lifetime behaviour, and build/source consistency.
- **ENGINE-NOTES.md** — native C++ calling mechanics. Current path defaults are
  controlled by `engine/cmake/FindTaichiEngine.cmake`.
- **CONFIGURATION.md** — Taichi configuration behaviour and source evidence.
- **DEVICES.md** — measurements from the development hardware at that time.
  These are measurements, not performance guarantees.
- **api-reference/** — copied Taichi API reference sources used during vetting;
  consult the curated `engine/taichi/` fork for current source authority.

## Confidence boundary

The native implementation has performed viable calculations and recorded
substantial CPU/CUDA functional validation. The complete recovered stack is
still only partially vetted. Performance and scalability under realistic HCP
load remain empirical work.

Python is not part of the HCP engine/data runtime path. Python contained in the
Taichi fork is upstream frontend/build/test material unless a specific build
step proves it is required.

