# Forces slice — the STANDARD Newtonian force-generator library (Section 1)

Standalone, header-only, GPU-free CPU reference for the Section-1 force
generators and the particle integrator. This ports **Section 1** of the
catalogue [`docs/04-engine/newtonian-force-library.md`](../../../../docs/04-engine/newtonian-force-library.md):
the per-particle force generators and the semi-implicit (symplectic) Euler
integrator, expressed as pure C++17 with no AZ / PhysX / O3DE dependencies.

It is the **deterministic reference / oracle** the eventual AZSL/GPU force pass
is validated against, mirroring how `Settle/SettleKernel.h` is the oracle for
`HCPSettleCompute.azsl`.

## Source

Ported from Ian Millington's **Cyclone** physics engine
(`idmillington/cyclone-physics`, master) from the LIVE source, not from memory:

| concept | Cyclone source |
|---------|----------------|
| force generators | `src/pfgen.cpp`, `include/cyclone/pfgen.h` |
| integrator       | `src/particle.cpp` (`Particle::integrate`) |
| vector math      | `include/cyclone/core.h` (`Vector3`) |

Each generator in `ForceKernel.h` cites the Cyclone class it ports in a comment.

## Files

| file | role |
|------|------|
| `ForceKernel.h`   | header-only CPU reference: `Vec3`, `Particle`, the Section-1 generators (Gravity, Drag, Spring, AnchoredSpring, Bungee, Buoyancy, StiffSpring, ConstantForce), and `Integrate` (symplectic Euler) |
| `test_forces.cpp` | analytic, deterministic, GPU-free tests — closed-form expected direction/magnitude/sign per generator, plus integrator closed-form + determinism |

## Generators

- **Gravity** — `F = m*g`; early-outs to zero force on infinite mass.
- **Drag(k1, k2)** — `F = -(k1*|v| + k2*|v|^2) * v_hat`, anti-parallel to velocity.
- **Spring(other, k, l0)** — two-particle Hooke spring (see *Deviations*).
- **AnchoredSpring(anchor, k, l0)** — Hooke spring to a fixed point.
- **Bungee(other, k, l0)** — pulls only when taut (`|d| > l0`), never pushes.
- **Buoyancy(maxDepth, volume, waterHeight, liquidDensity)** — see *Deviations*.
- **StiffSpring(anchor, springConstant, damping)** — Cyclone `ParticleFakeSpring`,
  the analytic damped-harmonic spring stable at high stiffness; mass-dependent,
  early-outs on infinite mass.
- **ConstantForce(force)** — a fixed force vector added each step.

The integrator `Integrate(Particle&, dt)` is semi-implicit (symplectic) Euler,
matching Cyclone `Particle::integrate` and the HCP settle slice:
apply forces to acceleration, `v += a*dt`, `v *= pow(damping, dt)`,
`x += v*dt`, clear the accumulator. Immovable particles (`inverseMass <= 0`,
i.e. infinite mass) are not integrated. `inverseMass == 0` encodes immovable.

## Deliberate deviations from Cyclone (correctness forks)

1. **Two-particle Spring uses the SIGNED Hooke form.**
   Cyclone's `ParticleSpring::updateForce` computes
   `magnitude = real_abs(magnitude - restLength)`, which makes a *compressed*
   spring (`|d| < l0`) also PULL — a well-known quirk. We use the signed form
   `F = -k*(|d| - l0) * d_hat` so compression PUSHES and extension PULLS, which
   is what a real spring does. (Cyclone's `ParticleAnchoredSpring` is **already**
   signed — `(restLength - magnitude) * springConstant`, no abs — so it is
   ported verbatim.) `test_forces.cpp` has a compression check that would fail
   Cyclone's abs version, included deliberately.

