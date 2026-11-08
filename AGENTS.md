# Agent guidance

Read this short file in full before contributing. Then follow [bounded work](docs/onboarding/AGENT-START.md) or [review](docs/onboarding/REVIEW.md). Humans and agents can choose conceptual, operational or historical paths at the [onboarding entry](docs/onboarding/README.md); those paths are not cumulative prerequisites.

## Rules for every contribution

- Start from current main unless a task names another branch. Inspect current PRs and preserve uncommitted work; avoid duplicating active contributions.
- Read [work discipline](docs/onboarding/WORK-DISCIPLINE.md), applicable component instructions and the owning accepted contract before editing. Follow the bounded-work table to load additional contracts only when your task touches them.
- Code and executable evidence establish what is built; accepted decisions govern required behavior. Report conflicts. Preserve built, planned, provisional, held and experimental distinctions.
- HCP operational components are native C++. Project Python is limited to early bulk-data bootstrap preparation/population/validation; upstream tooling is a separate boundary.
- Preserve component ownership and the single native execution authority. Private inputs enter only the active owner boundary; cache managers reject private namespaces before lookup. Read [system boundaries](docs/onboarding/SYSTEM-BOUNDARIES.md) when changing these interfaces.
- Preserve persistent data, credentials and Git history. No routine reset/drop/truncate of retained stores, secrets in Git or history rewriting for cleanup.
- Validate the changed scope and preserve source-pinned evidence. Do not duplicate running CI or rerun an accepted baseline merely to onboard. A local or CPU pass qualifies only that scope.
- Read the [Covenant](covenant.md), [Charter](charter.md) and [contribution workflow](CONTRIBUTING.md) before contributing.

## Current work and deeper contracts

The links below preserve established entry anchors while their content lives in smaller files. They are a navigation map, not a requirement to load every destination. Status updates belong in CURRENT-STATE and the owning plan; do not rebuild a second queue here.

<a id="current-state"></a>

**Current state:** [Read the scoped page](docs/onboarding/CURRENT-STATE.md#current-state).

<a id="active-and-deferred-work"></a>

**Active and deferred work:** [Read the scoped page](docs/onboarding/CURRENT-STATE.md#active-and-deferred-work).

<a id="latest-model-direction--2026-10-23"></a>

**Latest model direction — 2026-10-23:** [Read the scoped page](docs/onboarding/NUMERICS.md#latest-model-direction--2026-10-23).

<a id="latest-cumulative-publication-direction--2026-10-26"></a>

**Latest cumulative-publication direction — 2026-10-26:** [Read the scoped page](docs/onboarding/NUMERICS.md#latest-cumulative-publication-direction--2026-10-26).

<a id="required-placement-and-allocation-behavior"></a>

**Required placement and allocation behavior:** [Read the scoped page](docs/onboarding/OPERATIONS.md#required-placement-and-allocation-behavior).

<a id="activity-focus-and-privacy"></a>

**Activity, focus and privacy:** [Read the scoped page](docs/onboarding/NUMERICS.md#activity-focus-and-privacy).

<a id="addressing"></a>

**Addressing:** [Read the scoped page](docs/onboarding/DATA-MODEL.md#addressing).

<a id="architecture-invariants"></a>

**Architecture invariants:** [Read the scoped page](docs/onboarding/SYSTEM-BOUNDARIES.md#architecture-invariants).

<a id="do-not-reintroduce"></a>

**Do not reintroduce:** [Read the scoped page](docs/onboarding/SYSTEM-BOUNDARIES.md#do-not-reintroduce).

<a id="model-contract"></a>

**Model contract:** [Read the scoped page](docs/onboarding/DATA-MODEL.md#model-contract).

<a id="reading-paths"></a>

**Reading paths:** [Read the scoped page](docs/onboarding/README.md#reading-paths).

<a id="local-evidence-and-provisional-continuation--2026-11-02"></a>

**Local evidence and provisional continuation — 2026-11-02:** [Read the scoped page](docs/onboarding/VALIDATION.md#local-evidence-and-provisional-continuation--2026-11-02).

<a id="work-discipline"></a>

**Work discipline:** [Read the scoped page](docs/onboarding/WORK-DISCIPLINE.md#work-discipline).

<a id="contribution-flow"></a>

**Contribution flow:** [Read the scoped page](docs/onboarding/WORK-DISCIPLINE.md#contribution-flow).

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>
<a id="current-development-state"></a>

Earlier status anchors: [current work](docs/onboarding/CURRENT-STATE.md).

<a id="placement-correction--2026-10-20"></a>
<a id="allocation-foundation-correction--2026-10-20"></a>

Placement/allocation corrections: [operation constraints](docs/onboarding/OPERATIONS.md).

<a id="current-model-realignment-2026-10-09"></a>

Model realignment: [data/model contract](docs/onboarding/DATA-MODEL.md#model-contract).
