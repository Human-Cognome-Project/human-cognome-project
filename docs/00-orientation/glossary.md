# Glossary

Terms used throughout the HCP docs. Many are Patrick-coined or used in a precise, literal sense —
remember **no-analogy-default** (claim 13): structural claims are literal unless flagged.

---

**AA / AB namespace** — AA = universal concept tokens (`hcp_core`); AB = English text-form tokens
(`hcp_english`). A shared label (e.g. WANT / want) does **not** mean a shared entity. See
[../02-architecture/linguistic-vs-conceptual.md](../02-architecture/linguistic-vs-conceptual.md).

**Bit-class / bit-layout** — the per-type tree of conditional bits that encodes a token's
content-type-and-structure. The bit-layout **is** the level-of-detail (claim 60). *Under review* —
[../03-concept-substrate/bit-classes.md](../03-concept-substrate/bit-classes.md).

**Broadphase** — cheap pruning that surfaces only plausible candidates before expensive resolution;
the curation step that keeps the candidate set (n) small.

**Chamber (resolution chamber)** — the engine stage that converts input text into identified tokens
by progressive composition. Identification only; *meaning is irrelevant here*.
[../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md).

**Cold / warm / hot** — the three storage tiers. Cold = NAS shards (possibility space); warm =
Postgres `hcp_var` (the temporal triad — reference DBs / var DB / WAL); hot = LMDB (cognitive
resolution working set). [../04-engine/overview.md](../04-engine/overview.md).

**Confidence dynamics** — the determination engine's master variable; a top-p/top-k landscape over a
curated candidate set, not a hard threshold (claim 22). The *dynamics are the math*.

**Deeming** — the prioritization layer: what NAPIER attends to / loads as relevant. Mechanism built;
policy human-set for now (claims 276/277); the *deeper weighting math is deferred* (the math gap,
claim 286).

**Determination engine** — the inference-walk side: a persistent, rerootable determination tree;
confidence-driven energy minimization over curated candidates (claims 78–81).
[../02-architecture/intelligence.md](../02-architecture/intelligence.md).

**DI (Digital Intelligence)** — what HCP builds. *Not* AI, *not* NLP, *not* alternative-LLM.
Cognitive infrastructure grounded in the physics of thought (claim 93).

**Envelope** — used in the **oldest db sense**: a combination of queries and filters that shape a
workspace (claim 273). What NAPIER works on each moment. (Inner deeming-composition not yet fully
specified — claims 39/40/41.)

**Explication** — the db-instruction-set whose converging result is the one symbol it commonly tags.
A word's meaning **is** its explication-as-query (claim 258).
[../03-concept-substrate/explication.md](../03-concept-substrate/explication.md).

**Furnace (resolution furnace)** — the GPU. Its only job is parallel physics/PBD resolution.
**Cognition does not live in the furnace** (claim 281).

**Gedankenmodell / Phänomenmodell** — thought-model (executes the logic) vs phenomenon-model
(simulates the observable output). The foundations argument is that LLMs are the latter; DI requires
the former. [../01-foundations/](../01-foundations/).

**Greedy-LoD** — surface only the divergences relevant at the active scale (claim 16). The most
pervasive principle. [../02-architecture/principles.md](../02-architecture/principles.md).

**LoD (Level of Detail)** — the active scale at which divergences are surfaced and queries truncated.

**LMDB** — the hot tier. Two separate memory-mapped stores (input + output), each single-writer /
source-locked, zero-contention (claim 119).
[../04-engine/reconciliation-loop.md](../04-engine/reconciliation-loop.md).

**Molecule** — a compound concept; lexicalized topology of primes, not an arbitrary set (claim 220).
A prime is a depth-0 molecule (claim 222).

**NAPIER** — the inference-model name (claim 202).

**NSM (Natural Semantic Metalanguage)** — Wierzbicka/Goddard's 65 semantic primes (16 categories).
HCP uses them as the fixed particle taxonomy (claim 219) and as the current low-level compiler / on-
ramp toward language-neutral db primitives (claim 243) — *not as gospel*.

**Null vs zero (−0 / +0)** — −0 = unknown / no warrant; +0 = proven false / with warrant. Learning is
−0 → populated (claim 17). [../02-architecture/principles.md](../02-architecture/principles.md).

**PBD (Position Based Dynamics)** — the physics method the engine uses; molecule definitions are read
off PBD equilibrium settling (claim 213).

**PBD (Perspective Based Dimension)** — *different term, same initials.* Patrick-coined: a frame in
which factors orient; multiple PBDs **intersect** rather than conflict (claims 87/94). Context
disambiguates; the physics one is "Position Based Dynamics," the perspective one is "Perspective
Based Dimension."

**PBM (Pair-Bond Map)** — the bond structure at any scale (byte→char, char→word, …); the same
operation at every LoD (claim 225).

**Prime** — a primitive concept particle; one of the 65 NSM primes. The element types of the concept
field (claim 219). [../03-concept-substrate/primes-and-molecules.md](../03-concept-substrate/primes-and-molecules.md).

**Proof-point** — grade-school reading comprehension; the lower grade the better (claim 23). The
Phase 1 linguistic proof.

**See-it-mint-it** — every surface form seen mints its own token; regulars are derived, not stored
(claims 56/229). [../05-data-layer/tokenization-policies.md](../05-data-layer/tokenization-policies.md).

**Token** — a minted surface-form unit (the *key*); its referent is a concept node.

**ToM (Theory of Mind)** — the model of another mind's mental state. Dialogue runs 4+ concurrent ToM
constructs per participant; all expression conveys ToM (claims 82/200).

**Var DB** — `hcp_var`; the "what I am doing" scratchpad holding unresolved sequences with a lifecycle
(claim 232). [../05-data-layer/var-and-continuation.md](../05-data-layer/var-and-continuation.md).

**WAL log** — the write-ahead log capturing *every* db op across all DBs; the determination tree's
persistent form and the basis of determinism-by-audit (claims 81/122).

**World model** — the central object cognition serves; the persistent accumulation of NAPIER's
beliefs (claim 18). [../02-architecture/world-model-and-imagination.md](../02-architecture/world-model-and-imagination.md).
