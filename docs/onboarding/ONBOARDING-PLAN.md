# Layered onboarding plan and maintenance contract

Authorized by Patrick on 2026-11-08 while implementation work continues separately. This supersedes the earlier deferral of broad onboarding factoring until after the engine PoC. Engine responsibility factoring remains its own work under #229.

## Goal

Let humans and agents choose an immediate bounded path or a high-level understanding path. Separate files reduce the context loaded for each task; the full design and development history remain reachable. A small index describes a destination before the reader opens it.

## Implementation sequence

1. Inventory the existing entry points, universal constraints, live status, task-specific contracts and historical references.
2. Keep root AGENTS short: universal contribution rules and links. Move detailed constraints into focused files without changing their accepted meaning.
3. Give bounded work, understanding, review, first run and history separate entries, with explicit exit conditions and deeper links.
4. Maintain current acceptance in CURRENT-STATE and the next action in its owning continuation plan. Entry pages link rather than reproduce the queue.
5. Preserve old entry anchors as forwarding links. Preserve source-pinned evidence, failed/cancelled attempts and Git ancestry.
6. Verify moved sections, links, navigation paths and the absence of runtime changes. Land independently of the coding agent's implementation branch.

Steps 1–5 are implemented by this documentation change. Step 6 is recorded in the PR validation; final integration must recheck current main for intervening task/status updates.

## File responsibilities

| File | Owns | Load when |
|---|---|---|
| Root AGENTS | Universal contribution rules and compatibility links | Contributing |
| Onboarding README | Intent-based route selection | Choosing a path |
| AGENT-START | Bounded task workflow and contract selection | Coding or fixing |
| UNDERSTAND | Model and component overview | Understanding the project |
| REVIEW | Review workflow and exit criteria | Reviewing |
| FIRST-RUN | Matched native setup and named smoke check | Recovering/building an environment |
| HISTORY | Provenance navigation | Investigating the journey |
| CURRENT-STATE | Current acceptance, held scope and work ownership | Selecting work or checking status |
| NUMERICS / OPERATIONS / DATA-MODEL / SYSTEM-BOUNDARIES | Scoped constraints and links to owning contracts | Touching the named concern |
| WORK-DISCIPLINE / VALIDATION | Change and evidence rules | Editing or validating |
| RELEASE | Study and packaging intent | Preparing observation/release |
| IMPLEMENTATION-CHECKLIST | Preserved cross-component specification | Checking a change spanning multiple boundaries |

## Acceptance checks

- A bounded contributor can reach the task and its applicable contracts without loading UNDERSTAND or historical records.
- A conceptual reader can understand component relationships without reading the PR queue, build recipe or precise code stage.
- A reviewer can reach the governing contract and evidence rules directly.
- A historical reader can trace decisions and failures without treating them as today's instructions.
- Every former AGENTS section remains represented by a scoped file or explicit navigation replacement; old anchors still lead somewhere useful.
- Relative links in changed files resolve; moved internal headings remain reachable.
- Only documentation changes. No runtime tests or duplicate accepted-baseline CI are required for this restructuring.
- Analyst language preserves conceptual design while distinguishing deferred implementation/calibration.

## Keep the layers small

Add a contract to the appropriate scoped file, not to root AGENTS. A route states its audience, required context, deeper destinations and stopping point. Avoid umbrella instructions to read every linked file. If a file acquires unrelated responsibilities, split it and leave a short index.

Keep one status summary and link it from other entries. Detailed measurements stay in their owning evidence records; accepted decisions stay in their contracts. Update a plan's next task when work changes, not a copy in every onboarding page.

## Concurrent coding and adoption

This is a documentation-only branch from the accepted main baseline. The coding agent can finish its current bounded slice using its pinned instructions. On integration, retain any newer code, evidence and status updates, then direct subsequent sessions through the new entry paths. Do not reset the coding agent's branch or require an accepted-baseline rerun to adopt these docs.

The migration changes how context is loaded, not the engine architecture, acceptance scope or historical record.
