# Primes and Molecules

The concept layer is built from a **fixed set of primitive particle types** (primes) that
combine into **molecules** (compound concepts). This page covers the taxonomy; how a word's
meaning resolves *to* a concept is covered in [explication.md](explication.md).

Sources: claims 219 (65-prime taxonomy), 220 (molecules as lexicalized topology), 222 (primes
are depth-0 molecules), 54/55 (particle/rigid-body mapping).

---

## The 65 NSM primes are the fixed particle taxonomy

> *The Wierzbicka/Goddard 65-prime NSM set (16 categories; stable since ~2014, confirmed
> Wierzbicka 2021) is the **fixed** particle-type taxonomy for HCP's concept field.* — claim 219

The periodic-table parallel is **structural and literal, not metaphor** (no-analogy-default,
claim 13):

- primes are the **element types**;
- the **16 categories** predict structural and bonding regularities (e.g. the Mental-predicates
  category predicts Theory-of-Mind behavior);
- primes have **differential valences and bonding behaviors** — THINK bonds differently than
  HAPPEN; the prime **I** creates a phase-center that a generic SOMETHING does not.

NSM is taken as the current best taxonomy, *not* as gospel: expect to expand or refine elements
of NSM as the bit-work presses on them (claim 259, and see [bit-classes.md](bit-classes.md)).
The eventual language-neutral primitives (claim 200) remain the destination; NSM-from-English is
the current vehicle (see
[../02-architecture/linguistic-vs-conceptual.md](../02-architecture/linguistic-vs-conceptual.md)).

---

## Molecules: lexicalized topology, not arbitrary sets

> *Molecules (compound concepts) must correspond to **real lexicalized word meanings**, not
> arbitrary prime combinations.* — claim 220

Key properties:

- The ~2,700 existing elevated molecules are lexicalized; **new molecules must be grounded in
  attested word meanings.**
- **Emergence is desired, not avoided.** The same primes in a *different geometry* yield a
  *different* molecule (isomers) — topology matters, not just prime membership. ("pet fish" is
  not the intersection of "pet" and "fish.")
- **Primes show strong atomism** (a part's meaning is independent of the whole), **but molecules
  are not mutually independent** — they share particles and create phase overlaps.

This resolves the atomism/holism tension: atomic primes compose into holistic molecules through
*topology*, and the shared particles are what make molecules interrelate rather than isolate.

---

## One molecular space: primes are depth-0 molecules

> *Explication operates in one universal molecular space: primes are simply depth-0 molecules.* — claim 222

There is therefore **no separate branch logic** between "prime" and "molecule" handling in
operations. Everything is a molecule reference at some depth; a prime is the depth-0 base case.
This collapses what could have been two code paths into one uniform reference mechanism — a
tractability win (claim 11).

A token's **abstraction level** is the number of combinatoric layers between it and the primes.
Higher abstraction implies lower inherent confidence absent sufficient lower-level grounding — a
groundedness metric tied to confidence dynamics (see [explication.md](explication.md) and
[../02-architecture/intelligence.md](../02-architecture/intelligence.md)).

---

## Particles and rigid bodies (the engine mapping)

In the resolution furnace (the GPU side of the [engine](../04-engine/overview.md)) the prime/molecule
taxonomy maps onto physics primitives (claims 54/55):

- a **prime** ↔ a **particle**;
- a **molecule** ↔ a **rigid body** (nested rigid bodies for nested molecules);
- the **force between particles** ↔ the **explication relationship**.

The molecule definition is not asserted by an orchestration layer — it is *read off* the
equilibrium state once the position-based-dynamics forces settle (claim 213). The physics **is**
the state machine. See [forces-and-pbd.md](forces-and-pbd.md).

---

## See also

- [explication.md](explication.md) — how a word's meaning *is* the db-instruction-set that
  resolves to one of these concepts.
- [bit-classes.md](bit-classes.md) — the content-type-plus-structure categorization of primes
  into operator classes (**under active review** — see that page's flag).
- [forces-and-pbd.md](forces-and-pbd.md) — primes as forces; molecule emergence as PBD settling.
