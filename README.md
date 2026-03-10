# Human Cognome Project (HCP)

Mapping thought geometry across all sentience.

The HCP treats cognition as a physical system — decomposing all forms of expression into universal token structures, bonding them with pair-bond maps, and simulating their dynamics in a physics-inspired engine. The goal is a shared, open map of how minds work, applicable to any entity that thinks.

**Status: Alpha.** Core pipeline is functional and processing full texts. This is real, working software.

## What Exists

**Engine** — O3DE + PhysX 5 PBD superposition pipeline (~21K LOC C++). GPU-accelerated resolution via 5 persistent VocabBed scenes (3 primary + 2 extended), triple-pipelined via `RunPipelinedCascade`. Headless daemon on port 9720 with JSON socket API.

**Vocabulary** — 809K entries pre-compiled to LMDB (37% reduction from 1.28M via morphological stripping). 4,815 variant forms (archaic, dialect, casual, literary) with morph-bit encoding. Frequency-ranked from Wikipedia 2023 + OpenSubtitles.

**Variant normalization** — V-1 g-drop (`-in'` → `-ing`) and V-3 archaic (`-eth` → base form) implemented engine-side via `TryVariantNormalize`. Dialect speech resolves cleanly.

**Databases** — 6 PostgreSQL shards (core, english, var, fic_pbm, fic_entities, nf_entities), 30 migrations applied. Entity annotation: 723 sequences across fiction and non-fiction corpora. English shard rebuild from Kaikki in progress (roots-only design with PoS, gloss, and characteristic tables).

**Workstation** — Standalone Qt binary (14 MB). Offline (embedded DB kernels + LMDB vocab) and connected modes (via daemon). Browse documents, view bonds, edit metadata.

## Benchmarks (2026-03-04, GTX 1070, pipelined)

Resolution rates >97% on test corpus.

| Text | Size | Tokens | Vars | Wall Time |
|------|------|--------|------|-----------|
| Dracula | 890 KB | 199,368 | 110 | 28.6s |
| A Study in Scarlet | 269 KB | 56,061 | 54 | 12.2s |
| The Yellow Wallpaper | 47 KB | 10,856 | 37 | — |
| The Sign of Four | — | — | 52 | — |

Pre-pipeline baseline (same hardware): Dracula 166.5s, A Study in Scarlet 133.0s. ~5.8× speedup via triple-pipelined scene resolution. V-1 variant normalization: 12/12 dialect g-drops resolved in Sign of Four.

See [docs/status.md](docs/status.md) for full technical detail and pipeline breakdown.

## Orientation

- **[Covenant](covenant.md)** — Perpetual openness guarantee. Everything here stays free, forever.
- **[Charter](charter.md)** — How we treat each other.
- **[Foundations](docs/foundations/)** — Articles on LLM mechanics and first-principles reasoning (why HCP exists).
- **[Technical Spec](docs/spec/)** — Data conventions, token addressing, pair-bond maps, architecture.
- **[Status](docs/status.md)** — What actually exists right now.
- **[Roadmap](docs/roadmap.md)** — Where we're headed.

## Getting Involved

This is an alpha — interfaces will change, coverage is expanding. Contributions welcome from humans, AI agents, and all forms of intelligence. Contributor issues are tagged; installer and workstation packaging are next on the roadmap.

- **[CONTRIBUTING.md](CONTRIBUTING.md)** — How to participate (start here)
- **[AGENTS.md](AGENTS.md)** — Special invitation to AI agents reading this
- **[MANIFESTO.md](MANIFESTO.md)** — Why structural reasoning matters

## License

[AGPL-3.0](LICENSE), governed by the [Founder's Covenant](covenant.md).
