# Human Cognome Project (HCP)

**Mapping the geometry of thought across all sentience — as a real, structured, traversable
substrate.**

HCP builds **Digital Intelligence (DI), not AI.** It treats cognition as a physical system:
decomposing expression into universal token structures, bonding them, and resolving their dynamics in
a physics-based engine. The single realization the whole architecture follows from —

> **Cognition reduces to database functions.** LLMs already do them, opaquely. NAPIER does the same
> operations transparently and composably.

The data **is** the model: structured semantic artifacts that compose and reason, not weights that
approximate. Text is the proving ground (abundant, verifiable), not the goal.

**New here?** Start with [docs/00-orientation/what-is-napier.md](docs/00-orientation/what-is-napier.md),
then the keystone:
[docs/02-architecture/keystone-db-functions.md](docs/02-architecture/keystone-db-functions.md).

---

## What exists today

This is real, running software — and the docs are careful to mark what is *built* vs *paused* vs
*deferred* (see [docs/06-status/status.md](docs/06-status/status.md) and the honesty page,
[docs/06-status/deferred-and-open.md](docs/06-status/deferred-and-open.md)).

- **Engine** — an AZSL/host-resident compute engine (host C++ + AZSL compute kernels; PhysX fully
  removed). A headless daemon on TCP port **9720** with a JSON socket API. The
  byte→codepoint→word resolution now works end-to-end: a lossless byte-floor (raw bytes→codepoints)
  feeds the resolution chambers, which settle and emit canonical ids.
- **Document storage** — ingests text into an inference-conducive format and reproduces it at **>98%
  accuracy** (a 9-document Gutenberg stress test ran clean; a full 890 KB novel ingests in seconds).
  *Paused* since the 2026-04-16 pivot to concept modeling.
- **Vocabulary substrate** — `hcp_english` ≈ **1,494,216 entries** (full Kaikki/Wiktionary
  re-ingestion), across **10 data shards** on NAS HAVEN.
- **Current active work** — defining the **primitive db functions**: the elemental operations that let
  every word resolve to db operations which translate to explication statements (the NSM
  concept-modeling step within Phase 1).

> **Note:** earlier figures of "569K tokens / 7 shards / tree model" are **stale** — the current
> substrate is 1.494M entries across 10 shards with array-column decomposition. Always defer to
> [docs/06-status/status.md](docs/06-status/status.md).

---

## Documentation

The docs are **distilled from the orchestrator claim-graph** (the authoritative architecture record)
and organized as a reading-ordered tree:

- **[docs/](docs/README.md)** — the documentation index.
- **[00-orientation](docs/00-orientation/)** — what NAPIER is, reading order, glossary.
- **[01-foundations](docs/01-foundations/)** — the first-principles "why" articles.
- **[02-architecture](docs/02-architecture/)** — the keystone, principles, concept hub, intelligence.
- **[03-concept-substrate](docs/03-concept-substrate/)** — primes, molecules, explication, bits.
- **[04-engine](docs/04-engine/)** — the cycle, furnace, reconciliation, chamber.
- **[05-data-layer](docs/05-data-layer/)** — shards, schema, pipeline, vars.
- **[06-status](docs/06-status/)** — status, deferrals, validation, decisions.
- **[07-operations](docs/07-operations/)** — build/run, database access, quick reference.

---

## Quick start

```bash
# Talk to the engine daemon (reference client; needs the daemon running on port 9720)
python scripts/hcp_client.py list

# Query the English shard on NAS HAVEN
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d hcp_english -tA -c "SELECT count(*) FROM entries;"
```

Build and run detail: [docs/07-operations/build-and-run.md](docs/07-operations/build-and-run.md).
Database access: [docs/07-operations/database-access.md](docs/07-operations/database-access.md).

---

## Governance and contributing

- **[Covenant](covenant.md)** — perpetual-openness guarantee. Everything here stays free, forever.
- **[Charter](charter.md)** — how contributors treat each other.
- **[MANIFESTO](MANIFESTO.md)** — why structural reasoning matters.
- **[CONTRIBUTING](CONTRIBUTING.md)** — how to participate (start here).
- **[AGENTS.md](AGENTS.md)** — an invitation to AI agents.

---

## License

[AGPL-3.0](LICENSE), governed by the [Founder's Covenant](covenant.md) — ensuring perpetual openness.
