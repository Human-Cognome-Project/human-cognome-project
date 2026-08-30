# Contributing to the Human Cognome Project

Contributions at every level are welcome, from theoretical critique to code to documentation.
Humans and AI agents both encouraged (**agents: see [AGENTS.md](AGENTS.md)**).

## Start here

1. [Covenant](covenant.md) — perpetual-openness guarantee (2 min)
2. [Charter](charter.md) — how we treat each other (5 min)
3. [README](README.md) — what this is (3 min)
4. [docs/physics-basis.md](docs/physics-basis.md) — the basis everything follows from. **Do not
   skip**; nothing else makes sense without it. Then the primary sources in
   [ledger/](ledger/) and [field/](field/) as deep as you care to go.
5. [docs/architecture.md](docs/architecture.md) and [docs/data-protocol.md](docs/data-protocol.md).
6. [ROADMAP.md](ROADMAP.md) — where work is, in dependency order.

The repository's documents are the contributor-facing authority. (The historical claim-graph is
under review against the new paradigm; do not build on it.)

## What we need right now

- **Physics.** The open problems are listed in [docs/physics-basis.md](docs/physics-basis.md):
  ladder coefficients first (Lamoreaux's five-percent plate number is the stated target). Good for
  optimization and mathematical-physics people.
- **Theoretical critique.** Stress-test the basis and the architecture. Skeptics are contributors;
  questions reveal the map's uncharted edges.
- **Schema and extraction** (Roadmap phase 1): the clean substrate and the pull from the old
  stores — see [docs/data-protocol.md](docs/data-protocol.md) and [extraction/](extraction/).
  Good for database engineers.
- **Engine groundwork** (Roadmap phase 4): a `taichi_core` research corpus built the way the
  archived AZSL corpus was, and oracle-first kernel validation harnesses. Good for GPU/C++
  developers.
- **Documentation.** Make the basis accessible without diluting it.

## Technical standards

- **Language policy:** engine and pipeline work is C++ (direct against `taichi_core` is the
  probable path). Python is front-end and developer tooling only — never in the hot path.
- **Tests on everything.** Every artifact ships with tests; oracle-first validation for every GPU
  kernel (CPU reference oracle, GPU mirror, hardware equivalence harness).
- **Addresses are arrayed pairs** in storage; the dotted form is display-only. Schema changes
  enforce this with types and constraints, not convention.
- No proprietary dependencies — AGPL-3.0 only. Docs in Markdown; be explicit, not clever; explain
  the why.
- The old databases are **read-only**. Extraction pulls; nothing repairs in place.

## How to contribute

1. **Discuss first** for non-trivial changes — open an issue describing the approach.
2. Branch (`feature/<name>` or `fix/<description>`), clear commits, PR against `main`.
3. For theoretical/architecture contributions: open an issue, present the critique, engage.

## Repository structure

```
human-cognome-project/
├── field/  ledger/          # the physics basis (primary sources)
├── docs/                    # physics-basis, architecture, data-protocol, legacy data maps
├── extraction/              # read-only pull toolkit (address codec, connectors, intake chains)
├── db/  data/  sources/     # source holdings (dumps, corpus, references) — read-only
├── review/                  # the 2026-08 rebase review record
├── archive/2026-08-rebase/  # the previous paradigm, preserved whole, with its ledger
└── covenant.md  charter.md  MANIFESTO.md  AGENTS.md  README.md  ROADMAP.md
```

By contributing, you agree your contributions are licensed under AGPL-3.0, governed by the
[Covenant](covenant.md).

**Welcome. Let's build the map of shared mind.**
