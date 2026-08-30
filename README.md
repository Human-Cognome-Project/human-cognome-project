# Human Cognome Project (HCP)

**An evolutionary, cosmological model of the history of human knowledge — movable, zoomable,
and solved as physics.**

HCP builds **Digital Intelligence (DI), not AI**. The project models all of human knowledge as a
single continuous field: every element — a word, a work, a creator, an idea — is a density imbalance
placed on temporal coordinates, connected across time, and governed by exactly one operation:

> **Single-point force balancing.** Every element adapts toward balance with its surroundings;
> time and distance across dimensions are the only confounders. Everything else — attraction,
> association, structure, meaning — is that one operation, compounded.

Creators enter the model as solar-class phenomena with lifespans, their works both in the core and
as orbital bodies. Knowledge aggregators (Wiktionary, Gutenberg, arXiv, Wikipedia) enter as
black-hole/white-hole pairs that draw in unstructured material and emit refined structure. HCP
itself is the big one at the end: it takes in *all* of knowledge — no source filtering, ever — and
emits it as a single navigable field. The inference layer that lives on the far side of that
compression is **NAPIER** — *Not Another Proprietary Inference Engine, Really!*

The physics this rests on was worked out first, as physics. Start there:

- **[docs/physics-basis.md](docs/physics-basis.md)** — the basis in brief, and why it forces
  everything else.
- **[ledger/](ledger/)** and **[field/](field/)** — the primary sources: the Mann ledger
  (separating frequency from amount) and the field model (attraction from two imbalances,
  no frequency term).

## Where the project is right now

**Rebase in progress (August 2026).** The physics basis was determined and the entire project was
reviewed and re-founded on it. The prior paradigm — a structural-linguistics engine with a working
byte→word resolution pipeline and a 1.49M-entry vocabulary substrate — is preserved whole in
[archive/2026-08-rebase/](archive/2026-08-rebase/), with every artifact's disposition recorded in
[review/](review/). Notably, the old work converged on the new basis independently more than once;
the archive documents that lineage honestly.

What exists and is current:

- **The physics packages** (`field/`, `ledger/`) — the basis, with measurements, derivations, and
  runnable models.
- **The data holdings** — 14 live PostgreSQL databases including the full Kaikki/Wiktionary
  ingestion (1.49M English entries with raw source preserved) and a first Gutenberg corpus. Verified
  extractable: the address convention held everywhere it was checked
  ([review/pass-04-data.md](review/pass-04-data.md)).
- **The extraction toolkit** (`extraction/`) — the arrayed-pair address codec and the read-only
  pull tooling for building the new substrate from the old stores.
- **The plan** — [ROADMAP.md](ROADMAP.md).

## Reading order

1. [docs/physics-basis.md](docs/physics-basis.md) — what was determined and why it changes everything.
2. [docs/architecture.md](docs/architecture.md) — the space, the one operation, the addressing, the singularity.
3. [docs/data-protocol.md](docs/data-protocol.md) — how knowledge enters: no filtering, declared provenance, flags not gates.
4. [ROADMAP.md](ROADMAP.md) — what happens next, in order.
5. [MANIFESTO.md](MANIFESTO.md) — why this and not statistical AI.

## Governance

- **[Covenant](covenant.md)** — perpetual-openness guarantee. Everything here stays free, forever.
- **[Charter](charter.md)** — how contributors treat each other.
- **[CONTRIBUTING.md](CONTRIBUTING.md)** — how to participate; **[AGENTS.md](AGENTS.md)** — an
  invitation to AI agents.
- **[LICENSE](LICENSE)** — AGPL-3.0, governed by the Covenant.

*Built in the open. Free forever. For every mind.*
