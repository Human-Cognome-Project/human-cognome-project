# Next-step readiness

Prepared by Codex (AI agent), 2026-09-11.
Authority: execution-plan-2026-09-11.md and Patrick's direct clarifications.

## Bounded next work

1. Inspect the existing A1 ledger records through a bounded read-only query and
   retain exact addresses, table context, order and reciprocal relationships.
   Use existing connection configuration without printing credentials; retain the
   handoff's read-only and timeout settings. Missing links remain missing evidence.
2. Trace relative addressing in existing code/data before choosing a representation.
   extraction/token_id.py supplies base-50 couplet arithmetic and dotted boundary
   conversion. It does not implement a relative delta storage codec or establish
   that the live database uses one. Preserve intended arrayed/delta structure;
   distinguish it from what the inspected implementation actually provides.
3. Draft the fixture's CPU working-set mapping: canonical references, ordered
   component occurrences, contextual force memberships, reverse traversal,
   aggregated sections and recovery references. Include explicit LoD levels and
   show where multiple storage levels become one compact SNode representation.
4. Trace native field loading and level traversal against that mapping. The
   inspected C API exposes runtime allocation, AOT module loading and kernel/graph
   lookup. The synthetic SNode probe constructs layouts and runs explicit kernels;
   it does not demonstrate automatic semantic LoD selection. Verify the actual
   mechanism required without reopening the intended architecture.
5. Select the smallest C++ smoke slice that proves retained runtime state and the
   structural mapping before implementing force/movement mathematics.

## Tests tied to the next artifact

Exact address recovery from the chosen relative representation; composition order
and repeated positions; preservation of context and every force relationship;
expanded reconstruction from aggregates; expected membership at each defined LoD.
Use the accepted fixture as the oracle. No live database mutation is needed.

## Deferred until needed

Background pre-cache thread, autonomous runner priorities, production rendering,
capacity redesign, and qualification on other hardware. Existing field experiment
reports and sibling engine changes remain untouched. Legacy patch remains unapplied.

This preparation changed documentation only. Source and interface inspection was
performed; production behavior, live database contents, and GPU numerics were not
revalidated in this step.
