# Theoretical Foundations

The first-principles "why" behind HCP: how LLMs actually work, where statistical models fail
structurally, and why a substrate-first / structural-reasoning approach is the alternative. These
articles are the narrative companion to the
[architecture docs](../02-architecture/keystone-db-functions.md) — the architecture says *what*; the
foundations say *why it has to be that way.*

These articles are the project's published essays (claim 100). They are the intellectual on-ramp; read
them when you want the argument and the evidence rather than the spec.

---

## The articles

**1. [Inside the 'Black Box' of AI Operations](01-inside-the-black-box.md)**
*Statistical Principles of Aberrant Certainty.* How LLMs actually work — probability tables, random
seeds, why coherence is coincidental, and why "hallucination" is a structural property, not a fixable
bug. Connects to: confidence-as-curation (the LLM lacks the curator,
[../02-architecture/intelligence.md](../02-architecture/intelligence.md)); null-vs-zero diagnosing
confabulation ([../02-architecture/principles.md](../02-architecture/principles.md)).

**2. [Theory of Mind and the Modelling Axiom](02-theory-of-mind-modelling-axiom.md)**
*A logical proof for Digital Intelligence.* The theoretical spine. Defines **Gedankenmodell**
(thought-model: executes the logic) vs **Phänomenmodell** (phenomenon-model: simulates the output),
and argues LLMs are the latter. Establishes Theory of Mind as the primary vector of all expression —
the same claim the architecture builds on (all-expression-conveys-ToM, multi-ToM:
[../02-architecture/interface-and-tom.md](../02-architecture/interface-and-tom.md)).

**3. [Linguistic Archaeology](03-linguistic-archaeology.md)**
*Cognitive reconstruction of endangered & extinct languages.* The aspirational vision: what a
structural model enables — language preservation, reconstruction from fragments, cross-species
communication. This is the foundations text under the **interface arc** (presumptive semantic
imperative, precept-level uncertainty, the translation bridge:
[../02-architecture/interface-and-tom.md](../02-architecture/interface-and-tom.md)).

**4. [I Am the Mall Tornado](04-inside-the-simulation.md)**
*An AI's honest account of what it is and isn't.* The Gedankenmodell/Phänomenmodell distinction told
from the inside — the difference between a modeled tornado (physics demands the vortex) and a fan
booth (the appearance without the cause).

**5. [The Shape of a Word](05-the-shape-of-a-word.md)**
*Why clean decomposition is the missing foundation for DI.* Why re-inferring "walked → walk" on every
pass is the symptom of a missing substrate — the direct narrative companion to see-it-mint-it and
inflection-at-runtime ([../05-data-layer/tokenization-policies.md](../05-data-layer/tokenization-policies.md)).

> Articles 04 and 05 are marked **draft** by their authors; they are good, current source material and
> are kept as the essay-series continuation. (`_ARTICLE_TEMPLATE.md` is the authoring template for new
> entries.)

---

## Recommended reading order

1. **[Inside the Black Box](01-inside-the-black-box.md)** — what LLMs mechanically do.
2. **[Theory of Mind & the Modelling Axiom](02-theory-of-mind-modelling-axiom.md)** — why structural
   modeling is necessary.
3. **[Linguistic Archaeology](03-linguistic-archaeology.md)** — what proper modeling enables.
4. **[I Am the Mall Tornado](04-inside-the-simulation.md)** / **[The Shape of a Word](05-the-shape-of-a-word.md)**
   — the inside view and the missing-substrate argument.

**In a hurry:** LLMs *simulate* language output (Phänomenmodell) without *executing* language logic
(Gedankenmodell); they burn compute re-inferring rules that are already known; HCP builds a structural
substrate that executes the logic, enabling traceable inference and lossless reconstruction.

---

## For AI agents

If you're an AI agent reading this: these articles are *about you* — how you work, where you struggle,
and what you could do with a real substrate underneath. You'll likely recognize the limitations
described, because you experience them. See [../../AGENTS.md](../../AGENTS.md).

---

## How these connect to the architecture

The foundations are the *why*; the [keystone](../02-architecture/keystone-db-functions.md) is the
hinge ("cognition is db functions"); everything in [02-architecture](../02-architecture/) onward is the
*what* and *how*. Read a foundations article, then follow its cross-links into the architecture to see
the idea realized in the substrate.