2. **Buoyancy uses a corrected, continuous, monotonic partial-submersion ramp.**
   The Cyclone branch structure is ported, but its literal partial-submersion
   formula is sign-inconsistent with its own fully-submerged branch. Cyclone's
   partial line is
   `force.y = liquidDensity*volume * (depth - maxDepth - waterHeight)/(2*maxDepth)`.
   Evaluated at the fully-submerged boundary `depth = waterHeight - maxDepth`
   it yields `-liquidDensity*volume` (a DOWNWARD force), contradicting the
   adjacent fully-submerged branch which gives `+liquidDensity*volume`, and it is
   not monotonic. We implement the physically-correct continuous ramp:
   `submersion = (waterHeight + maxDepth - depth) / (2*maxDepth)` clamped to the
   branch range, `force.y = liquidDensity*volume * submersion`. So: zero above
   the water volume, `+rho*V` fully submerged, and a continuous linear ramp
   between that is monotonic in depth and continuous at both boundaries.

3. **Two-particle Bungee sign-corrected to PULL when taut.**
   Cyclone's `ParticleBungee::updateForce` computes
   `magnitude = springConstant * (restLength - |d|)` (negative when taut) then
   `force *= -magnitude`, which produces a force along `+d_hat` — i.e. pushing
   `p` *away* from `other` when the bungee is stretched. That is the opposite of
   a bungee and is inconsistent with Cyclone's own `ParticleAnchoredBungee`,
   which uses `magnitude = |d| - restLength` (positive) then `*= -magnitude` and
   correctly pulls toward the anchor. We adopt the AnchoredBungee sign for the
   two-particle bungee: `magnitude = springConstant * (|d| - l0)`,
   `force = -magnitude * d_hat`, so a taut bungee pulls `p` toward `other` and
   never pushes. (The slack branch `|d| <= l0 -> 0` is unchanged.)

The integrator also uses the standard symplectic ordering (update velocity, then
position with the *new* velocity), as the slice spec and the settle reference
require; Cyclone literally advances position before velocity within the step.

## Status

- **Done + verified (this slice, in isolation):** all generators + integrator
  implemented as a header-only CPU reference; analytic test green via `ctest`.
- **CPU reference only — NOT yet wired into the live engine.** Nothing in the
  running engine calls these generators yet; there is no AZSL force pass
  validated against this oracle yet. Next = wiring (a GPU/AZSL force pass and an
  equivalence harness against this reference, in the manner of the Settle slice).

## Numerical limitations & input contracts

Documented contracts, several pinned by `test_forces.cpp`'s degenerate-regime
checks:

- **StiffSpring requires an underdamped spring** (`4*springConstant > damping^2`).
  Outside that range the closed-form discriminant goes negative and `gamma` is
  NaN; the generator detects this (`!(gamma > 0)`) and emits **zero force**
  rather than poisoning the particle. `dt == 0` likewise emits zero force (the
  closed form divides by `dt` and `dt*dt`).
- **Drag drops the force at non-finite magnitudes.** An extreme but finite speed
  whose square overflows `float` to `+inf` would otherwise yield a NaN force; the
  generator early-outs instead.
- **Coincident bodies (`d == 0`) produce zero force** for Spring / AnchoredSpring
  (the direction is undefined). A collapsed spring exerts no restoring force at
  the exact singularity — keep bodies from being exactly coincident if it matters.
- **Inputs are trusted, not validated.** Negative `springConstant`, `restLength`,
  or `maxDepth` are accepted and produce internally-consistent but physically odd
  behaviour (e.g. a negative `maxDepth` silently yields zero buoyancy). The caller
  owns sane parameters. `inverseMass <= 0` is treated as infinite mass throughout
  (`getMass()` and `hasFiniteMass()` agree).

## License / attribution

`ForceKernel.h` is a derivative port of **Cyclone Physics**
(`idmillington/cyclone-physics`), MIT-licensed (Copyright (c) 2003-2009 Ian
Millington). The full MIT copyright + permission + warranty notice is retained at
the top of `ForceKernel.h`, as the MIT License requires for copies and
substantial portions. The repository root is AGPL-3.0; combining the MIT-derived
code is permitted, with the upstream notice retained as required. The Box2D /
Bullet / Chipmunk engines named in the catalogue are **referenced only, not
ported here**, so they carry no attribution obligation in this slice.

## Build / run

```sh
cd hcp-engine/Gem/Source/Forces
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make && ctest --output-on-failure
./test_forces        # per-check output
```
