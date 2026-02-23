# HCP TODO — Master Reference

Current date: 2026-02-23. See area-specific TODOs for detailed task lists.

## Area TODOs

| Area | Location | Current Priority |
|------|----------|-----------------|
| **Engine** | [hcp-engine/TODO.md](hcp-engine/TODO.md) | Source workstation, physics detection scene |
| **Database** | [db/TODO.md](db/TODO.md) | Marker PK fix, LMDB purge process |
| **Python tools** | [src/hcp/TODO.md](src/hcp/TODO.md) | Reference tooling, ingestion scripts |

## Cross-Cutting Tasks

### High Priority
- [ ] **Possessive morpheme splitting** — engine needs to split word failures on 's / s' and retry stem (tokens exist: AB.AB.CB.BF.At, AB.AB.CB.BF.Ay)
- [ ] **Persist bond tables** as files in engine area (don't recompile from Postgres every startup)
- [ ] **Marker table PK collision** — control tokens share (t_p3, t_p4), needs t_p5 column
- [ ] **Lossless round-trip proof** — Phase 1 key deliverable, validates entire pipeline

### Documentation
- [ ] **Root AGENTS.md** — update "What doesn't exist yet" section (much now exists)
- [ ] **Purge stale references from docs/** — OpenMM, Godot, "7 force types", old positional storage
- [ ] **Align docs/status.md** with current state (stale since Feb 5)
- [ ] **Update hcp-engine/ROADMAP.md** phases to match root ROADMAP (PBM stored, not derived)

### Contributor Onboarding
- [ ] Vocabulary expansion guide (how to identify and define tokens for new formats/domains)
- [ ] Language shard creation guide (vocabulary + sub-categorization patterns + force constants)
- [ ] Entity cataloging guide (expand librarian pipeline)

### Infrastructure
- [ ] **Context watcher script** — tested, needs refinement (scripts/context_watcher.sh)
- [ ] **LMDB purge process** — background Python, monitors size, evicts cold entries
- [ ] SQLite vocabulary backend (portable alternative to Postgres for standalone tools)
- [ ] CI/CD pipeline for engine builds

### Done (remove on next review)
- [x] GitHub issues created (#3-#21), labels and milestones set up
- [x] Root ROADMAP.md rewritten with current architecture
- [x] Context recovery protocol documented in shared.md

### Backlog
- [ ] B's team fork review (42+ commits behind)
- [ ] Force profile junction table (deferred until experimentation)
- [ ] Punctuation tokens in hcp_english (AB.AA.AA.AF.*) — created in error, belongs in core
- [ ] Discord bot development (on hold)
- [ ] Analysing Sentences textbook (rights unclear)
- [ ] Local model setup — Ollama for cross-vet (when hardware allows)
