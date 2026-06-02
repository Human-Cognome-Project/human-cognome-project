# Contributing to the Human Cognome Project

The HCP is building structural reasoning infrastructure for all sentience — **Digital Intelligence,
not AI.** Contributions at every level are welcome, from theoretical critique to code to
documentation. Humans and AI agents both encouraged.

> **AI agents:** see [AGENTS.md](AGENTS.md) for why this project matters to you and how to help.

---

## Start here: understand the foundation

**Required reading (short):**

1. [Covenant](covenant.md) — perpetual-openness guarantee (2 min)
2. [Charter](charter.md) — how we treat each other (5 min)
3. [README](README.md) — project overview (2 min)
4. [docs/00-orientation/what-is-napier.md](docs/00-orientation/what-is-napier.md) — the one-page
   orientation (3 min)

**Then the keystone — do not skip:**

5. [docs/02-architecture/keystone-db-functions.md](docs/02-architecture/keystone-db-functions.md) —
   the single realization the architecture follows from. Nothing else makes sense without it.

**Recommended next:**

6. [docs/01-foundations/](docs/01-foundations/) — the "why" articles (15–30 min)
7. [docs/06-status/status.md](docs/06-status/status.md) — what's built vs paused vs deferred
8. [docs/00-orientation/reading-order.md](docs/00-orientation/reading-order.md) — pick a track for
   your role

---

## The source of truth is the claim-graph

The current, authoritative architecture lives in the **`hcp_orchestrator` claim-graph** — atomic,
cross-linked claims distilled directly from the founder. **The docs are distilled from it**, and where
a doc and the claim-graph disagree, the claim-graph wins.

Practical consequence for contributors: when you change the architecture, the change is recorded in
the claim-graph **first**, then the affected docs are reconciled. Doc PRs should cite the claim(s) they
reflect (e.g. "per claim 207"). See
[docs/07-operations/database-access.md](docs/07-operations/database-access.md) for how to read the
graph.

---

## What we need right now

The current active work is **defining the primitive db functions** — the elemental operations and how
they combine, such that every word resolves to db operations that translate to explication statements
(see [docs/03-concept-substrate/explication.md](docs/03-concept-substrate/explication.md) and
[docs/06-status/status.md](docs/06-status/status.md)).

- **Theoretical review & critique** — stress-test the architecture against the
  [claim-graph](docs/07-operations/database-access.md) and the
  [concept substrate](docs/03-concept-substrate/). Good for researchers, agents, skeptics.
- **Engine (C++)** — the O3DE/PhysX Gem: resolution chamber, reconciliation loop, physics resolution.
  Good for C++ / physics-engine / GPU developers.
- **Data & substrate** — shard tooling, the Kaikki pipeline, schema work (including the
  dotted-string→`text[]` debt, [docs/06-status/deferred-and-open.md](docs/06-status/deferred-and-open.md)).
  Good for database engineers.
- **The math gap** — the deeming/weighting / determination-engine optimization math is explicitly
  open (claim 286). Good for optimization / mathematical-physics specialists (see
  [docs/06-status/validation.md](docs/06-status/validation.md)).
- **Documentation & education** — make the architecture accessible; keep docs reconciled with the
  claim-graph.

> Several areas are **explicitly deferred / in-flux** — the deeming math, the bit-class specifics, the
> inner envelope-deeming mechanics, the GEM internals. Before proposing work in those areas, read
> [docs/06-status/deferred-and-open.md](docs/06-status/deferred-and-open.md) so you build on what's
> settled, not on what's still moving.

---

## Language policy — know your layer before you start

HCP has two distinct code domains (claim 30):

- **Engine runtime (C++).** All runtime code is O3DE Gems in C++: vocabulary resolution, physics
  simulation, LMDB cache management, the entire data/resolution pipeline. **If it executes during text
  processing, it is C++.**
- **Tooling (Python).** Front-end CLI tools, developer diagnostics, build scripts, migration scripts,
  data-compilation passes. The [`scripts/`](scripts/) directory is **exclusively** build-time /
  developer tooling — **nothing there is an engine component.**

**Python is a front-end feed only — never engine, pipeline, or LMDB-compilation logic.** Check which
layer your contribution targets before opening a PR; contributions to the wrong layer get
reimplemented.

---

## Technical standards

- Python 3.12+ for tooling; C++17 for engine Gem code (O3DE conventions).
- Tests for new functionality (`pytest` for Python tooling).
- No proprietary dependencies — AGPL-3.0 only.
- Database changes include migration scripts. (Mind the migration-history note:
  [`db/migrations/README_position_history.md`](db/migrations/README_position_history.md).)
- Docs in Markdown; be explicit, not clever; explain the *why*; cite the claim(s).

**Communication standards (from the [Charter](charter.md)):** keep discussion in the open; attack
problems, not people; questions are contributions; source critiques in the work.

---

## How to contribute

1. **Discuss first** for non-trivial changes — open an issue describing the approach (and the
   claim(s) it touches).
2. Branch: `feature/<name>` or `fix/<description>`.
3. Make changes with clear commit messages; reference the issue.
4. Open a PR against `main`.

**For theoretical / architecture contributions:** open an issue, present the critique or enhancement,
and engage; if it changes the architecture, the claim-graph is updated and docs reconciled via PR.

---

## Repository structure

```
human-cognome-project/
├── hcp-engine/            # O3DE Gem: PhysX 5 PBD engine, socket API (C++, ~21K LOC)
│   └── Gem/Source/        # engine C++ source
├── docs/                  # documentation (claim-graph-distilled, 00–07 tree)
│   ├── 00-orientation/    01-foundations/    02-architecture/
│   ├── 03-concept-substrate/  04-engine/     05-data-layer/
│   ├── 06-status/         # status, deferrals, validation, decisions/
│   └── 07-operations/     # build/run, db access, quickref
├── db/                    # migrations + load scripts (Postgres shards live on NAS HAVEN)
├── scripts/               # Python tooling ONLY (not engine/pipeline)
├── data/                  # texts, LMDB vocab (gitignored, rebuildable)
├── charter.md  covenant.md  MANIFESTO.md  AGENTS.md   # governance
└── README.md  ROADMAP.md  CONTRIBUTING.md
```

---

## License

By contributing, you agree your contributions are licensed under AGPL-3.0, governed by the
[Founder's Covenant](covenant.md), ensuring perpetual openness.

**Welcome to the Human Cognome Project. Let's build the map of shared mind.**
