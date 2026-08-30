# Design Principles

The meta-principles that govern how NAPIER's database operations are organized. These are
not style preferences — they are the rules that decide which architectural elements earn
their place and how each behaves. Each is sourced from a Patrick-direct claim in the
orchestrator graph.

---

## Greedy Level-of-Detail (greedy-LoD)

> *Surface only divergences relevant at the required scale.* — claim 16

The single most pervasive principle. The system never carries more detail than the active
scale needs; it surfaces only what *diverges* from the steady state at that scale. It is the
unifying idea behind:

- **broadphase** (cheap pruning before expensive resolution),
- **pivot aggregation** (roll detail up to the level being asked about),
- the **engine boundary** (the flat slice the engine works on is an LoD-scoped emulation),
- **memory tiering** (cold / warm / hot),
- the **particle/bit architecture** (a token's bit-layout tree *is* its LoD),
- **confidence dynamics** (fly past dominant winners, only drop back where it's competitive).

Greedy-LoD also governs *time*: the cognitive cycle runs at a frequency *sufficient* for a
continuous impression, not at maximal resolution (claim 275). Snapshots, interpolated.

When a force, a query, or a detail "doesn't matter at this scale," greedy-LoD is why you are
allowed to drop it.

---

## Tractability

> *Every architectural element exists to make one or more essential factors tractable.* — claim 11

Justify a new element by the tractability it buys. Reject ornamental complexity. When you are
tempted by an elegant design, ask whether the elegant version improves a tractability or just
feels nice — if it only feels nice, it does not go in. This is the gate every structural
decision passes through.

---

## Activation, not existence

> *Whether a function is a liability is about when it fires and how, not whether it exists.* — claim 12

The same machinery serves productive and failure modes. You do not remove a function because
it can misfire; you design its **activation profile** — when and how it fires. Throwing out
the baby with the bathwater (deleting useful machinery to kill a failure mode) is the
anti-pattern this principle names.

---

## Failure modes are signals

> *System design's job is not to eliminate failure modes; integrate them as signals at the
> right substrate points.* — claim 14

Most "failure modes" are useful machinery firing at the wrong time or place. Integrate them
where they help instead of designing them out.

**The one explicit exception: confabulation.** NAPIER deliberately does *not* reproduce it.
Confabulation is ego-driven and non-universal in biological cognition; NAPIER follows the
non-confabulating, ego-inverted variant. This connects to the grounding story: gap-filling is
legitimate when it is *physics-warranted prediction* and illegitimate when it is *ungrounded
invention* (see [world-model-and-imagination.md](world-model-and-imagination.md)).

---

## Null vs zero (−0 vs +0)

> *−0 (NULL, unknown, no warrant) is not +0 (proven false, with warrant).* — claim 17

A substrate-level property with sharp consequences:

- **−0** — unknown, no warrant. The cell exists but holds no proven value.
- **+0** — proven false, *with* warrant.

The lifecycle of any datum: *doesn't-exist → −0 (minted, unknown) → populated (a value, or
+0).* **Learning is the −0 → populated transition.**

This diagnoses a specific LLM failure: training corpora collapse NULL into 0, so the model
treats "I have no warrant for this" as "this is the answer." NAPIER keeps the two distinct,
which is what lets it say *I don't know* honestly and fill gaps only with warrant. It is also
why **erasure is native** (claim 248): deleting a record returns a populated unit to −0 —
provably the exact prior state.

---

## Good and adaptive, not perfect first-pass

> *Catch what you cleanly can with bounded mechanisms; pass the rest to deeper layers; learn
> from the misses.* — claim 184

Subsystems are designed to be cheap-and-fast at the front and deep-and-thorough at the back,
with learning routing between them:

- catch what bounded mechanisms cleanly can (greedy-LoD at the fore),
- pass uncertain/missed cases to slower, deeper layers (concept engine, cold shard,
  background crawl),
- tag relevant cases for priority on next encounter,
- improve over passes.

This is the opposite of LLM-style brittleness, where comprehensive first-pass coverage is the
only mode. Practical results gate the design, not theoretical exhaustiveness.

---

## Knowledge lives in the connections

> *Knowledge lives in the cross-connections of a system, not in isolated atoms.* — claim 140

Every system is a web. Atomic facts in isolation carry almost no knowledge; the *structure of
relationships* between them is where the knowledge is. **Explication** is the discipline of
making that connection structure visible and queryable so the knowledge becomes shareable.

This applies recursively: concepts connect via prime decomposition, tokens connect via bonding
signatures, the world model is the persistent accumulation of these connections — and the
orchestrator claim-graph that sources *these very docs* earns its keep through its **edges**,
not its claims-as-atoms. A pile of nodes is data; the linked web is knowledge (claim 263).
This is also half of the intelligence equation — see
[intelligence.md](intelligence.md).

---

## Read claims literally (no-analogy-default)

> *Structural claims are literal unless explicitly flagged as analogy.* — claim 13

A reading discipline that matters for these docs specifically. When the architecture says
primes are *the element types* of a periodic table, or that imagination *is* a maintained
Newtonian simulation, or that grammar *is* the db-function API — these are **structural
equivalences**, not metaphors. Do not soften them to "like" or "as if." Where something is
genuinely an analogy, it is marked as one.

---

## How these compose

These are not independent rules; they intersect:

- greedy-LoD + tractability decide *what detail to carry*;
- activation + failure-modes-as-signals decide *how machinery behaves when stressed*;
- null-vs-zero + good-not-perfect decide *how the system handles what it doesn't yet know*;
- knowledge-in-connections is *why the whole thing is a graph and not a pile of facts*.

Each architectural element downstream of here should be readable as an application of one or
more of these.
