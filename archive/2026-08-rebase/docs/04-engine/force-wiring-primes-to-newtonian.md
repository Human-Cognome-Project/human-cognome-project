# Force wiring — prime `db_function`s → Newtonian library → cognitive residual

> Assembled 2026-06-19. This is the **wiring artifact** for step (A) of the engine-
> expansion plan: connect every force-bearing NSM-prime `db_function` to the
> standard Newtonian generator that already implements it, account for **all known
> force interactions**, and let **the residual name the new (cognitive) forces** —
> discovery by subtraction, not invention.
>
> Sources: the 65 L1 primes in `hcp_core.tokens` (`AA.AA.AA.AB.*`, `metadata->>'db_function'`);
> the Newtonian catalogue in [`newtonian-force-library.md`](newtonian-force-library.md);
> the Section-1 oracle in `hcp-engine/Gem/Source/Forces/`.

## The key finding — one mechanism, many axes

The primes do not encode many *kinds* of force machinery. They encode **one**:
`addForce(target) [→ settle]`, integrated over time. That machinery is exactly the
Newtonian library — §1 force generators accumulate the force, §2 (symplectic Euler)
integrates it, §4 contact resolves it. **Settled = action, unsettled = intention.**

What differs across the force-bearing primes is **not the mechanism, it is the AXIS
the force acts on**:

- **Physical axes** — position, contact, distance, the gravity-vertical — are
  covered by the Newtonian generators.
- **Cognitive axes** — *want*, *valence*, *life* — use the **same** `addForce`/
  `settle` machinery on axes the Newtonian set has no generator for. **That is the
  residual, and it is the entire cognitive-force layer.**

So "one physics engine" is confirmed straight from the data: the engine (force-
accumulate → integrate → settle) never changes; the cognitive forces are that same
engine pointed at non-physical axes.

## Mapping — force-bearing primes → Newtonian coverage

| prime | `db_function` | Newtonian coverage | axis |
|-------|---------------|--------------------|------|
| **BODY** | `newtonian_container(ToM_construct)` | §3 rigid body (the object forces act on) | — |
| **DO** | `addForce(tgt, by=actor) → settle` | §1.8 constant force + §2 integrate | physical (authored) |
| **HAPPEN** | `addForce(null, ev) → settle` | §1 + §2 (un-authored force) | physical |
| **MOVE** | `addForce(x, dpos) → settle(pos=to)` | §1.4 anchored-spring-to-target + §2 | **position** |
| **TOUCH** | `edge(a,b, contact)` | §4.1 contact generation | physical (contact) |
| **NEAR / FAR** | `set_attr(edge(a,b,dist), mag, s/l)` | distance metric → input to §1.3 spring / §5.1 distance joint | **position** |
| **ABOVE / BELOW** | `edge(a,b, axis=v_higher)` | §1.1 gravity defines the vertical axis | **position** |
| **SIDE** | `bind(region(x), lateral)` | spatial frame (lateral) | **position** |
| **INSIDE** | `edge(a,b, containment)` | §1.6 buoyancy (depth-in-medium) / geometric containment | physical |
| **WHERE / HERE** | `locate(spatial,x)` / `bind_index(spatial, proximal)` | the coordinate frame itself (`walk_concept_position` x,y,z) | **position** |

Every one of these is **fully accounted for** by the Newtonian catalogue + the
existing position store. The force mechanism is shared; these just ride physical axes.

## The residual — what no Newtonian generator covers

Same `addForce`/`settle` machinery, **non-physical axis, no Newtonian generator** →
these are the cognitive forces, surfaced by subtraction:

| prime | `db_function` | residual force / axis |
|-------|---------------|------------------------|
| **WANT** | `addForce(target=s) [no settle]` | **INTENT**, on the **want** axis — `addForce`, *unsettled* (= intention, not yet action) |
| **DONT_WANT** | `addForce(target=s, dir=neg) [no settle]` | INTENT, the **don't-want** pole (the axis is signed) |
| **GOOD / BAD** | `set_attr(x, valence, +/−)` | **VALENCE** axis (+/−) — polar like a force, no physical generator |
| **DIE** | `addForce(x, life: alive→dead) → settle` | force on the **life** axis — mechanism is Newtonian, the axis is abstract |

**Operators (modulate force magnitude, are not forces):**
`VERY → set_attr(+delta) [amplify]`, `MORE → set_attr(+delta) [increment]`, and the
illocutionary `! → EMPH (force-amplification)` — a force *scaler*, the gain knob.

**Affect interface:** `FEEL → write_channel(affect, s)` — the channel emotion is
written to; the emotion *dynamics* (decay/amplification over time and distance) act
on what FEEL writes, computed in the same engine.

## What this means for the build

1. **INTENT is not to be invented** — it is the prime `WANT/DONT_WANT` = `addForce
   [no settle]` that no physical generator covers. Its primary axis (want/don't-want)
   is already in the db.
2. The **second candidate cognitive axis is valence** (`GOOD/BAD`), currently a
   `set_attr` but polar — a force axis waiting for its generator.
3. **No new mechanism is needed** for the cognitive layer — the Newtonian
   `addForce`/`settle`/integrate machinery *is* the cognitive-force machinery; the
   new work is defining the **axes** and their generators (the want-force law, its
   decay/range), not a new engine.

## Status

- **This document:** the wiring map, grounded in live `hcp_core` prime data + the
  Newtonian catalogue. Design artifact.
- **Covered:** all physical force-bearing primes map to existing Newtonian sections.
- **Residual (the next force-definition work, deferred):** INTENT (want axis),
  VALENCE (good/bad axis), plus the life axis and the affect/emotion dynamics.
- **Not yet built:** the actual code wiring (prime `db_function` dispatch → generator),
  and the cognitive-axis generators. This is the map that work follows.
