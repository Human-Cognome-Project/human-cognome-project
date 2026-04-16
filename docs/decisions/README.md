# Design Decisions

This directory records significant design decisions for the HCP. Each decision gets its own file.

## Index

| ID | Title | Date | Status |
|----|-------|------|--------|
| 001 | Token ID decomposition | 2026-02-12 | Implemented. Note: references old `hcp_names` shard (retired). |
| 002 | Names shard elimination | 2026-02-12 | Implemented. `hcp_names` merged into `hcp_english` as Labels (AD namespace). |
| 005 | Decompose all token references | 2026-02-12 | Implemented. Note: references old `tokens` table (now `entries`). |

## Template

When adding a decision, create a file named `NNNN-short-title.md`:

```markdown
# NNNN: Short Title

**Date:** YYYY-MM-DD
**Status:** Proposed | Accepted | Superseded by NNNN

## Context

What is the issue? Why does a decision need to be made?

## Decision

What was decided.

## Consequences

What follows from this decision -- both positive and negative.
```
