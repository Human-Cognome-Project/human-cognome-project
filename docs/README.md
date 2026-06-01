# HCP Documentation

The documentation for the Human Cognome Project, **distilled from the orchestrator claim-graph**
(`hcp_orchestrator`) — the current authoritative source of truth for the architecture. Where these
docs and the claim-graph disagree, the claim-graph wins. Claim IDs are cited inline so any statement
can be traced back.

**New here?** → [00-orientation/what-is-napier.md](00-orientation/what-is-napier.md), then pick a
track in [00-orientation/reading-order.md](00-orientation/reading-order.md).

---

## The tree

The sections are numbered to encode a default reading order; the docs are a linked web, so follow the
cross-links freely.

### [00-orientation/](00-orientation/)
- [what-is-napier.md](00-orientation/what-is-napier.md) — the one-page orientation.
- [reading-order.md](00-orientation/reading-order.md) — tracks by role.
- [glossary.md](00-orientation/glossary.md) — terminology.

### [01-foundations/](01-foundations/)
The first-principles "why" articles — LLM mechanics, the Theory-of-Mind modelling axiom, linguistic
archaeology, the shape of a word.

### [02-architecture/](02-architecture/)
- [keystone-db-functions.md](02-architecture/keystone-db-functions.md) — **read first**: cognition is
  db functions.
- [principles.md](02-architecture/principles.md) — greedy-LoD, tractability, null-vs-zero, etc.
- [linguistic-vs-conceptual.md](02-architecture/linguistic-vs-conceptual.md) — the concept hub.
- [world-model-and-imagination.md](02-architecture/world-model-and-imagination.md) — two spaces,
  predictive perception.
- [interface-and-tom.md](02-architecture/interface-and-tom.md) — interfaces, PBD, the translation
  bridge.
- [intelligence.md](02-architecture/intelligence.md) — intelligence = data × traversal; the
  determination engine.

### [03-concept-substrate/](03-concept-substrate/)
- [primes-and-molecules.md](03-concept-substrate/primes-and-molecules.md)
- [explication.md](03-concept-substrate/explication.md) — meaning as a db query (the live edge).
- [bit-classes.md](03-concept-substrate/bit-classes.md) — *under review*.
- [forces-and-pbd.md](03-concept-substrate/forces-and-pbd.md) — *rough English skin, flagged*.
- [punctuation-nonverbal.md](03-concept-substrate/punctuation-nonverbal.md)

### [04-engine/](04-engine/) — *light, by design (claim 292)*
Engine docs are kept light: Gem internals are in rework and much of the mechanics is forward-looking
design. Deep per-subsystem pages are deferred (parked in `_archive/04-engine-deep-deferred/`).
- [overview.md](04-engine/overview.md) — the three-part meta-structure (marks current vs future).
- [implementation-baseline.md](04-engine/implementation-baseline.md) — the verified current engine
  (what's actually built).
- [resolution-chamber.md](04-engine/resolution-chamber.md) — text → tokens (the built ingestion path).

### [05-data-layer/](05-data-layer/)
- [shards-and-schema.md](05-data-layer/shards-and-schema.md) — 10 shards, 1.494M entries, the
  decomposition pattern.
- [kaikki-pipeline.md](05-data-layer/kaikki-pipeline.md) — the source pipeline.
- [tokenization-policies.md](05-data-layer/tokenization-policies.md) — see-it-mint-it, inflection at
  runtime.
- [var-and-continuation.md](05-data-layer/var-and-continuation.md) — vars + learning loop.

### [06-status/](06-status/)
- [status.md](06-status/status.md) — done / paused / active / deferred.
- [deferred-and-open.md](06-status/deferred-and-open.md) — **the honesty page.**
- [validation.md](06-status/validation.md) — cross-model + adjacent-worker validation signals.
- [decisions/](06-status/decisions/) — annotated decision records (001/002/005).

### [07-operations/](07-operations/)
- [build-and-run.md](07-operations/build-and-run.md) — build the Gem, run the daemon.
- [database-access.md](07-operations/database-access.md) — connect to shards + the claim-graph.
- [quickref.md](07-operations/quickref.md) — the cheat-sheet.

---

## Top-level companions (repo root)

- [../README.md](../README.md) — project front door.
- [../ROADMAP.md](../ROADMAP.md) — the 4-phase macro arc.
- [../CONTRIBUTING.md](../CONTRIBUTING.md) — how to contribute.
- [../charter.md](../charter.md) · [../covenant.md](../covenant.md) · [../MANIFESTO.md](../MANIFESTO.md)
  — governance.

---

## How these docs are maintained

The architecture is recorded in the orchestrator claim-graph **first**; the docs follow it. When you
update the architecture, update the claim-graph (see
[07-operations/database-access.md](07-operations/database-access.md)) and then reconcile the affected
docs. A `_archive/` folder holds superseded docs with a one-line "superseded by" header, rather than
deleting them (auditable supersession — cf. the native-erasure ethos, claim 248).
