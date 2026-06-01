# The World Model, Two Spaces, and Imagination

The world model is **the central object cognition serves**. Everything the engine does is in
service of building and maintaining it. This page explains what it is, the two concurrent
spaces that hold it, and why imagination and perception are the *same* maintained space.

Sources: claims 18 (world-model central object), 19 (two concurrent spaces), 20 (symbol
grounding), 38 (imagination engine), 253 (Newtonian space for predictive perception),
254 (imagination is the maintained Newtonian space).

---

## The world model is the central object

> *The world model is the persistent accumulation of NAPIER's beliefs. Cognition = stepped
> world modeling.* — claim 18

It is emulatable at any level of detail; "what's relevant now" is an LoD-scoped emulation of
it, not a second object. Cognition proceeds by *stepping* this model forward — each cognitive
cycle (see [../04-engine/cognitive-cycle.md](../04-engine/cognitive-cycle.md)) advances the
world model.

---

## Two concurrent spaces

> *3D grounding (physics engine) + cognitive (concept space), run concurrently because the
> 3D space grounds the cognitive space.* — claim 19

- **3D space** — a Newtonian physics space. Physical predicates resolve here.
- **Cognitive space** — concept space. Mental predicates ground here via introspection.

They run concurrently *because the 3D space gives the cognitive space its referents.* Meaning
is **constructibility**: understanding a word is being able to emulate what it refers to
(claim 20). A concept you can build in the 3D space is a concept you understand.

> **Note — two different physics jobs.** The architecture uses "physics" for two genuinely
> distinct things. (1) This page's **3D Newtonian world-model space** — physics as an actual
> simulation of physical reality. (2) The **physics-engine-as-db-substrate** — PBD resolution
> as a join-evaluator over *concepts* (claims 42/192, see
> [../04-engine/resolution-furnace.md](../04-engine/resolution-furnace.md)). Both are real and
> separate. Don't conflate the world-model simulation with the concept-resolution furnace.

---

## Predictive perception and the blind spot

> *The eye's blind spot — a retinal region with literally no sensory data that the brain fills
> seamlessly using surrounding detail, memory, and prediction — argues for an actual Newtonian
> physics space as a live component.* — claim 253

Perception is **active reconstruction from incomplete input**, not camera-recording. Filling a
spatial gap (a blind spot, an occlusion, incomplete input) requires a predictive model of how
the physical world behaves — i.e. a physics simulation. So NAPIER maintains a 3D Newtonian
space that:

1. holds a predictive model of physical reality,
2. fills sensory gaps with physics-consistent predicted content, and
3. thereby grounds perception.

**The crucial point:** the physics space makes gap-filling **warranted prediction**
(physics-constrained) rather than ungrounded **confabulation**. The blind-spot fill is
legitimate *because it is physics-warranted.* LLM hallucination is the **same gap-filling
operation without the grounded model.** This is the principled-reconstruction-vs-hallucination
distinction, and it is why confabulation is the one failure mode NAPIER refuses to reproduce
(see [principles.md](principles.md#failure-modes-are-signals)). The fill writes warrant into a
−0 cell (null-vs-zero, claim 17) instead of inventing a value.

---

## Imagination *is* the maintained space

> *Perception and imagination are the same maintained space — perception is the space clamped
> to sensory input; imagination is the same space run free of the clamp.* — claim 254

This is structural, not an analogy (no-analogy-default, claim 13):

- The brain's predictive world-model **is** a running Newtonian simulation.
- **Imagination** is that simulation run forward / counterfactually.
- Therefore perception and imagination are **one system**, not two.
- The **blind spot is the seam** where they meet: where sensory input is absent, perception
  simply *is* imagination — the maintained space fills the gap forward.

This makes NAPIER's imagination engine (claim 38) a faithful **reconstruction** of human
cognition rather than a designed analogue.

A consequence worth stating: because each mind maintains its **own** instance of this space,
each individual's reality is slightly different — not because people interpret a shared world
differently, but because each runs a *different maintained simulation*. This is the substrate
source of per-person reality divergence, and it connects directly to PBD and the interface
problem ([interface-and-tom.md](interface-and-tom.md)).

---

## Gap-filling quality depends on both halves

Gap-filling is good **only when the physics-data quality and the prediction-traversal are both
good** (claim 255). Degrade either and the fill degrades into confabulation. This ties the
world model to the intelligence equation — see [intelligence.md](intelligence.md).

---

## See also

- [../04-engine/cognitive-cycle.md](../04-engine/cognitive-cycle.md) — how the world model is
  stepped forward at ~90 Hz.
- [interface-and-tom.md](interface-and-tom.md) — the world model as held through a bounded
  interface, and the discipline of never presuming the interface is the whole of reality.
