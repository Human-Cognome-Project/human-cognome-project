# Pass 6 — GitHub issues (51: 40 open, 11 closed)

**2026-08-30. Dispositions per REBASE_REVIEW_PLAN.md. No issue is closed/edited until Pass 7.**

Mechanism at Pass 7: a **rebase announcement issue** is opened first (linking the new front-door doc and this review); every close below uses a standard comment pointing at it. Closed issues (3, 4, 6, 7, 11, 12, 13, 20, 22, 24, 39) need no action beyond being covered by the announcement's context.

## Dispositions

**Close with pointer — old-paradigm implementation, no successor** (the machinery they serve is archived):
5 (bond tables), 8 (marker-table PK — old schema), 9 (boilerplate forward walk), 10 (LMDB purge), 19 (SQLite vocab backend), 23 (entity cross-ref bug), 25 (systemd binary name — service retired), 26 (envelope wiring), 27 (env_* variants), 28 (label propagation), 29 (initialisms), 30 (Workstation SIGTERM), 46 (bed packing), 47 (batch resolve), 48 (packer wiring), 49–52 (refactors of retired files), 53 (MorphBits cleanup), 54 (SQLite schema sync), 55 (possessive handling — design pass on retired resolver).

**Close with pointer + successor opened at synthesis** (the need survives, re-founded):

| old | successor (new-paradigm statement) |
|---|---|
| 14–17 (format builders: PDF/EPUB/HTML/Markdown) | Intake converters for aggregator emissions — byte-particle intake faces per format, provenance-path recorded |
| 18 (Wikipedia dump processor) | Wikipedia singularity intake: raw + refined faces, path-through characterized |
| 33 (variant expansion) | Lexical lattice accretion (variant forms as placed elements) |
| 34–35 (secondary characters, dramatis personae) | Entity placement in the field (works' internal populations as composition entries) |
| 36 (title cleanup) | Work-identity reconciliation = dedup-as-balancing over titles |
| 37 (edition deltas) | Flow solving between editions of one work (target-flow pair) |
| 38 (same-text detection) | Compression-as-balancing: duplicate identity = differential, merge removes it |
| 40 (LMDB verification) | Store-probe tooling (Pass 4 probe becomes standing verification) |
| 41 (benchmark regression) | Balancing-kernel benchmark harness (oracle-first discipline) |
| 31–32 (contributor setup / architecture overview) | Superseded outright by the Pass 7 document set — close pointing at the new docs rather than reopening |
| 42 (contributor guides) | New contribution-paths doc (deliverable 5) |

**Stay open (paradigm-neutral):**
- 21 (Seeking: human issue-tracker maintainer) — still true, arguably more so during the rebase.
- 43 (CI/CD) — survives with a retitle at synthesis (taichi_core/C++ era builds, not O3DE).

## Flags

1. The successor list above is a *menu*, not a commitment — which successors actually get opened at synthesis should match the rewritten roadmap's phase 1 (Kaikki lattice + first intake), not recreate a 40-issue backlog on day one. Recommend opening only: format-builder successors (as one umbrella intake issue), same-text/dedup, store-probe tooling, and the contribution-paths doc; the rest live in the roadmap until real.
2. Engine TODO.md items (packer wiring, SIGTERM, label propagation) duplicate #48/#30/#28 — the TODO files archive in Pass 7 and the issues close; no orphaned work.
3. Labels: the `contributor-path` / `agent-suitable` / `librarian` label taxonomy is good and survives; successor issues reuse it.
