# Cognitive-force placeholders — close-enough Newtonian analogues

> Purpose: the rigorous cognitive force-laws are open (see
> [`../entry-points/cognitive-physics-the-open-problem.md`](../entry-points/cognitive-physics-the-open-problem.md)).
> But many cognitive factors have a **close-enough Newtonian analogue** that works as a
> runnable **placeholder** — it reuses existing machinery, lets the engine run *now*,
> and gets swapped for the real force-law when one is derived. This is the "establish
> analogues to class the forms" step. It **precedes** the math; it does not replace it.
>
> Every row is a placeholder until its force-law is written. Flagged as such on purpose.

## The map

| cognitive factor | Newtonian analogue | reuses | where it slots | status |
|---|---|---|---|---|
| **habit** | **inertia / mass** | the mass term in the integrator (`F = ma`); `damping` for persistence | the **resistance** term in the action threshold — intent must exceed it to change a behaviour; once changed, inertia carries it | placeholder |

*(growable — rows added as factors are placed)*

## Notes per row

### habit → inertia
A behaviour's habit-strength is modelled as its **mass**. Changing the behaviour
requires intent-force to exceed that mass; once the behaviour is moving (established or
broken), inertia keeps it going. This matches lived dynamics on both ends: hard to
*start* a new habit (overcome rest inertia) and hard to *stop* an old one (overcome
moving inertia). No new machinery — it is the mass term already in the integrator.

**What the placeholder does *not* yet capture (the real force-law, open):** whether
habit-mass **grows with repetition** and **decays with disuse** — i.e. whether mass is
dynamic rather than fixed. Plain inertia is constant mass; habit clearly is not. That
dynamic is the actual law to derive; the constant-mass placeholder is the runnable
stand-in until then.

## How to read this file

- A placeholder is **good enough to run and to reason with**, and **wrong in the
  details** by definition. Use it; don't trust it past its slot.
- When a real force-law lands for a factor, replace its row and move the analogue note
  to a "superseded placeholders" record so we keep the history of what we leaned on.
