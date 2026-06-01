# Documentation -- TODO

Last updated: 2026-04-16

---

## Needs Writing

- [ ] **Contributor setup guide** -- SDK install, project build, Postgres setup, LMDB compilation steps. Critical for onboarding. (Issue #31)
- [ ] **Architecture overview** -- Current two-phase pipeline (char PBD -> vocab PBD), LMDB data flow, socket API. Replace stale architecture.md references. (Issue #32)

## Needs Updating

- [ ] **Stale research docs** -- Several docs in `docs/research/` reference outdated approaches (force patterns, sub-categorization). Review and either update or mark as historical.
- [ ] **DB specialist consultation** -- `docs/db-specialist-consultation.md` has 17 questions. Some answered in `docs/engine-to-db-feedback-response.md`. Consolidate or close resolved questions.
- [ ] **README.md** -- References stale DB count (7 shards, now 11) and old benchmarks only. Should reference stress-test results.
- [ ] **spec/namespace-reference.md** -- Migration count and token counts stale.

## Cleanup

- [ ] **Purge "7 force types" references** -- research docs and specs still contain stale force-type references.

---

## Completed (recent)

- [x] status.md rewritten for 2026-04-16 project update (positions, storage model, stress test, deferred items)
- [x] engine-quickref.md updated (recording flow, key tables, entries not tokens, no morph-bit reconstruction)
- [x] data-pipeline-status-2026-04-07.md annotated as partially stale
- [x] Migrations 041 and 048 marked as superseded
- [x] README_position_history.md updated through migration 049
- [x] decisions/README.md index populated with status notes
- [x] roadmap.md current focus updated to NSM concept modeling
- [x] AGENTS.md and CONTRIBUTING.md updated for April 2026 state
- [x] Design docs (tokenization-plan, delta-audit-spec/handoff) annotated with completion/pause status
- [x] README.md rewrite -- alpha status, benchmarks, what exists (ea42aae)
- [x] Scene pipeline plan -- docs/scene-pipeline-plan.md (a4937e7), marked COMPLETED
- [x] Variant rules proposal -- docs/variant-rules-proposal.md (a4937e7)
- [x] Variant forms audit -- docs/variant-forms-audit-2026-03-04.md (a897569)
- [x] Engine quickref -- docs/engine-quickref.md (a4937e7)
- [x] 6 superseded research docs deleted: OpenMM x3, Taichi, Warp, LMDB assembly (27151b6)
- [x] LMDB design specs added (27151b6)
- [x] DB consultation docs added (27151b6)
