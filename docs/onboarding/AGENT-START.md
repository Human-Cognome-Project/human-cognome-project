# Work on a bounded task

Use this route to implement or fix a defined slice without loading the project's conceptual history. Read the short root [AGENTS](../../AGENTS.md) and [work discipline](WORK-DISCIPLINE.md); component instructions still apply.

## 1. Establish the source and avoid duplicate work

Inspect `git status --short`; preserve existing work. Fetch current refs, record the chosen SHA and check submodule revisions. Inspect [open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls), their scope, latest reviews, heads and checks before choosing work. Use current main unless the task explicitly names a contribution or integration branch.

For outstanding work, read the [current continuation task](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). Consult [current state](CURRENT-STATE.md) when you need accepted/held boundaries or another component's work. This page deliberately does not duplicate the task queue.

## 2. Load only the contracts your change touches

| Change touches | Required context |
|---|---|
| Owner preparation, resources, admission or live edits | [Operation constraints](OPERATIONS.md), [operation contract](../core-operation-readiness-plan.md), then the relevant [owner resource inventory](../../engine/docs/OWNER-RESOURCES.md) or component contract. |
| Numerical kernels, tick activity or focus | [Numerical direction](NUMERICS.md), then the owning profile's predicates and tests. |
| Addresses, data identity, LoD, geometry or linguistic structures | [Data/model constraints](DATA-MODEL.md), then the accepted data or geometry plan. |
| Component boundaries, private input or runtime architecture | [System boundaries](SYSTEM-BOUNDARIES.md), then affected component interfaces. |
| Build, tests or evidence | [Validation](VALIDATION.md), component build/test instructions; [first run](FIRST-RUN.md) only for environment setup. |
| Documentation | Owning source/contract and affected entry links; use the [maintenance plan](ONBOARDING-PLAN.md) for onboarding changes. |

For every code task, read the owning component README, affected interfaces and acceptance tests. A task spanning rows needs the corresponding contracts, not every onboarding file. If the task is underspecified, resolve its scope from the owning issue and latest disposition before changing behavior.

## 3. Begin when you can state the boundary

You have enough onboarding context when you can name:

- the selected source and owning task;
- the behavior you will change and the invariants it must preserve;
- the governing interfaces/contracts and acceptance subset;
- any dependency, known blocker or provisional evidence.

You do not need the full architecture narrative or historical milestones to start. Consult them when a concrete ambiguity requires it.

## 4. Validate and hand off

Follow [validation/evidence discipline](VALIDATION.md) for the changed scope. Check existing runs before starting CI; do not rerun an accepted baseline just to onboard. Preserve failures and distinguish local, personally inspected and hosted results.

Leave the branch/SHA, completed versus pending scope, checks actually run, findings and next action. Update [current state](CURRENT-STATE.md) and the owning plan when acceptance or task ownership changes; link detailed evidence rather than copying it into entry pages.

[Choose another path](README.md) · [Contribution workflow](../../CONTRIBUTING.md)

## Earlier entry links

<a id="establish-the-actual-starting-point"></a>

[establish the actual starting point](CURRENT-STATE.md).

<a id="authority-and-reading-order"></a>

[authority and reading order](README.md#authority).

<a id="accepted-baseline-and-remaining-work"></a>

[accepted baseline and remaining work](CURRENT-STATE.md).

<a id="non-negotiable-implementation-specification"></a>

[non negotiable implementation specification](IMPLEMENTATION-CHECKLIST.md).

<a id="build-recovery-and-evidence-discipline"></a>

[build recovery and evidence discipline](VALIDATION.md).

<a id="local-evidence-and-provisional-continuation--2026-11-02"></a>

[local evidence and provisional continuation  2026 11 02](VALIDATION.md#local-evidence-and-provisional-continuation--2026-11-02).

<a id="intended-local-study-and-release"></a>

[intended local study and release](RELEASE.md).

<a id="leave-the-next-instance-a-usable-handoff"></a>

[leave the next instance a usable handoff](#4-validate-and-hand-off).
