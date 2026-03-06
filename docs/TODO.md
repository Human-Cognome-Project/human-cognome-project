# Documentation — TODO

Last updated: 2026-03-06

---

## Needs Writing

- [ ] **Contributor setup guide** — SDK install, project build, Postgres setup, LMDB compilation steps. Critical for onboarding.
- [ ] **Architecture overview** — Current two-phase pipeline (char PBD → vocab PBD), LMDB data flow, socket API. Replace stale architecture.md references.
- [ ] **Label phase design doc** — Label tier 0 broadphase, label propagation rules. Design exists in memory, needs doc.

## Needs Updating

- [ ] **Stale research docs** — Several docs in `docs/research/` reference outdated approaches (force patterns, sub-categorization). Review and either update or mark as historical.
- [ ] **DB specialist consultation** — `docs/db-specialist-consultation.md` has 17 questions. Some answered in `docs/engine-to-db-feedback-response.md`. Consolidate or close resolved questions.

## Cleanup

- [ ] **Purge "7 force types" references** — research docs and specs still contain stale force-type references.

---

## Completed (recent)

- [x] README.md rewrite — alpha status, benchmarks, what exists (ea42aae)
- [x] status.md updated to 2026-03-06 with all session work (df1f87b, ea42aae)
- [x] roadmap.md updated with pipelined benchmarks (df1f87b)
- [x] Scene pipeline plan — docs/scene-pipeline-plan.md (a4937e7), marked COMPLETED
- [x] Variant rules proposal — docs/variant-rules-proposal.md (a4937e7)
- [x] Variant forms audit — docs/variant-forms-audit-2026-03-04.md (a897569)
- [x] Engine quickref — docs/engine-quickref.md (a4937e7)
- [x] 6 superseded research docs deleted: OpenMM x3, Taichi, Warp, LMDB assembly (27151b6)
- [x] LMDB design specs added (27151b6)
- [x] DB consultation docs added (27151b6)
