# Reading Order

The docs are numbered `00`–`07` to encode a default traversal, but the right path depends on why
you're here. Pick a track.

> The docs are themselves a web (knowledge lives in the connections, claim 140) — every page links to
> its neighbors. These tracks are entry paths, not fences.

---

## Everyone: start here

1. [what-is-napier.md](what-is-napier.md) — the one-page orientation.
2. [../02-architecture/keystone-db-functions.md](../02-architecture/keystone-db-functions.md) — the
   single realization the rest follows from. **Read this before anything technical.**
3. [glossary.md](glossary.md) — keep it open in a tab.

Then, before going deep on any subsystem, skim:

- [../06-status/status.md](../06-status/status.md) — what's built vs paused vs active.
- [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) — what's deferred / in-flux.
  (This keeps you from mistaking intent for done-state.)

---

## Track: the theory / "why" (newcomers, skeptics, reviewers)

1. [../01-foundations/](../01-foundations/) — the first-principles articles (LLM mechanics; ToM
   modelling axiom; linguistic archaeology; the shape of a word).
2. [../02-architecture/principles.md](../02-architecture/principles.md) — the design meta-principles.
3. [../02-architecture/linguistic-vs-conceptual.md](../02-architecture/linguistic-vs-conceptual.md) —
   the concept/language separation and the hub.
4. [../02-architecture/interface-and-tom.md](../02-architecture/interface-and-tom.md) — interfaces,
   PBD, the translation bridge.
5. [../02-architecture/intelligence.md](../02-architecture/intelligence.md) — intelligence = data ×
   traversal.

---

## Track: the engine (engine devs)

> Engine docs are intentionally **light** right now (Gem internals are in rework; much of the
> mechanics is forward-looking design — claim 292). Start with the overview and the *built* state;
> the deep per-subsystem pages are deferred until the rework settles.

1. [../04-engine/overview.md](../04-engine/overview.md) — the three-part meta-structure (light; marks
   current vs future).
2. [../04-engine/implementation-baseline.md](../04-engine/implementation-baseline.md) — the verified
   current engine (what's actually built).
3. [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md) — text → tokens (the built
   ingestion path).
4. [../07-operations/build-and-run.md](../07-operations/build-and-run.md) — build + run.

---

## Track: the data / substrate (DB + data devs)

1. [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) — shards + schema.
2. [../05-data-layer/kaikki-pipeline.md](../05-data-layer/kaikki-pipeline.md) — the source pipeline.
3. [../05-data-layer/tokenization-policies.md](../05-data-layer/tokenization-policies.md) — what gets
   stored vs derived.
4. [../05-data-layer/var-and-continuation.md](../05-data-layer/var-and-continuation.md) — vars + the
   learning loop.
5. [../07-operations/database-access.md](../07-operations/database-access.md) — connect + query.

---

## Track: the concept substrate (the current active work)

1. [../03-concept-substrate/primes-and-molecules.md](../03-concept-substrate/primes-and-molecules.md)
2. [../03-concept-substrate/explication.md](../03-concept-substrate/explication.md) — meaning as a db
   query (the live edge).
3. [../03-concept-substrate/punctuation-nonverbal.md](../03-concept-substrate/punctuation-nonverbal.md)
4. [../03-concept-substrate/forces-and-pbd.md](../03-concept-substrate/forces-and-pbd.md) — *rough,
   flagged*.
5. [../03-concept-substrate/bit-classes.md](../03-concept-substrate/bit-classes.md) — *under review,
   flagged*.

---

## A note on source of truth

These docs are **distilled from the orchestrator claim-graph** (`hcp_orchestrator`), which is the
current authoritative architecture. Where a doc and the claim-graph disagree, the **claim-graph
wins** — and the doc is stale. Claim IDs are cited inline (e.g. "claim 192") so you can trace any
statement back. See [../07-operations/database-access.md](../07-operations/database-access.md) for how
to query the graph.
