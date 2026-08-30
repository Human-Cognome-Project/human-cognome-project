# Intelligence: Data Quality × Traversal

NAPIER's working definition of intelligence is engineerable, not mystical.

> *Understanding, intelligence, and intuition reduce to two engineerable factors: (1) the
> **quality** of the underlying data — the maintained world-model / conceptual substrate
> (accuracy, completeness, structure, cleanliness) — and (2) the **ability to traverse** it
> (inference, the determination-engine walk, navigation over connections). Intelligence is the
> **product** of the two.* — claim 255

Neither factor is a special spark. Raise either and you raise intelligence; the product is what
matters.

| Term | What it is |
|------|-----------|
| **Intuition** | fast, good *traversal* over high-quality, densely-connected data — leaps that feel mystical are efficient pathfinding through well-formed structure (greedy-LoD surfacing the relevant path quickly). |
| **Understanding** | data that is both well-structured *and* walkable — you can traverse the decomposition/explication (the acyclic DAG + abstraction levels, claim 221). |

This is "[knowledge lives in the connections](principles.md#knowledge-lives-in-the-connections)"
(claim 140) **times** the walk over them (the determination engine).

---

## The two halves are both engineering targets

NAPIER raises intelligence by engineering **both** halves deliberately:

- **Data quality** — a clean substrate and a good world model (the maintained Newtonian space,
  claims 253–254; see [world-model-and-imagination.md](world-model-and-imagination.md)).
- **Traversal** — fast walking via db-functions, indexing, and broadphase-BVH pruning
  (the db-functions keystone, claim 192).

Together they are the difference between **determined correctness** and **coincidental
correctness** — the same distinction the foundations articles draw between a Gedankenmodell
(executes the logic) and a Phänomenmodell (simulates the output).

---

## Why the substrate-efficiency debt is an *intelligence* problem

This reframes a piece of technical debt as more than storage (claims 31, 212): token-reference
arrays are currently stored as full dotted token-ID **strings** inside the connection columns.
That costs ~5× bloat and lost B-tree/BVH compression — but the deeper cost is that it **cripples
the traversal half.** Dot-strings in the connections defeat the index-walking that traversal
depends on. So the debt is an **intelligence** problem, not merely a storage one. (Detail and
the planned fix — `text[]` with diff-only encoding — are in
[../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) and
[../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).)

---

## The traversal machinery: the determination engine

The traversal half is the **determination engine** — a persistent, rerootable determination
tree with all branches intact (claims 78–81).

- **Confidence is the master variable.** Cognition seeks low-energy states through
  determinations: *fly past* on dominant winners, *progressively drop back* on competition
  (claim 78).
- **Confidence is the softmax probability of the chosen token over a *curated* candidate set**
  (claim 79). The landscape is a top-p / top-k distribution; the dynamics **are** the math, not
  a separate layer. The *small n* produced by the curator (broadphase, slot-matching) is what
  keeps the O(n²) work tractable. **LLMs lack the curator** — same math, but over huge n, which
  is what makes their outcomes catastrophically opaque.
- **Backprop is live, not pre-training-only** (claim 80): within a current operation it acts as
  a rebalancing force *behind* established tokens. The surface form locks when chosen; the
  semantic/energy load stays mutable. Reinterpretation changes the energy landscape, not the
  literal word. Magnitude is proportional to surprise.
- **The WAL log is the tree's persistent form** (claim 81): it captures *every* db operation
  across all databases — envelope assembly, engine writebacks, scratchpad writes, crawl
  activity, LMDB composition — not just committed transactions. That breadth is what makes the
  audit trail complete and gives **exact backtrack to any inference state.**

---

## Determinism and traceability are one property

> *Given identical factors and identical data, the system produces essentially the same decision
> every time. Determinism and traceability are the same architectural property.* — claim 122

Both rest on:

- **no hidden state** — every db op lands in the WAL, exposing the complete chain of inputs and
  intermediate state, and
- **no exogenous noise** — confidence dynamics is deterministic energy-minimization over a
  curated candidate set, not stochastic sampling.

The WAL's completeness is the *formal evidence* of determinism: if everything is recorded,
replay reproduces the trace by construction. This is **determinism by audit completeness** — a
structural invariant, not a bolted-on forensic feature. Recreate-ability falls out as a
consequence.

> **Note for math-foundations conversations.** Confidence dynamics is described as a top-p/top-k
> landscape with a three-factor escalation (option count × importance × map complexity), not a
> hard threshold (claim 22). The *deeper weighting math* — what gets evaluated, with what weight,
> under what circumstances — is **explicitly deferred** and is the project's acknowledged math
> gap. See [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## See also

- [keystone-db-functions.md](keystone-db-functions.md) — why traversal is db-function dispatch.
- [../04-engine/overview.md](../04-engine/overview.md) — where in the engine the traversal
  (inference) side lives, versus the resolution (furnace) side.
