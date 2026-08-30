# Pass 1 — Root identity documents

**2026-08-30. Dispositions per REBASE_REVIEW_PLAN.md. Nothing executes until Pass 7.**

| path | disposition | reason / successor |
|---|---|---|
| `README.md` | **RW** | Project face. Old paradigm throughout (NAPIER, "cognition reduces to database functions" keystone, resolution chambers, NSM phases). Successor: deliverable 1 — the cosmological knowledge model, physics basis first. Factual inventory (1.494M-entry `hcp_english`, 10 shards, engine state) migrates to the archive's status snapshot, not the new README. |
| `MANIFESTO.md` | **RW** | The *stance* survives (cognition is physics — now literally field physics; openness; all sentience; transparency). The *content* is old paradigm: PBMs, NSM, two-engine model, O3DE/PhysX (internally stale vs README — evidence of drift, and part of why we rewrite rather than patch). Anti-statistical-AI framing should be re-founded on the new basis rather than carried as polemic. |
| `ROADMAP.md` | **RW** | 4-phase arc (Linguistic→ToM→Inference→Multimodal) is old paradigm and claim-graph-anchored. Successor: deliverable 5 — Kaikki lattice first, accretion order, open physics problems. |
| `CONTRIBUTING.md` | **RW** | Structure survives (read-first order, language policy, standards); content is old paradigm (keystone doc, claim-graph-as-authority for contributors, NSM work list). Claim-graph authority statement must be softened/reworked — graph is itself queued for supersession review. |
| `AGENTS.md` | **RW** | Two documents fused: (a) agent outreach — old-paradigm pitch, rewrite on new basis; (b) agent-team operations — roles/rules mostly paradigm-neutral, carry into a leaner ops section. NOTE: commit email here (`patrick@donaeley.com`) differs from current instruction (gmail); reconcile in rewrite. |
| `covenant.md` | **CF** | Perpetual-openness guarantee. Paradigm-neutral values; architecture-free. Carries forward unchanged. |
| `charter.md` | **CF** | Contributor conduct. Paradigm-neutral; carries forward unchanged. |
| `LICENSE` (AGPL-3.0) | **CF** | Unchanged. |
| `co.txt` | **AR** | A pasted Claude Code session transcript (early grammar-modeling instance orientation; trails off into API-error noise). Design history only. |
| `Human Cognome Project Invitation.docx` | **AR** | Feb 2026 outreach on the old paradigm. Successor: fresh invitation after the rewrite (and prefer a portable format over .docx). |
| `.gitignore` / `.gitattributes` | **CF** | Mechanics; prune entries as their targets are archived (Pass 7). |
| `.mcp.json` | **CF** | Local infra (browse + Discord relay). Untouched by paradigm. |
| `pyproject.toml` | **CF (revisit Pass 5)** | Tooling env. `warp-lang` dependency is a leftover GPU experiment — candidate DR at Pass 5; FastAPI/uvicorn likely relay-side. |

## Flagged for discussion

1. **Does the name NAPIER survive?** It names the old inference model (claim 202). The new system has no engine name yet — the field/ledger vocabulary ("Mann" is taken by the unit) doesn't obviously supply one. Rewrite needs a decision: retire NAPIER, repurpose it, or name the new engine.
2. **Tone of the public face.** MANIFESTO/AGENTS are framed *against* statistical AI. The new basis gives a positive framing instead (the model of knowledge; LLM-style systems are samplers with undeclared cycles — a ledger statement, not a polemic). Proposed: rewrite leads with what we build, not what we oppose.
3. **Claim-graph authority language** in CONTRIBUTING/README predates the graph's own review; the rewritten docs should point at the repo's new document set as the contributor-facing authority until the graph sweep lands.
4. **Covenant/Charter untouched** — confirming that's the intent; they read as deliberately paradigm-independent.

## Carried to later passes

- Old README/MANIFESTO factual claims (shard counts, accuracy numbers) → verified against live stores in Pass 4 before the archive snapshot asserts them.
- AGENTS.md ops rules overlap with `.claude/` and memory conventions → reconciled in Pass 5/7.
