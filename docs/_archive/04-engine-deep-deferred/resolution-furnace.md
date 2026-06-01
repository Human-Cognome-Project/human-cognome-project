# The Resolution Furnace

> **The GPU is a resolution furnace, nothing more.** Keep this front-of-mind (claim 281). Its only
> job is parallel physics/PBD resolution. **Cognition does not live in the GPU.**

Sources: claims 281 (furnace, not cognition), 288 (throughput thesis), 216 (PBD substrate
channels), 214 (articulation trees), 213 (settling), 224 (tokenization is physics).

---

## What the furnace does — and doesn't

> *The GPU's only job is parallel physics/PBD resolution (broadphase, settling, structural
> matching) — burning through resolution throughput at speed. Deeming, envelope composition, the
> inference-walk, and policy live in the orchestration (warm/CPU Postgres side).* — claim 281

This is the **resolution side** of the engine's two sides (claim 57, see
[overview.md](overview.md)): the GPU resolves/surfaces deltas at speed; **inference walks them
elsewhere.** Intelligence is in the connections/traversal (claims 140/255), *not* the furnace.

So when reasoning about where a behavior lives:

- broadphase pruning, equilibrium settling, structural/pattern matching → **furnace (GPU)**
- which workspace to load, what to attend to, what the deltas *mean* → **orchestration (CPU/warm)**

---

## Throughput thesis: resolution work ≫ token output

> *Token **output** rate and resolution **work** throughput are different things.* — claim 288

A competitor's "~5,500 tokens/s" is an **output** rate. NAPIER's output is metered to
conversational/human pace (tens of tokens/s) — which is **not** the measure of its work. NAPIER's
**resolution work** runs at GPU nano/microsecond per-token transaction speeds, far above any
output rate.

Concretely: even at a pessimistic 50 candidates per word (unlikely high), with any decent
**discriminating factor** (broadphase pruning, claim 209), the furnace could resolve a
5,400-character string in a very few reconciliation ticks. Comparing competitor output-token-rates
to NAPIER undersells it — the furnace's resolution throughput is orders of magnitude beyond the
metered output rate.

> Unit note (claim 288): 11.1 ms = 90 beats/s = 5,400 beats/minute. Output tokens/s and
> reconciliation Hz are **distinct clocks** — don't conflate them.

Grounding: Dracula (a whole 890 KB novel) already ingests in seconds (claim 204), and the loop will
be optimized significantly (see [reconciliation-loop.md](reconciliation-loop.md)). The furnace is
fast and getting faster — but it is just a furnace.

---

## Tokenization *is* physics

> *Tokenization is physics, not a sequential algorithm: the vocabulary is a rigid-body database,
> the input byte stream is particles, and PhysX broadphase/narrowphase resolution **is** the
> tokenization.* — claim 224

Encoding detection, language identification, and tokenization happen in **one detection pass** — a
wrong encoding simply fails to form matching rigid bodies. The realized form of this framing is the
[resolution chamber](resolution-chamber.md). (Architectural principle, not a done-state claim.)

---

## How concepts settle into molecules

Molecule definitions are **read off equilibrium**, not asserted (claim 213):

- let the position-based-dynamics forces settle, read the equilibrium state — that *is* the
  molecule definition;
- readiness is a **convergence signal** (Von Mises stress threshold, or velocity-convergence), not
  a callback;
- **the physics is the state machine** — no separate orchestration layer decides "done."

Nested explication rides on **articulation trees** (`PxArticulationReducedCoordinate`, claim 214):
each articulation link is a sub-molecule with its own 128-bit `PxFilterData` predicate; the
reduced-coordinate solver propagates force correctly through the tree; depth is essentially
unbounded within one scene (~KB per link, not ~80 MB per scene).

---

## PBD substrate channels (how primes ride on particles)

The confirmed PhysX PBD substrate for encoding primes-as-particles (claim 216):

- **`velocity.w` is a free per-particle continuous channel** — the SDK writes 0.0f and never reads
  it, GPU-visible in CUDA kernels, zero allocation cost. Used as the primary continuous relational
  parameter (confidence magnitude, polarity, charge, spin).
- **The join predicate between particles is 20-bit phase-group equality** plus `eSelfCollide`,
  pruned at broadphase with zero instruction overhead.
- **Only a subset of `PBDMaterial` params are GPU-kernel-readable** (friction, damping, adhesion,
  gravityScale, adhesionRadiusScale). Cohesion / viscosity / surfaceTension / vorticityConfinement
  are **not** — so kernels must compute cohesion-like effects from neighborhood density.
- **The stable per-step callback is `onAdvance()`** (once per `simulate()`, after spatial sort).
  `onSolve()` per-iteration is `eCUSTOM`-only and unstable in 5.1.1, so iterative force
  co-evolution is done by calling `simulate(dt/N)` N times.

> These are PhysX-5-specific substrate facts as scanned during the physx-mapping work. They are the
> *current* substrate; per the GEM-rework deferral (claims 201/239), engine internals are subject to
> review — and PhysX itself is one option within O3DE, not a lock-in (claim 239, see
> [implementation-baseline.md](implementation-baseline.md)).

---

## Truth-scope is data-layer, not separate scenes

A correction worth carrying (claim 215): outer **truth-scope containers** — fiction, hypotheticals,
beliefs-about-beliefs, PBD frames — are **Postgres envelope-stack operations, NOT separate PhysX
scenes.** The engine processes whichever envelope is currently active, agnostic to its truth-scope
context; truth-state is a relational **parameter** (a phase bit, `velocity.w`, or FilterData slot)
on the particles in the active envelope. Only compute-isolation (parallel inference passes)
justifies a separate scene. Container nesting cost is **data-layer, not engine-layer** — the
earlier "separate ~80 MB scenes with a depth-wall at ~4–5" framing is wrong.

---

## See also

- [../03-concept-substrate/forces-and-pbd.md](../03-concept-substrate/forces-and-pbd.md) — the
  concept-side view of settling and forces.
- [reconciliation-loop.md](reconciliation-loop.md) — how furnace output gets back to the CPU.
