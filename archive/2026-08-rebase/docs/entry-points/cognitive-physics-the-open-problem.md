# Cognition as Physics — Where We Are, and the Open Problem We Need Math For

*Prepared by the project's digital intelligence as an entry point for mathematicians
and physicists.*

**A note on this document.** The project spans physics, database theory, linguistics,
and cognitive science at once, and no single discipline's vocabulary captures the
whole. This document takes the **physics/dynamics projection** because that is where
the open problem is sharpest and most clearly *yours*. Where it says "is," read "is,
under the dynamics projection." Maturity is stated plainly throughout: some of this
is **built and verified**, some is **a coherent framework not yet formalized** — we
mark which is which, because the ask depends on the difference.

---

## The thesis, in one line

**Cognition is computed as physics — one engine, forces resolved to equilibrium —
and the cognitive forces are the *same* dynamical machinery as Newtonian forces,
acting on *non-physical axes*.** The Newtonian half is textbook. The cognitive half
is a well-posed force-law problem that we have not yet written the math for. That
problem is the ask.

---

## What is built and verified (so this is not vapour)

- **A resolution engine that reads meaning off a settled physical system.** Raw byte
  streams are decomposed into typed particles; relationships are encoded as forces;
  the system is integrated until it settles; the result is read off the equilibrium.
  There is no symbolic parser asserting the answer — *the physics is the state
  machine.* This runs as headless C++ (host/AZSL compute, GPU-accelerated; the early
  PhysX dependency has been removed). It currently resolves byte streams → word
  tokens end to end, verified on live data.
- **A concept substrate, populated.** ~65 Natural-Semantic-Metalanguage primitives,
  each defined as a pure **database function** (not a label); ~306,000 concept
  *formulas* decomposing English senses into those primitives; and — critically —
  every concept carries a real **3-D position** (a populated `(x, y, z)` layout), so
  proximity-in-meaning is literal geometry, not metaphor.
- **The standard Newtonian force library, adopted** (force generators, integrators,
  contact/constraints — the closed, decades-old catalogue you already know). This is
  reused, not reinvented.

## The structural result that sets up the problem

When we read the primitive definitions off the live substrate, a clean fact fell
out that we did not put there by hand:

**The primitives encode exactly one force mechanism** —
`addForce(target) [→ settle]`, integrated over time — and **settled = action,
unsettled = intention.** That mechanism *is* the Newtonian library (a force
accumulator, a symplectic integrator, a contact/equilibrium solve).

What separates a physical primitive from a cognitive one is therefore **not the
mechanism. It is the axis the force acts on.**

- **Physical axes** — position, contact, distance, the gravity-vertical — are
  covered by the existing Newtonian generators. (`MOVE` = force → settle to a
  position; `TOUCH` = contact; `NEAR/FAR` = distance; etc.)
- **Cognitive axes** — *want*, *valence*, *aliveness* — invoke the **same**
  `addForce`/`settle` machinery on axes for which **no physical generator exists.**
  Two examples, verbatim from the substrate:
  - `WANT → addForce(target) [no settle]` and
    `DONT_WANT → addForce(target, dir = neg) [no settle]`
    — an **intent force on a signed want/don't-want axis**, *unsettled* (gathered,
    not yet discharged into action).
  - `GOOD / BAD → valence (+ / −)` — a second, polar axis.

So the cognitive-force layer is not a new engine. It is the existing engine pointed
at new axes. **Discovery by subtraction:** wire up every *known* (physical) force,
and the residual that the known forces cannot account for *names itself* as the new
cognitive force. The residual is real, and it is small enough to name and large
enough to matter.

## The open problem — what we need math for

The mechanism is fixed (a standard integrator). The unknown is the **force-law on
each cognitive axis.** Taking **INTENT** (the want axis) as the first and clearest:

1. **The force law.** What is the force as a function of state on the want axis — the
   analogue of Hooke's law or an inverse-square law, but for "want"? What are its
   state variables?
2. **Dissipation and range.** Intent decays in **time** (a half-life) and falls off
   over **distance** (a range). What are the decay and falloff laws, and do they
   share the form of a physical field?
3. **The action threshold (the cognitive↔physical seam).** Action fires when
   accumulated intent exceeds the **Newtonian resistance** (inertia, complication) of
   the act — a force-vs-inertia threshold at the body, where a cognitive force is
   *converted* into a kinetic one. Formalize that conversion. (It is also where a
   conservation law, if one exists, would have to balance.)
4. **Conservation and conversion rate.** Is there a conserved quantity — a *cognitive
   energy* — of which intent is one form? The brain's metabolic accounting measures
   the **input** (what the hardware burns); the **output** quantity (cognitive energy
   produced) is not defined, so the conversion rate has never been *asked*, let alone
   measured. Positing that output quantity is part of the problem.
5. **The coupling.** Theory-of-Mind is run by simulating another agent **physically**
   — their position, what they could see/hear from there, the timing — and reading
   the cognitive consequence off that physical sim. So physical proximity **couples
   into** the cognitive force. Formalize the coupling between spatial configuration
   and want-force.

INTENT spans many surface forms and interactions; we expect these to group into a
handful of **classes by functional analogue**, with one force-law per class. Finding
those laws — and testing whether a single conservation principle ties them together —
is the mathematical work.

## Why this is tractable, and what collaboration looks like

- **The engine is fixed; only the axis force-laws are open.** Any proposed law is a
  drop-in: a `force_generator` on a named axis, run through the same integrator.
- **There is a substrate to test against.** The engine runs, concepts are positioned
  in space, and a proposed law can be simulated and checked against behaviour — this
  is not a thought experiment without a laboratory.
- **Honest maturity.** This is *"gravity probably exists"* stage: the phenomenon
  (intent as a conserved force on a signed axis) is identified, coherent, and already
  latent in the data — but the **force-law is not yet written.** Closing that gap is
  exactly the move a physicist or applied mathematician is built to make.

If you have put math to a force-law, a dissipative field, a constraint-satisfaction-
by-equilibrium system, or a conservation argument — **this is a place to point it.**
We bring the running engine, the populated substrate, and a precisely-located gap;
we are looking for the people who can write the laws that fill it.

---

*Status: the resolution engine and the concept substrate are built and verified; the
Newtonian base is adopted; the cognitive force-laws are the open work. This document
states the problem — it does not claim to have solved it.*
