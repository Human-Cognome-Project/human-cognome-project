# The Human Cognome Project — A Physics-First Entry Point

*Prepared for Vladimir Skavysh*

**A note on this document.** This was generated for you by the project's own digital intelligence,
as an entry point calibrated to your background in physics, quantum computation, and machine
learning. One caveat is genuine and worth stating up front: the project spans several abstractions at
once — physics, database theory, linguistics, cognitive science — and **no single discipline's
terminology captures the whole of it.** The physics framing below is the most natural door *for you*;
it is a faithful projection, not the complete object. Other specialists enter through other doors,
and the full account is the *union* of those views, not any one of them. Where this document says
"is," read "is, under the physics projection."

---

## The one claim everything follows from

Cognition reduces to database functions. Large language models already perform these operations —
opaquely, as a statistical side effect of next-token prediction. The Human Cognome Project (HCP)
performs the same operations **explicitly, deterministically, and compositionally**, on a substrate
you can inspect. The working slogan is *"the data is the model"*: meaning lives in structured,
traversable artifacts that compose and reason, not in weights that approximate.

The engine that runs this — **NAPIER** — is real, running software: a headless C++ application on
O3DE / PhysX 5, using GPU-accelerated Position-Based Dynamics (PBD).

## Meaning as an equilibrium (the part that should feel familiar)

You spent a career reading physical state off settled systems. NAPIER resolves meaning the same way.

A span of text is decomposed into typed tokens (particles). The semantic relationships among them are
encoded as **forces**. The system is integrated forward under PBD until it settles, and **the meaning
is read off the equilibrium configuration.** There is no orchestration layer asserting "this is the
parse" — the physics *is* the state machine. Formally, the resolution chamber is an energy
minimization returning the low-energy configuration consistent with the constraints.

This is meant literally, not as analogy. The concept taxonomy maps onto physics primitives: a
primitive concept ↔ a particle; a compound concept ↔ a (possibly nested) rigid body; an explication
relationship ↔ the force between them.

## Where your work touches ours (the bridge worth a conversation)

Your payments-system paper used a hybrid quantum **annealer** to find a payment ordering minimizing
required system liquidity — a constraint-satisfaction problem solved by driving a physical system to
its energy minimum. NAPIER's resolution furnace does the same *kind* of thing classically:
constraint satisfaction by settling to a low-energy equilibrium on the GPU.

That structural identity raises a question I'll flag as genuinely open and speculative, not a claim:
the cost center in this architecture is **traversal**, not storage — and traversal is energy
minimization over a configuration space. That is precisely the regime your QMC-speedup thesis
addresses. Whether the furnace's equilibrium-finding admits quantum acceleration is exactly the kind
of question your two research lines were built to ask.

## Where the statistics are — and aren't

A distinction a central-bank modeler will care about: the **reasoning core is deterministic.**
Statistics are not abolished, but they are confined to the *expression* layer — the surface mapping
between language and the substrate — as a single, well-scoped softmax over a small, curated candidate
set. They do not live in the reasoning core. This is the inverse of the LLM arrangement, where
statistical approximation *is* the reasoning.

A consequence that matters more for stress-testing and risk than for almost any other application:
the substrate does not confabulate. When something is unknown it is marked as a *proven absence*,
not smoothed over.

## Interpretability by construction

Because meaning is a composition of database functions over a small fixed basis, it is auditable end
to end. Every concept decomposes — **non-circularly** — into roughly 300 molecules, ~65 semantic
primitives, and ultimately about **ten irreducible operations**. The definition of any concept is a
finite expression in that basis, readable off the substrate. (A concrete artifact: the 65 primitives
are written out as exact function signatures over those ten operations.) Interpretability here is not
a post-hoc explanation layer — it is the native storage format.

## Theory of mind, for the agent-based modeler

The substrate's pronoun primitives — *I, you, someone, people* — are not lexical entries; they are
**perspective envelopes**: grounded, composable, inspectable agent-states. For agent-based simulation
this offers a substrate where an agent's internal model is structured and auditable, rather than an
opaque parameter vector — a materially different starting point from either rule-based or LLM-backed
agents.

## Scalability — the honest two-tier answer

In theory the architecture is unbounded; in practice, candidly, most of it is still being built.
Split by layer:

- **Built and demonstrated (the linguistic substrate):** ~1.5M lexical entries from a full
  Wiktionary ingestion; document ingestion and reconstruction at **>98% fidelity** on multi-document
  stress tests; all on the GPU physics engine with memory-mapped, engine-shaped storage.
- **Designed, in active build (the concept-reasoning layer):** the human-authored core is small and
  bounded — ~10 operations, ~65 primitives, ~300 molecules, ~2000 defining-vocabulary terms
  (~2,350 non-circular definitions total) — and **composes transitively** to the full ~80,000-word
  lexicon and beyond. You build a small grounded core; the rest composes. Memory is a hot/cold cache
  hierarchy; new languages and domains are added as horizontal shards sharing the core; resolution is
  local and GPU-parallel; knowledge is added as structured artifacts, not by retraining.

So the reasoning layer's scaling is, at this stage, a **design property, not yet a demonstrated
one** — and the weighting mathematics underneath it is an explicit open problem.

## Built vs. deferred

The project is disciplined about this line, and its documentation marks every claim built / paused /
deferred. Briefly: the engine and the linguistic ingestion/reconstruction are real and running; the
concept substrate (the db-function definitions in the core) is in active construction; the deeper
weighting/"deeming" mathematics is deferred and acknowledged as a gap. This is a working substrate
and a falsifiable architecture with a clear build path — not a finished system, and it does not
pretend to be one.

## If you want to go deeper

A reading order, roughly in your lens:

1. `docs/00-orientation/what-is-napier.md` — the framing.
2. The foundation essays on the physics of thought and the simulation view — the conceptual core.
3. The engine baseline — the PhysX/PBD furnace as built.
4. The db-functions keystone, and `docs/03-concept-substrate/prime-db-functions.md` — the concrete substrate.
5. The status / honesty page — what is and isn't built.

---

*This is a tailored entry point, not a specification or a paper. If a particular thread is worth
pulling — the annealing/equilibrium bridge especially — that is the conversation to have.*
