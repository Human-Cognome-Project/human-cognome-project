# Documentation reconciliation — 2026-11-01

Baseline: main `939632dedb9df0234d7e828e5c0555313ee14cec` (#201).
Purpose: let a fresh coding instance align without prior chat or the previous
machine's build state. [Start here](onboarding/AGENT-START.md).

## Scope and findings

The audit inventoried 142 project Markdown documents outside archived research,
per-run evidence and the upstream Taichi tree, scanned their status/instruction
claims and relative links, and reconciled the active entry points and affected
contracts. Fork provenance/configuration and its two old session entry points
were checked separately. The [documentation map](documentation-map.md) identifies
the current references and preserved collections. This is a documentation/status
audit, not a new source review of every component or a re-execution of all tests.

| Finding | Correction |
|---|---|
| Fresh agents needed chat history to choose work | Added onboarding/spec brief with accepted baseline, invariant summary, authority order, setup/recovery, existing-PR handling and release intent. |
| AGENTS began with many obsolete “next action” notices | Moved dated acceptance notices to linked history; current state and active work now lead. Original pins and evidence limits remain available. |
| Index and hierarchy plan called accepted C3 work unbuilt | Reconciled #195/#196/#198 acceptance and #201 CPU B1 across entry points. |
| Continuation and #199 plan instructed agents to start B1 again | Marked B1 complete, draft #202 as existing B2 work, B3 and owner/control integration pending. |
| Older component references still said “for review” | Corrected geometry #175, updates #169 (not #184), prepared foundation closure #162, warm v2 #171, activity #153 and numerical #192/#193 status. |
| Legacy numerical prose could override current wide-profile rules | Added explicit profile/successor pointers; preserved pinned historical runs and accepted #196 motion. |
| Database/fork handoffs looked like new-session instructions | Added historical banners and current onboarding links; no seed reruns or old workstation assumptions. |
| CPU build recipe omitted CI's explicit disabled options | Aligned the CPU fork example with workflow DX11/DX12/backtrace/RHI-example OFF flags. |
| Recovered data maps linked to pre-reorganization paths | Repointed those references to their preserved August archive locations. Removed fake link targets from the agent issue template. |

## Current work boundary

At inspection, #202 is open and draft at
`1214e9a0265eb693e4d605add7c496f16e124d36`; its author reports outstanding full
CPU/compiler/PostgreSQL qualification. That report is not hosted evidence or
maintainer acceptance. This audit does not review or approve its implementation.
A replacement agent should inspect and continue the existing contribution;
maintainer review follows its ready source/evidence. Recheck live state first.

## Validation and preservation

- Checked local Markdown destinations against the pinned recursive repository
  tree plus added files, and heading fragments where target Markdown was in the
  audit set. External URLs were not exhaustively crawled.
- Checked the named smoke target and optional-client defaults against
  `engine/CMakeLists.txt`, and the CPU configuration against the native workflow.
  No fresh compilation or engine acceptance run was performed for this docs change.
- Checked the documentation diff for whitespace errors and kept changes confined
  to Markdown. No workflow dispatch, CI rerun, code, database, model-state or
  open-contribution mutation is part of this cleanup.
- Preserved archives, research derivations, source-pinned test artifacts,
  checksums and snapshot manifests. Old failed/cancelled results remain historical
  failures/cancellations; later acceptance does not rewrite them.

For future changes, update the maintained status and owning specification at
acceptance, and let other entry points link there. Preserve dated historical
meaning while removing stale instructions from the current work path.
