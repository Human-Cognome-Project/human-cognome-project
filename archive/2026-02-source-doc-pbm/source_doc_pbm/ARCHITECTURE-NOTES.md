# HCP Infrastructure Architecture Notes

_Working notes from design discussion, 2026-02-06_

## Two Instances, Two Scopes

- **Instance A (source_doc_pbm/):** GitHub Actions, CI/CD, data pipelines, public query infrastructure, WHC backend API
- **Instance B:** PBM chunking tool for users to process source documents into PBMs for submission

Both instances need to coordinate on: PBM submission format, API contract for token resolution, validation rules.

## Overall Architecture

```
GitHub repo (source of truth, AGPL)
├── GitHub Pages    → static query/browse UI (HTML/JS/CSS)
├── GitHub Actions  → CI, PBM validation, data transform, deploy
├── LFS dumps       → versioned DB snapshots (.sql)
└── source_doc_pbm/ → PBM submission staging
         ↓ (Actions transforms SQL dumps → MySQL format, pushes to WHC)
    WHC (whc.ca hosted — LAMP stack, MySQL + PHP)
    ├── Public query API (JSON endpoints)
    └── Token resolution backend for chunking tool
         ↑
    GitHub Pages frontend queries this
```

## Two Pipelines

### Outbound (data visibility)
```
Postgres (Patrick's workbench)
  → SQL dump → push to repo (LFS)
    → Actions: hydrate Postgres, export to MySQL format
      → Push to WHC
        → PHP API serves queries
          → GitHub Pages UI consumes API
```

### Inbound (PBM submission)
```
User runs chunking tool (offline or online mode)
  → Generates PBM
    → Submits PR to repo
      → Actions validates structure/format
        → Patrick reviews, pulls, ingests locally
          → ID assignment, TBD resolution, storage
            → Updated dump pushed back to repo
```

Early stage: Patrick curates manually, pulls ~daily. Automation increases as validation rules mature.

## Two Chunking Modes

The user-facing PBM generation tool has two backends for token resolution:

### Offline/Bulk Mode
- User downloads shard dumps from repo
- Chunking tool runs against local DB copy
- Fast, no network dependency
- For heavy users processing large volumes

### Online/Light Mode
- Chunking tool calls WHC API for token lookups
- Zero local setup required
- For casual users or small jobs

**Same PBM output format either way.** Both feed into the same submission pipeline.

## Technology Decisions

| Component | Tech | Reason |
|-----------|------|--------|
| Construction DB | PostgreSQL | Heavy analysis, writes, cross-shard assembly |
| Inference layer (future) | LMDB or engine-specific | Read-only, compiled from Postgres snapshots |
| Repo data | SQL dumps via LFS | Already working, versioned |
| Public API | PHP + MySQL on WHC | Available hosting, generous data allowances |
| Public UI | GitHub Pages | Free, version-controlled, PRs welcome |
| CI/CD | GitHub Actions | Native to repo, can hydrate Postgres from dumps |

## Key Constraints

- **Actions runner time:** 2,000 free min/month on public repos. English shard (685 MB, ~480 MB after prefix compression) takes time to load — be selective about which shards load per workflow.
- **WHC is LAMP:** PHP and MySQL. No Postgres on hosting. Actions handles the format translation.
- **CORS:** WHC API must allow requests from *.github.io domain.
- **Namespace authority:** ID assignment stays on Patrick's workbench. No distributed writes to namespace allocation.

## Open Items

- [ ] API contract between WHC backend and chunking tool (see api-design.md)
- [ ] PBM submission format spec (Instance B to define, Instance A to validate)
- [ ] GitHub Actions CI basics (Python linting, tests)
- [ ] Issue/PR templates for the repo
- [ ] MySQL schema design (subset of Postgres, read-optimized)
- [ ] GitHub Pages site scaffolding
- [ ] WHC deployment pipeline from Actions
