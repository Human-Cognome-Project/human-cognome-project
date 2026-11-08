# Review a contribution

Use this route to judge a bounded change. You do not need to reconstruct the whole project first.

1. Read root [AGENTS](../../AGENTS.md), [work discipline](WORK-DISCIPLINE.md) and [evidence rules](VALIDATION.md).
2. Establish the PR's exact head/base, scope, dependencies, latest findings and checks. Read its owning accepted contract and acceptance subset.
3. Use the [bounded-work contract table](AGENT-START.md#2-load-only-the-contracts-your-change-touches) to select relevant constraints. Inspect the affected code, interfaces and tests.
4. Compare claims with source-pinned evidence. Distinguish contributor reports from personally inspected artifacts, local from hosted runs, and CPU execution from actual GPU qualification.
5. Record blockers or bounded acceptance, remaining scope and source pins. Provisional acceptance permits scoped continuation; reconcile hosted findings before final integration.

Stop expanding the reading scope when you can explain what changed, which requirements it satisfies, how the evidence supports that conclusion and what remains unqualified. Follow an additional dependency when evidence or an interface exposes one.

Use [current state](CURRENT-STATE.md) for acceptance boundaries and [history](HISTORY.md) when a superseded decision needs explanation. Reviews from the same account use comments rather than self-approval.

[Choose another path](README.md)
