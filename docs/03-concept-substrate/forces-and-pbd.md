# Forces, PBD Settling, and the English Skin

Primes are **forces** before they are concepts (the ontological layer of the semiotic triangle,
claim 223). This page covers how molecule definitions emerge from force settling, why forces are
level-of-detail-relative, and the honest status of the "force notation" work.

Sources: claims 213 (explication as PBD settling), 214 (articulation trees), 227 (forces are
LoD-relative), 241 (force notation is a rough English skin), 216 (PBD substrate channels).

---

## Molecule definitions emerge from settling — not a promotion event

> *Rigid-body / molecule emergence from prime-particles is **PBD equilibrium settling**, not a
> discrete promotion event.* — claim 213

You let the position-based-dynamics forces settle and **read the equilibrium state** — which *is*
the molecule definition. Readiness is detected via convergence signals (a Von Mises stress
threshold, or velocity-convergence), not a callback. **The physics is the state machine** — there
is no separate orchestration layer deciding when a structure is "done."

Nested explication sub-structures (deep molecule decomposition) ride on PhysX articulation trees
(`PxArticulationReducedCoordinate`, claim 214): each articulation link is a sub-molecule carrying
its own predicate, and the reduced-coordinate solver propagates force correctly through the tree
to essentially unbounded depth. (Engine substrate detail is in
[../04-engine/resolution-furnace.md](../04-engine/resolution-furnace.md).)

---

## Forces are level-of-detail-relative, not absolute

> *Grammatical/conceptual forces are LoD-level-specific and aggregate **upward** (word → phrase →
> clause → sentence): at each level the lower-level detail is consumed, and force values are
> contextual **within** that level, never globally absolute.* — claim 227

This is greedy-LoD (claim 16) applied to force evaluation: the *same* force type has a different
magnitude and relevance at different scales, evaluated relative to the active level. There is no
global force constant; a force value only means something *at a stated LoD.* This is
**aspect-neutral** — it holds regardless of how the linguistic/conceptual done-state shakes out
(claim 236).

---

## The honest status of "force notation"

> ## ⚠ Rough, not refined — and it is the *English skin*, not the concept base
>
> *The current force notation / force-propagation work is **rough ideas extracted from grammar**,
> not properly refined. It is specifically the construction of a proper **English-text expression
> skin** over the conceptual base — the linguistic-expression layer, NOT the conceptual base
> itself.* — claim 241

Two things this resolves, and you should carry both:

1. **It is rough.** The concrete force types and bond-strength constants that appear in older
   research notes (e.g. `english-force-patterns`, the grammar-identifier color/tone PoS encoding)
   are *rough extractions*, not a settled force model. Do not present specific constants as the
   spec.
2. **It is a *different aspect*, not superseded.** The Feb force-constant work and the May prime/
   force work are **different aspects of the same thing**, not one superseding the other (claim
   236). Their done/not-done status is an **open review item.** Do not mark the older
   force-constant material "superseded."

Why English is mined for this at all: English, *for all its irregularity — or because of it — has
one of the clearest db-construction structures of any language* (claim 241). It is the worked
example mined for the universal substrate (claim 257, see
[../02-architecture/linguistic-vs-conceptual.md](../02-architecture/linguistic-vs-conceptual.md)),
and the messy flat-file concept-smear of this very material was a prime driver of the move to the
distilled claim-graph. Refining the skin is part of the upcoming linguistic/conceptual
current-state review.

---

## Where this connects

- The **substrate channels** that actually carry force/relational parameters on the GPU
  (velocity.w, 20-bit phase-group equality, the GPU-readable PBDMaterial subset) are documented as
  engine detail in [../04-engine/resolution-furnace.md](../04-engine/resolution-furnace.md)
  (claim 216).
- Force evaluation feeds the **inference** (determination) side, not the resolution furnace — see
  [../04-engine/overview.md](../04-engine/overview.md) for the two-sides split.

---

## See also

- [primes-and-molecules.md](primes-and-molecules.md) — what settles into what.
- [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) — the force-skin refinement
  and the linguistic/conceptual done-state are tracked open items.
