# Newtonian force library — the standard, reusable catalogue

> Assembled 2026-06-19. This is the **standard Newtonian physics function set** —
> the well-known, decades-old, reusable library that every physics/game engine
> reimplements. It is *not* novel and *not* HCP-specific: it is the Newtonian
> **base** the engine builds on. The novel (cognitive) forces are a separate,
> later layer and are deliberately **not** in this document.

## Why this exists

The standard force/integration/contact set is a closed, finite, textbook body of
work with battle-tested permissive implementations — you **adapt** it, you do not
invent it. The primary named-function source is **Ian Millington's Cyclone
engine** (`idmillington/cyclone-physics`, MIT), which structures physics exactly
as a registry of named *force generators*. Cross-references fill gaps:

| source | license | role |
|--------|---------|------|
| [`idmillington/cyclone-physics`](https://github.com/idmillington/cyclone-physics) | MIT | primary: force generators, integrators, rigid-body rotation, rod/cable |
| [`erincatto/box2d`](https://github.com/erincatto/box2d) (v3) | MIT | contact solver (TGS-soft sequential impulse), joints (revolute, weld, distance) |
| [`bulletphysics/bullet3`](https://github.com/bulletphysics/bullet3) | zlib | narrow-phase GJK/EPA, sequential-impulse constraint solver |
| [`slembcke/Chipmunk2D`](https://github.com/slembcke/Chipmunk2D) | MIT | arbiter/friction reference, contact persistence |
| XPBD (Macklin, Müller, Chentanez) | paper | position-based / compliant constraint solve |

---

## Section 1 — Force generators (particle level)

Cyclone models these as a registry of named `ParticleForceGenerator` subclasses,
each `updateForce(particle, dt)` accumulating into the particle's force sum. This
**is** the "library of basic Newtonian functions." All MIT, in `src/pfgen.cpp`.

| # | Name | Equation | Cyclone source |
|---|------|----------|----------------|
| 1.1 | **Gravity** | `F = m·g` (g a constant accel vector) | `ParticleGravity::updateForce` |
| 1.2 | **Drag** (linear + quadratic) | `f = -v̂·(k1·‖v‖ + k2·‖v‖²)` | `ParticleDrag::updateForce` |
| 1.3 | **Spring / Hooke** (two particles) | `F = -k·(‖d‖ − l₀)·d̂`, `d = x_self − x_other` | `ParticleSpring::updateForce` |
| 1.4 | **Anchored spring** | as 1.3 with a fixed world anchor | `ParticleAnchoredSpring::updateForce` |
| 1.5 | **Elastic bungee** (pull-only) | `‖d‖ ≤ l₀ → 0`; else `F = -k·(‖d‖ − l₀)·d̂` | `ParticleBungee::updateForce` |
| 1.6 | **Buoyancy** (depth-based) | above water → 0; fully submerged → `F_y = ρ·V`; partial → linear ramp | `ParticleBuoyancy::updateForce` |
| 1.7 | **Stiff / damped spring** | closed-form damped harmonic toward anchor | `ParticleFakeSpring::updateForce` |
| 1.8 | **Constant force / thrust** | `F = const` | trivial `addForce(const)` |

**Implementation note (a real correctness fork):** Cyclone's *two-particle*
`ParticleSpring` uses `abs(‖d‖ − l₀)`, which makes a **compressed** spring also
*pull* (a known quirk of the book's code). The engine port uses the **signed**
form `-k·(‖d‖ − l₀)·d̂` so compression pushes and extension pulls — physically
correct, and matches Cyclone's own *anchored* spring (which is signed). The
port's tests pin both directions. Document any such deviation at the call site.

---

## Section 2 — Integrators

| # | Name | Update | Notes |
|---|------|--------|-------|
| 2.1 | Explicit (forward) Euler | `x ← x + v·Δt`; `v ← v + a·Δt` (both from old state) | conditionally stable; injects energy on stiff springs — avoid |
| 2.2 | **Semi-implicit (symplectic) Euler** | `v ← v + a·Δt`; `x ← x + v·Δt` (position uses new v) | energy-stable; the game-physics default; **what Cyclone and our settle use** (`Particle::integrate`, with drag as `v *= damping^Δt`) |
| 2.3 | Verlet / velocity-Verlet | `x ← x + v·Δt + ½a·Δt²`; `v ← v + ½(a+a_new)·Δt` | time-reversible, 2nd-order; underlies PBD |
| 2.4 | RK4 | 4 force-evals per step, weighted average | 4th-order, not symplectic, slow energy drift — not for the real-time hot loop |

---

## Section 3 — Rigid-body rotational dynamics

All Cyclone (`src/body.cpp`, `include/cyclone/core.h`), MIT.

| # | Quantity | Equation / code |
|---|----------|-----------------|
| 3.1 | Inertia tensor | symmetric 3×3 `I`; engine stores `I⁻¹` |
| 3.2 | Box/cuboid tensor | `Iₓ = c·m·(hy²+hz²)`, … (Cyclone uses coeff `0.3`; textbook exact is `1/3·m·half²`) |
| 3.3 | Solid sphere | `I = (2/5)·m·r²` per diagonal |
| 3.4 | Solid cylinder | axis `½·m·r²`; transverse `(1/12)·m·(3r²+h²)` |
| 3.5 | Torque from force at point | `τ = (p − x_cm) × F` |
| 3.6 | Angular integration | `α = I⁻¹_world·τ`; `ω ← ω + α·Δt`; `ω *= angularDamping^Δt` |
| 3.7 | Quaternion orientation | `q ← q + ½·(0,ω)·q·Δt` |
| 3.8 | Quaternion renorm | `q ← q/‖q‖` each step |
| 3.9 | World inertia transform | `I⁻¹_world = R·I⁻¹_body·Rᵀ` |

---

## Section 4 — Contact / collision resolution

**Narrow-phase (cite, do not reimplement):** SAT (Box2D `manifold.c`), GJK
(Bullet `btGjkPairDetector`), EPA (Bullet `btGjkEpaPenetrationDepthSolver`).

| # | Primitive | Core update | Reference |
|---|-----------|-------------|-----------|
| 4.1 | Contact generation | `(point, normal, penetration)` | Cyclone `collide_*`, Box2D `manifold.c` |
| 4.2 | Restitution `e` | `Δv = −(1+e)·v_sep`, `e→0` below a closing-speed threshold | Cyclone `Contact::calculateDesiredDeltaVelocity` |
| 4.3 | Coulomb friction | tangential impulse clamped to cone `‖j_t‖ ≤ μ·j_n` | Cyclone `contacts.cpp`, Box2D `contact_solver.c` |
| 4.4 | Sequential impulse (Catto) | per-contact `j_n = −normalMass·(vn+bias)`, accumulate-and-clamp `j_n≥0`, warm-start, **Gauss–Seidel** | Box2D `contact_solver.c` (TGS-soft), Chipmunk `cpArbiter.c` |
| 4.5 | Position-based (XPBD) | `Δλ = (−C − α̃λ)/(∇C·M⁻¹·∇Cᵀ + α̃)`, `Δx = M⁻¹·∇Cᵀ·Δλ`, `α̃ = α/Δt²` (PBD = XPBD, α=0) | XPBD paper |

---

## Section 5 — Common constraints / joints

| # | Joint | Enforces | Cleanest reference |
|---|-------|----------|--------------------|
| 5.1 | Distance | `‖x_a − x_b‖ = d` | Box2D `distance_joint.c`; Cyclone `ParticleRod`/`Cable` (`plinks.cpp`) |
| 5.2 | Contact | non-penetration + friction | §4.4/4.5 |
| 5.3 | Revolute / hinge | shared anchor, free axis | Box2D `revolute_joint.c`; Bullet `btHingeConstraint` |
| 5.4 | Fixed / weld | lock relative pose | Box2D `weld_joint.c`; Bullet `btFixedConstraint` |
| 5.5 | Rod / cable | rod `‖d‖=l`; cable `‖d‖≤l_max` | Cyclone `plinks.cpp` |

---

## Porting map — parallelism profile for the GPU stage

**Embarrassingly parallel (one thread per particle/body):**
- All **Section 1** force generators (two-body springs need an atomic/scatter on
  the second body's force accumulator, or a gather formulation).
- All **Section 2** integrators (RK4 just runs the force kernel 4×).
- All **Section 3** rigid-body terms (force-at-point needs an atomic add to torque).

**Iterative / global solves — the hard half:**
- **§4.4 sequential impulse** and **§4.5/§4.6 position-based** are **Gauss–Seidel**:
  each constraint reads velocities/positions mutated by prior constraints this
  iteration. Naive per-constraint parallelism **races** on shared bodies. The GPU
  port needs **graph/constraint colouring** (independent batches sharing no body)
  or a Jacobi-relaxation variant.
- **Do NOT** port Cyclone's serial *worst-contact-first* resolver to the GPU —
  replace it with coloured-batch **XPBD** (the GPU-natural family; what
  Flex/PhysX use). The engine's existing differential-contact-floor settle is
  already position-based, so this is an extension of the settle, not a new
  paradigm.

**Net:** Sections 1–3 port straight to host C++ now and to per-body kernels later
with minimal friction. Sections 4–5 are the real engineering: pick one solver
family (coloured-batch XPBD) and build the colouring pass as a first-class
component.

---

## Two build caveats (flagged, not yet resolved)

1. **Box-inertia coefficient** — Cyclone uses `0.3`, not the textbook
   `1/3 (≈0.333)`; a deliberate approximation in the source. Decide keep-vs-correct
   when Section 3 is ported.
2. **XPBD equations** — the `Δλ`/`α̃ = α/Δt²` forms above are the standard
   published ones and corroborated across sources, but the original paper PDF was
   not re-read line-by-line during assembly. Verify verbatim against the paper
   before banking the compliance term in §4.5.

## Status

- **This document:** the standard catalogue, source-grounded. Reference only.
- **Ported so far:** Section 1 force generators → `hcp-engine/Gem/Source/Forces/`
  (standalone CPU-reference slice + ctest). Not yet wired into the live settle.
- **Next:** Section 2 integrators already present as the slice's test harness;
  Sections 3–5 are later slices, with §4–5 gated on the constraint-colouring work.
