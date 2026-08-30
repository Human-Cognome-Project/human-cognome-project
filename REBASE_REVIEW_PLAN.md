# HCP Rebase Review Plan

**2026-08-30. Working instrument for the paradigm rebase.**

The physics basis of the project has been determined (see `field/` and `ledger/` packages). The entire repository — files, issues, notes, docs — is to be reviewed and replaced/rewritten on that basis. The goal of this phase is **not** implementation: it is to produce a project that others can read and understand, grounded in the new physics, with the old work archived honestly.

This document is the plan for that review. It will be updated as passes complete. Elements get discussed with Patrick as they come up; **nothing is moved, deleted, or closed until the pass it belongs to is signed off.**

## End state (the deliverables)

A repository whose front door reads, in order, for a newcomer:

1. **What this is** — an evolutionary, cosmological model of the history of human knowledge: movable, zoomable, placed on temporal coordinates, solved as target flows. (Successor to README/MANIFESTO/charter.)
2. **The physics basis** — the frequency/amount separation, the Mann ledger, field balancing, the exponent ladder; pointing into `field/` and `ledger/` as the primary sources.
3. **The architecture** — single-point force balancing as the sole operation; the structure of the space (nibble/byte particle layer, arrayed-pair addressing, flat pool + logical LoD chains, one-level composition, compression as balancing); aggregator singularities and the path-through principle; the DI on the emission side of the singularity.
4. **The data protocol** — no-filter intake with quality-first ordering; read-status/continuity/provenance flags; the precept (arrayed addresses, display form never persisted); what is pulled from the legacy DBs and how.
5. **The plan** — phased roadmap (Kaikki lattice first, then accretion), open problems carried from the physics packages, contribution paths.
6. **The archive** — the pre-rebase project, intact and reachable, with a disposition ledger explaining what each part was and why it was archived, rewritten, or carried forward.

## Governing principles for the review

- **Archive, don't destroy.** The old work is the raw-intake face of our own singularity. Nothing is deleted; the loss of any compression is declared. The disposition ledger is the path-through record.
- **No in-place repair.** Legacy DBs, code, and docs are read-only sources. New structure is built clean; content is *pulled* into it where it earns a place.
- **Pull-driven.** The new document set and schema enumerate what they need; the review pulls exactly that. Not-yet-pulled ≠ excluded.
- **Every artifact gets a disposition and a reason.** No silent drops.
- **Discuss the judgement calls.** Anything ambiguous between dispositions gets flagged for Patrick rather than decided unilaterally.

## Disposition categories

| code | meaning |
|---|---|
| **F** | Foundation — new-paradigm primary source; stays, possibly relocated (`field/`, `ledger/`). |
| **RW** | Rewrite — the artifact's *role* survives, its content is re-authored on the new basis (README, ROADMAP, architecture docs). |
| **AR** | Archive — old-paradigm design/engine/docs; moved intact to an archive area with a pointer to its successor concept where one exists. |
| **SRC** | Source — data holdings kept as read-only intake material (legacy DBs, corpora, kaikki-derived data). |
| **CF** | Carry forward — survives with minor adaptation because it is paradigm-neutral (licence, some infra, some tooling). |
| **DR** | Drop — generated artifacts, caches, dead weight with no design-history value. Explicit list, still recorded. |

## Review passes, in order

Each pass produces a **disposition note** (one file per pass, `review/pass-NN-<area>.md`): per-artifact table (path → disposition → reason → successor if any), plus flagged discussion items.

**Pass 1 — Root identity documents** (13 files: README, MANIFESTO, charter, covenant, ROADMAP, CONTRIBUTING, AGENTS, LICENSE, co.txt, invitation docx, configs).
The project's face. Mostly RW; LICENSE/covenant likely CF. Sets the outline of the new document set that every later pass pulls toward.

**Pass 2 — `docs/` tree** (69 files: numbered doc layers 00–07, entry-points, research, azsl-training-corpus, `_archive`).
The largest statement of the old paradigm (cognitive physics, Newtonian force library, AZSL). Expect mostly AR with successor pointers; research notes individually judged — some contain reasoning that feeds the new writing. Existing `docs/_archive` merges into the unified archive.

**Pass 3 — Engine and code** (`hcp-engine/` 226, `src/` 45, `relay/`, `tests/` 5, `benchmarks/`).
Old-paradigm implementation (resolution chambers, tokenizer, envelope manager, socket server, LMDB pipeline). Expect AR nearly throughout, with a **successor map** recorded where a concept re-lands (e.g. CPU marshaller/staging → chain bookkeeper; vocab beds → composition pool). Nothing is ported in this phase.

**Pass 4 — Data holdings** (`data/` 110, `db/` 93, `sources/`, `source_doc_pbm/`, plus live Postgres/LMDB stores off-repo).
SRC by default. Deliverable here doubles as the **data assessment**: per-store usability verdict for pull-driven extraction (clean / needs per-table rules / mashed), including the read-only dotted-blob round-trip probe on reference columns. The uncommitted `db/spacing_rules.sql` change gets a look here.

**Pass 5 — Tooling and infra** (`scripts/` 64, `tools/` 18, `infra/`, `.github/`, `resources/`).
Mixed CF/AR/DR. Python tooling judged against the standing rule (front-end/tooling use is legitimate; engine use is not).

**Pass 6 — GitHub issues** (51: 40 open, 11 closed).
Nearly all are old-paradigm implementation work (refactors of engine files, format builders, envelope wiring). Proposal: close en masse with a standard comment linking the rebase announcement doc; re-open only what has a new-paradigm successor (e.g. format builders re-emerge as intake singularity work; dedup issue #38 re-emerges as compression-as-balancing). A few are paradigm-neutral (contributor-facing) and get rewritten. Each issue's disposition recorded like any file.

**Pass 7 — Synthesis: write the new document set.**
With all dispositions known, author deliverables 1–5 above, restructure the repo (archive move executes here, after sign-off), and publish. The orchestrator graph review is queued behind this pass (separate effort, per session note — the graph is old-paradigm and is not consulted as authority during this review).

## Working mode

- I work pass by pass, in order; each pass ends with its disposition note and a discussion of flagged items before anything is executed.
- Passes 1–6 change nothing outside `review/`. All moves, closes, and rewrites happen in Pass 7 after sign-off.
- Interleaved discussion is expected — protocol/architecture decisions made mid-review get folded into the target document set as they land.

## Explicitly out of scope for this phase

- Implementation of the engine (Taichi/C++ evaluation included).
- DB migration/extraction (Pass 4 assesses; extraction happens when the new schema exists).
- The orchestrator graph supersession sweep (queued behind Pass 7).
