# HCP TODO — Master Reference

Current date: 2026-03-06. See area-specific TODOs for detailed task lists.

## Area TODOs

| Area | Location | Current Priority |
|------|----------|-----------------|
| **Engine** | [hcp-engine/TODO.md](hcp-engine/TODO.md) | Workstation packaging, variant engine wiring |
| **Database** | [db/TODO.md](db/TODO.md) | Marker PK fix, envelope activation |
| **Documentation** | [docs/TODO.md](docs/TODO.md) | Contributor guides, architecture overview |
| **Scripts** | [scripts/TODO.md](scripts/TODO.md) | SQLite export, LMDB verification |

## High Priority

- [ ] **Source Workstation packaging** — installer for multiple workstation configs. Viewer first, then data manipulation and ingestion station. Patrick is user #1.
- [ ] **Persist bond tables** as files in engine area (don't recompile from Postgres every startup)
- [ ] **Marker table PK collision** — control tokens share (t_p3, t_p4), needs t_p5 column
- [ ] **Document metadata pipeline** — HCPJsonInterpreter hasn't been exercised since kernel decomposition. Needs testing/rejig with Gutenberg JSON files.
- [ ] **Activity envelope implementation** — schema exists (migration 021). Engine-side activation/eviction in HCPEnvelopeManager needed.
- [ ] **Label propagation** — if word appears as Label anywhere, restore firstCap on all suppressed instances
- [ ] **Known initialisms** — U.S., U.K., M.D. etc. as curated list
- [ ] **Entity cross-ref OR bug** — [GitHub #23](https://github.com/Human-Cognome-Project/human-cognome-project/issues/23). ANY token match instead of ALL.

## Contributor / As Available

- [ ] **Archaic/casual/dialect variant expansion** — extend the 4,815 annotated forms, classify new variants from ingested texts. Build on V-1/V-3 rule patterns.
- [ ] **Secondary character registration** — only primary characters were cataloged. Supporting/minor characters in available texts need entity entries.
- [ ] **Dramatis personae** — compose character lists per work from entity data
- [ ] **Title cleanup** — standardize work titles across entity DBs
- [ ] **Multiple edition deltas** — detect differences between editions of the same work
- [ ] **Same text detection** — deduplication across different sources/editions
- [ ] **Document metadata connections** — link Gutenberg JSON metadata (author, date, subjects) to ingested documents
- [ ] Vocabulary expansion guide (how to identify and define tokens for new formats/domains)
- [ ] Language shard creation guide (vocabulary + sub-categorization patterns + force constants)
- [ ] Entity cataloging guide (expand librarian pipeline)
- [ ] **Format builders**: PDF ([#14](https://github.com/Human-Cognome-Project/human-cognome-project/issues/14)), EPUB ([#15](https://github.com/Human-Cognome-Project/human-cognome-project/issues/15)), HTML ([#16](https://github.com/Human-Cognome-Project/human-cognome-project/issues/16)), Markdown ([#17](https://github.com/Human-Cognome-Project/human-cognome-project/issues/17)), Wikipedia dump ([#18](https://github.com/Human-Cognome-Project/human-cognome-project/issues/18))

## Infrastructure

- [ ] **LMDB purge process** — background Python, monitors size, evicts cold entries
- [ ] SQLite vocabulary backend (portable alternative to Postgres for standalone tools)
- [ ] CI/CD pipeline for engine builds
- [ ] **Service file binary mismatch** — [GitHub #25](https://github.com/Human-Cognome-Project/human-cognome-project/issues/25)
- [ ] **Workstation SIGTERM handling** — UI ignores SIGTERM, needs SIGKILL. Fix shutdown.

## Backlog

- [ ] B's team fork review (42+ commits behind)
- [ ] Force profile junction table (deferred until experimentation)
- [ ] Punctuation tokens in hcp_english (AB.AA.AA.AF.*) — created in error, belongs in core
- [ ] Discord bot development (on hold)
- [ ] Analysing Sentences textbook (rights unclear)
- [ ] Local model setup — Ollama for cross-vet (when hardware allows)
- [ ] Web address mini-language — Patrick will set up in core (language-independent)
- [ ] NAPIER/ToM architecture — NSM primes as superposition selectors

## Recently Completed

- [x] Dead code cleanup — 3,132 lines removed (8ecade1)
- [x] Boundary fixes — /, [, ], •, trailing ., numeric var tagging (8ecade1)
- [x] Variant form storage — migrations 023-024, 4,815 clean variants (a897569, 202a184)
- [x] Scene pipeline refactor — RunPipelinedCascade, 3 workspaces (c7b9cb6)
- [x] V-1/V-3 variant normalization — TryVariantNormalize engine wiring (d59c2fa)
- [x] Entity normalization — name cleanup, position reindexing, phantom refs (db-specialist 2026-03-05)
- [x] README.md rewrite — alpha status, benchmarks, what exists (ea42aae)
- [x] GitHub issues #22, #24 closed
- [x] Possessive morpheme splitting — handled via morph bit field
- [x] docs/status.md aligned with current state
