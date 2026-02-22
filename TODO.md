# HCP TODO — Master Reference

Current date: 2026-02-22. See area-specific TODOs for detailed task lists.

## Area TODOs

| Area | Location | Current Priority |
|------|----------|-----------------|
| **Engine** | [hcp-engine/TODO.md](hcp-engine/TODO.md) | Source workstation (Phase 1) |
| **Database** | [db/TODO.md](db/TODO.md) | Boilerplate loading, marker PK fix |
| **Python tools** | [src/hcp/TODO.md](src/hcp/TODO.md) | Reference tooling, ingestion scripts |

## Cross-Cutting Tasks

### High Priority
- [ ] **Persist bond tables** as files in engine area (don't recompile from Postgres every startup)
- [ ] **Marker table PK collision** — control tokens share (t_p3, t_p4), needs t_p5 column
- [ ] **Purge "7 force types" from docs** — OpenMM artifact, we define our own forces
- [ ] **Update docs/status.md** — stale (Feb 5), doesn't reflect engine direction or recent work
- [ ] **Update docs/roadmap.md** — open questions section answered (physics engine = O3DE + PhysX 5)

### Project Management
- [ ] **GitHub issues** — create from TODO lists, labels by domain, milestones by phase
- [ ] **GitHub labels** — engine, db, linguistics, infra, librarian, good-first-issue, agent-suitable
- [ ] **Root AGENTS.md** — update "What doesn't exist yet" section (some things now exist)

### Documentation Drift
- [ ] Review all docs/research/ for stale OpenMM/Godot references
- [ ] Review docs/spec/ for consistency with current architecture
- [ ] Align docs/status.md with actual current state

### Contributor Onboarding
- [ ] Format builder template and guide (PDF, EPUB, HTML, Markdown, Wikipedia)
- [ ] Language shard creation guide (for other languages)
- [ ] Entity cataloging guide (expand librarian pipeline)

### Infrastructure
- [ ] **OpenClaw evaluation** — wingman skill for automated tmux orchestration
- [ ] **Local model setup** — Ollama on Patrick's machine for cross-vet and alternate view
- [ ] SQLite vocabulary backend (portable alternative to Postgres)
- [ ] CI/CD pipeline for engine builds

### Backlog
- [ ] B's team fork review (42+ commits behind)
- [ ] Force profile junction table (deferred until experimentation)
- [ ] Punctuation tokens in hcp_english (AB.AA.AA.AF.*) — created in error, belongs in core
- [ ] Discord bot development (on hold)
- [ ] Analysing Sentences textbook (rights unclear)
