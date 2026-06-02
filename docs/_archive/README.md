# Archive

These are the **pre-2026-06 documentation files**, retained for history. They were superseded
wholesale by the ground-up documentation rewrite (Patrick directive, claim 289) that built a fresh
`docs/` tree (`00-orientation` … `07-operations`) sourced from the orchestrator claim-graph.

> **The legacy files are SEALED in [`legacy-docs-pre-2026-06.tar.gz`](legacy-docs-pre-2026-06.tar.gz)**
> — deliberately compressed so that loose stale markdown is **not** sitting in the live tree where an
> agent or reader could mistake it for current. This README is the plaintext pointer; the content is
> in the tarball (and in git history). To inspect: `tar -xzf legacy-docs-pre-2026-06.tar.gz` in a
> scratch location.

> **Status: historical.** Nothing in that tarball is current. Where those files contain a fact, the
> current, authoritative version is in the new tree (and ultimately in the `hcp_orchestrator`
> claim-graph). They are kept rather than deleted so provenance is auditable — consistent with the
> project's native-erasure / auditable-supersession ethos (claim 248). Several claims cite specific
> files here in their `source_file` provenance.

> **Also here:** [`deferred-deep-engine.tar.gz`](deferred-deep-engine.tar.gz) — the three deep engine
> pages that were *deferred* (not superseded) per claim 292. See
> [04-engine-deep-deferred/README.md](04-engine-deep-deferred/README.md) for why those are a separate,
> promotable category rather than stale legacy.

---

## Where the content went

The rewrite reorganized by topic rather than by document. The mapping below points each archived area
to its current home.

| Archived | Superseded by | Notes |
|----------|---------------|-------|
| `status.md`, `project-update-2026-04-07.md`, `data-pipeline-status-2026-04-07.md` | [../06-status/status.md](../06-status/status.md) | counts + done-state were stale (569K → 1.494M; tree-model gone) |
| `roadmap.md` | [../../ROADMAP.md](../../ROADMAP.md) | macro arc kept; stale per-phase focus lists dropped |
| `spec/architecture.md`, `spec/napier-inference-engine.md` | [../04-engine/](../04-engine/) | engine rewritten from claims 201/57/281 |
| `spec/namespace-reference.md`, `spec/token-addressing.md` | [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) | Layer A–F encoding superseded by tree model |
| `spec/pair-bond-maps.md` | [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md), [../03-concept-substrate/forces-and-pbd.md](../03-concept-substrate/forces-and-pbd.md) | PBM mechanics (claims 225/226) |
| `spec/data-conventions.md` | [../03-concept-substrate/explication.md](../03-concept-substrate/explication.md) | explication DAG constraints (claim 221) |
| `spec/identity.md` | [../../ROADMAP.md](../../ROADMAP.md) (Phase 2) | personality seed/living layer (claim 233) |
| `spec/cache-miss-resolver-spec.md` | — | pre-orchestrator, old token-table layout; obsolete |
| `kaikki-curation-standards.md`, `kaikki-tag-mapping.md`, `kaikki-analysis.md`, `hcp-english-schema-design.md` | [../05-data-layer/kaikki-pipeline.md](../05-data-layer/kaikki-pipeline.md) | curation rules remain good source material (claims 228–230) |
| `kaikki-population-plan.md` | [../05-data-layer/kaikki-pipeline.md](../05-data-layer/kaikki-pipeline.md) | 6-phase staging plan self-superseded |
| `prefix-predictive-pipeline-design.md` | [../05-data-layer/tokenization-policies.md](../05-data-layer/tokenization-policies.md) | prefix stripping (claim 231) |
| `variant-rules-proposal.md`, `variant-forms-audit-2026-03-04.md` | [../05-data-layer/tokenization-policies.md](../05-data-layer/tokenization-policies.md) | pre-tree-model; `canonical_id` column gone; V-1/V-3 implemented |
| `research/source-workstation-design.md`, `research/var-db-schema-design.md`, `research/continuation-index-design.md` | [../05-data-layer/var-and-continuation.md](../05-data-layer/var-and-continuation.md), [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md) | tokenization-as-physics + var mechanics (claims 224/232) |
| `research/english-force-patterns.md`, `research/english-sub-cat-patterns.md`, `research/force-pattern-db-requirements.md`, `grammar-identifier-spec.md` | [../03-concept-substrate/forces-and-pbd.md](../03-concept-substrate/forces-and-pbd.md) | force notation is a **rough English skin** (claim 241); a *different aspect*, not superseded outright (claim 236) — see that page |
| `research/nsm-isko-primitives-research.md` *(in main tree)* | [../03-concept-substrate/primes-and-molecules.md](../03-concept-substrate/primes-and-molecules.md) | newest/foundational NSM source (claims 219–223) |
| `research/pbm-storage-schema-design.md`, `research/pbm-reference-systems.md`, `research/tokenizer-redesign.md`, `research/sub-structure-analysis.md` | [../05-data-layer/](../05-data-layer/) | superseded by 6-way entity split + array columns + O(n) continuation index |
| `research/entity-db-design.md`, `research/pbm-entity-db-review.md` | [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) | entity DB design → decision record / current schema |
| `o3de_architecture_research.md`, `o3de_engine_architecture_research.md` | [../04-engine/implementation-baseline.md](../04-engine/implementation-baseline.md) | O3DE-as-compiler premise holds (claim 239); ingestion-Gem path pending review |
| `engine-quickref.md`, `contributor-guide.md` | [../07-operations/quickref.md](../07-operations/quickref.md), [../../CONTRIBUTING.md](../../CONTRIBUTING.md) | quickref referenced morph reconstruction (gone) |
| `db-*`, `engine-*`, `questions-for-db-specialist.md`, `*-consultation.md`, `*-feedback-*.md` | — | ephemeral cross-specialist conversation transcripts; no current home |
| `root-TODO-2026-03-06.md` (was repo-root `TODO.md`) | [../../ROADMAP.md](../../ROADMAP.md) + [../06-status/status.md](../06-status/status.md) + [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) | stale planning artifact (morph-bit storage gone; old pivot priorities; dead `docs/TODO.md` link). Per-area TODOs under `hcp-engine/`, `db/`, `scripts/` are owned by those areas and untouched. |

> A couple of the newest foundational source docs (`research/nsm-isko-primitives-research.md`,
> `physx-mapping-2026-05-11.md`, `prime-force-definitions-2026-05-11.md`,
> `explication-instruction-set-draft.md`) live only in the **main working tree**, not on this branch
> — they were distilled into the claim-graph (claims 213–222, 258) and the new docs source from those
> claims. They are not duplicated into this archive.

---

## Why archive instead of delete

History has value, and the covenant/erasure ethos (claim 248) favors **auditable supersession** over
silent deletion: keep proof of what was there, with a clear pointer to what replaced it. If you are
hunting for the origin of a current claim, the `source_file` tag in the claim-graph will often point
into this directory.
