// ----------------------------------------------------------------------------
// Portions of this file are derived from Cyclone Physics
// (idmillington/cyclone-physics) — the Section-1 force generators and the
// particle integrator are ports of its updateForce/integrate bodies.
//
//   Copyright (c) 2003-2009 Ian Millington.  Licensed under the MIT License:
//
//   Permission is hereby granted, free of charge, to any person obtaining a
//   copy of this software and associated documentation files (the "Software"),
//   to deal in the Software without restriction, including without limitation
//   the rights to use, copy, modify, merge, publish, distribute, sublicense,
//   and/or sell copies of the Software, and to permit persons to whom the
//   Software is furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//   DEALINGS IN THE SOFTWARE.
// ----------------------------------------------------------------------------
//
// ForceKernel.h — portable CPU reference for the STANDARD Newtonian
// force-generator library (Section 1) + a particle integrator.
//
// This is a standalone AZSL-port slice: the Section-1 force generators and the
// semi-implicit (symplectic) Euler particle integrator from Ian Millington's
// Cyclone physics engine, expressed as pure C++17 (no AZ / PhysX / O3DE deps)
// so it (a) builds and tests standalone and (b) is the DETERMINISTIC REFERENCE
// the eventual AZSL/GPU force pass is validated against.
//
// It ports Section 1 of the catalogue docs/04-engine/newtonian-force-library.md.
//
// SOURCE: idmillington/cyclone-physics (master)
//   - generators : src/pfgen.cpp,    include/cyclone/pfgen.h
//   - integrator : src/particle.cpp
//   - vector math: include/cyclone/core.h
// Each generator below cites the Cyclone class it ports in a comment.
//
// DELIBERATE DEVIATIONS FROM CYCLONE (see comments + README "Status"):
//   1. Two-particle Spring uses the SIGNED form  F = -k*(|d|-l0)*d_hat  so that
//      compression PUSHES and extension PULLS. Cyclone's ParticleSpring uses
//      real_abs(magnitude - restLength), which makes compression also pull
//      (a known quirk). The anchored spring is already signed in Cyclone.
//   2. Buoyancy uses a corrected, continuous, monotonic partial-submersion ramp.
//      Cyclone's literal partial formula is sign-inconsistent with its own
//      fully-submerged branch (it yields -rho*V at the fully-submerged boundary
//      instead of +rho*V); see the Buoyancy comment for the derivation.

#pragma once

#include <cmath>

namespace hcp::forces
{
    // ---- Minimal Vec3 (mirrors cyclone::Vector3 in include/cyclone/core.h) ----
    struct Vec3
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;

        Vec3() = default;
        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
        Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
        Vec3 operator*(float s)       const { return {x * s, y * s, z * s}; }

        Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vec3& operator*=(float s)       { x *= s; y *= s; z *= s; return *this; }

        // dot / scalar product (cyclone operator*)
        float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

        // cross / vector product (cyclone operator%)
        Vec3 cross(const Vec3& v) const
        {
            return { y * v.z - z * v.y,
                     z * v.x - x * v.z,
                     x * v.y - y * v.x };
        }

        float magnitudeSquared() const { return x * x + y * y + z * z; }   // cyclone squareMagnitude
        float magnitude()        const { return std::sqrt(magnitudeSquared()); }

        // cyclone normalise(): unit vector if non-degenerate, else unchanged/zero.
        Vec3 normalized() const
        {
            const float m = magnitude();
            if (m > 0.0f) return {x / m, y / m, z / m};
            return {0.0f, 0.0f, 0.0f};
        }
    };

    // ---- Particle (mirrors cyclone::Particle; see include/cyclone/particle.h) -
    // inverseMass == 0 encodes IMMOVABLE / infinite mass. Generators that depend
    // on mass (Gravity, StiffSpring) early-out on !hasFiniteMass(), exactly as
    // Cyclone's updateForce bodies do.
    struct Particle
    {
        Vec3  position;
        Vec3  velocity;
        Vec3  acceleration;     // base/constant acceleration (e.g. baked gravity); 0 by default
        Vec3  forceAccum;       // accumulated force this step, cleared after integrate
        float damping     = 0.99f;   // per-second velocity retention (cyclone `damping`)
        float inverseMass = 1.0f;    // 0 => infinite mass / immovable

        // cyclone Particle::addForce
        void addForce(const Vec3& f) { forceAccum += f; }

        // Per the slice spec: inverseMass == 0 means infinite mass / immovable,
        // so a finite mass requires inverseMass > 0 (gravity & friends early-out).
        bool hasFiniteMass() const { return inverseMass > 0.0f; }

        // cyclone Particle::getMass (1/inverseMass; infinite => a huge sentinel).
        // Treats inverseMass <= 0 as infinite mass, consistent with
        // hasFiniteMass() — so a stray negative inverseMass never yields a
        // negative mass to a generator that forgets to gate on hasFiniteMass().
        float getMass() const
        {
            if (inverseMass <= 0.0f) return 3.4e38f;   // ~REAL_MAX
            return 1.0f / inverseMass;
        }

        // cyclone Particle::clearAccumulator
        void clearAccumulator() { forceAccum = {0.0f, 0.0f, 0.0f}; }
    };

    // ======================================================================
    // Section 1 force generators. Each models a Cyclone ParticleForceGenerator
    // as a small struct with `void updateForce(Particle&, float dt) const`.
    // ======================================================================

    // Gravity — ports cyclone ParticleGravity::updateForce (src/pfgen.cpp).
    //   if (!particle->hasFiniteMass()) return;
    //   particle->addForce(gravity * particle->getMass());
    struct Gravity
    {
        Vec3 gravity;   // acceleration vector (e.g. {0,-9.81,0})
        explicit Gravity(const Vec3& g) : gravity(g) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            if (!p.hasFiniteMass()) return;          // immovable: no force
            p.addForce(gravity * p.getMass());       // F = m * g
        }
    };

    // Drag — ports cyclone ParticleDrag::updateForce (src/pfgen.cpp).
    //   dragCoeff = k1*|v| + k2*|v|^2;  force = -dragCoeff * v_hat
    struct Drag
    {
        float k1;   // linear drag coefficient
        float k2;   // quadratic drag coefficient
        Drag(float k1_, float k2_) : k1(k1_), k2(k2_) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            Vec3 force = p.velocity;
            float dragCoeff = force.magnitude();
            dragCoeff = k1 * dragCoeff + k2 * dragCoeff * dragCoeff;
            // |v|^2 in the quadratic term overflows float to +inf for extreme
            // (but finite) speeds; -inf * unit-vector would yield NaN and poison
            // the particle. Drop the force rather than emit NaN.
            if (!std::isfinite(dragCoeff)) return;
            force = force.normalized();
            force *= -dragCoeff;                      // anti-parallel to velocity
            p.addForce(force);
        }
    };

    // Spring (two-particle) — ports cyclone ParticleSpring::updateForce, BUT with
    // the SIGNED Hooke form rather than Cyclone's real_abs(magnitude-restLength).
    //   Cyclone : magnitude = real_abs(|d|-l0); force = -k*magnitude*d_hat
    //   here    : force = -k*(|d|-l0) * d_hat
    // With d = this->position - other->position and d_hat = d/|d|:
    //   extended (|d|>l0): (|d|-l0)>0 => force points -d_hat (toward `other`) PULL
    //   compressed (|d|<l0): (|d|-l0)<0 => force points +d_hat (away)        PUSH
    // Cyclone's abs() makes the compressed case ALSO pull (the quirk we drop).
    // (Cyclone's AnchoredSpring is already signed; see AnchoredSpring below.)
    struct Spring
    {
        const Particle* other;
        float springConstant;
        float restLength;
        Spring(const Particle* other_, float k, float l0)
            : other(other_), springConstant(k), restLength(l0) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            Vec3 force = p.position - other->position;   // d
            float length = force.magnitude();
            // SIGNED deviation from Cyclone (no real_abs):
            float magnitude = (length - restLength) * springConstant;
            force = force.normalized();
            force *= -magnitude;
            p.addForce(force);
        }
    };

    // AnchoredSpring — ports cyclone ParticleAnchoredSpring::updateForce.
    //   magnitude = (restLength - |d|) * springConstant;  force = magnitude * d_hat
    // This is ALREADY the signed form in Cyclone (restLength - |d|, no abs), so
    // it is ported verbatim: extended pulls toward anchor, compressed pushes away.
    struct AnchoredSpring
    {
        Vec3  anchor;
        float springConstant;
        float restLength;
        AnchoredSpring(const Vec3& anchor_, float k, float l0)
            : anchor(anchor_), springConstant(k), restLength(l0) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            Vec3 force = p.position - anchor;             // d
            float length = force.magnitude();
            float magnitude = (restLength - length) * springConstant;
            force = force.normalized();
            force *= magnitude;
            p.addForce(force);
        }
    };

    // Bungee (two-particle) — ports cyclone ParticleBungee::updateForce branch
    // structure, with a SIGN CORRECTION so a taut bungee PULLS toward `other`.
    //
    // Cyclone (src/pfgen.cpp) ParticleBungee:
    //   if (magnitude <= restLength) return;                  // slack: no force
    //   magnitude = springConstant * (restLength - magnitude);  // NEGATIVE when taut
    //   force.normalise();  force *= -magnitude;                // => +d_hat => AWAY
    // With d = p.position - other.position and d_hat = d/|d|, when taut
    // (|d|>l0) Cyclone yields a force along +d_hat — i.e. AWAY from `other`,
    // which pushes the bungee further apart. That is the opposite of a bungee
    // (and is inconsistent with Cyclone's own ParticleAnchoredBungee, which uses
    // `magnitude = magnitude - restLength` (positive) then `*= -magnitude`,
    // correctly pulling toward the anchor). We adopt the AnchoredBungee sign:
    //   magnitude = springConstant * (|d| - l0);  force = -magnitude * d_hat
    // so a taut bungee pulls p toward `other`, and it never pushes. (See README.)
    struct Bungee
    {
        const Particle* other;
        float springConstant;
        float restLength;
        Bungee(const Particle* other_, float k, float l0)
            : other(other_), springConstant(k), restLength(l0) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            Vec3 force = p.position - other->position;   // d
            float length = force.magnitude();
            if (length <= restLength) return;            // slack -> no force
            // SIGN-corrected (matches Cyclone's AnchoredBungee, not its Bungee):
            float magnitude = springConstant * (length - restLength);  // positive when taut
            force = force.normalized();
            force *= -magnitude;                          // toward other (pull)
            p.addForce(force);
        }
    };

    // Buoyancy — ports cyclone ParticleBuoyancy::updateForce branch STRUCTURE,
    // with a CORRECTED partial-submersion ramp.
    //
    // Cyclone (src/pfgen.cpp):
    //   depth = position.y;
    //   if (depth >= waterHeight + maxDepth) return;                 // above water: 0
    //   if (depth <= waterHeight - maxDepth) { force.y = rho*V; ... } // fully under: +rho*V
    //   force.y = rho*V * (depth - maxDepth - waterHeight)/(2*maxDepth);   // partial
    //
    // The literal partial formula is sign-inconsistent with the fully-submerged
    // branch. Evaluating it at the fully-submerged boundary depth=waterHeight-maxDepth:
    //   (depth - maxDepth - waterHeight)/(2*maxDepth)
    //     = ((wH - maxDepth) - maxDepth - wH)/(2*maxDepth)
    //     = (-2*maxDepth)/(2*maxDepth) = -1   => force.y = -rho*V  (DOWNWARD!)
    // which contradicts the +rho*V of the adjacent fully-submerged branch and is
    // not monotonic. We implement the physically-correct continuous ramp:
    //   submersion = (waterHeight + maxDepth - depth) / (2*maxDepth) in [0,1]
    //   force.y    = rho*V * submersion
    // so: above water -> 0; fully submerged -> +rho*V; linear, continuous,
    // monotonically increasing as the particle sinks. (See README "Status".)
    struct Buoyancy
    {
        float maxDepth;        // submersion depth at which full buoyancy is reached
        float volume;          // volume of the object
        float waterHeight;     // y of the water surface
        float liquidDensity;   // density of the liquid (default water ~ 1000)
        Buoyancy(float maxDepth_, float volume_, float waterHeight_,
                 float liquidDensity_ = 1000.0f)
            : maxDepth(maxDepth_), volume(volume_),
              waterHeight(waterHeight_), liquidDensity(liquidDensity_) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            const float depth = p.position.y;

            // Out of the water above the surface (+ maxDepth slack): no force.
            if (depth >= waterHeight + maxDepth) return;

            Vec3 force{0.0f, 0.0f, 0.0f};

            // Fully submerged: maximum buoyancy, upward.
            if (depth <= waterHeight - maxDepth)
            {
                force.y = liquidDensity * volume;
                p.addForce(force);
                return;
            }

            // Partially submerged: CORRECTED continuous, monotonic ramp in [0,1].
            const float submersion =
                (waterHeight + maxDepth - depth) / (2.0f * maxDepth);
            force.y = liquidDensity * volume * submersion;
            p.addForce(force);
        }
    };

    // StiffSpring — ports cyclone ParticleFakeSpring::updateForce. An analytic
    // (closed-form damped-harmonic) spring to a fixed anchor that stays stable
    // for very stiff constants where an explicit spring would explode. Depends
    // on mass, so it early-outs on !hasFiniteMass() exactly as Cyclone does.
    struct StiffSpring
    {
        Vec3  anchor;
        float springConstant;
        float damping;
        StiffSpring(const Vec3& anchor_, float springConstant_, float damping_)
            : anchor(anchor_), springConstant(springConstant_), damping(damping_) {}

        void updateForce(Particle& p, float dt) const
        {
            if (!p.hasFiniteMass()) return;
            if (dt == 0.0f) return;                    // closed form divides by dt and dt*dt

            Vec3 position = p.position - anchor;

            // gamma is NaN when 4k <= damping^2 (critically/over-damped: the
            // discriminant goes negative). The (!(gamma > 0)) form rejects NaN,
            // zero, AND negative — a bare `gamma == 0` check would let NaN
            // through (NaN == 0 is false) and poison the particle permanently.
            const float gamma = 0.5f * std::sqrt(4.0f * springConstant - damping * damping);
            if (!(gamma > 0.0f)) return;               // over/critically damped, or NaN: no force

            Vec3 c = position * (damping / (2.0f * gamma)) + p.velocity * (1.0f / gamma);

            Vec3 target = position * std::cos(gamma * dt) + c * std::sin(gamma * dt);
            target *= std::exp(-0.5f * dt * damping);

            Vec3 accel = (target - position) * (1.0f / (dt * dt))
                       - p.velocity * (1.0f / dt);
            p.addForce(accel * p.getMass());
        }
    };

    // ConstantForce — a fixed force vector added every step. (Not a distinct
    // Cyclone class; the trivial generator implied by addForce/the catalogue.)
    struct ConstantForce
    {
        Vec3 force;
        explicit ConstantForce(const Vec3& f) : force(f) {}

        void updateForce(Particle& p, float /*dt*/) const
        {
            p.addForce(force);
        }
    };

    // ======================================================================
    // Integrator — semi-implicit (symplectic) Euler, matching cyclone
    // Particle::integrate (src/particle.cpp) and the HCP settle slice:
    //   resultingAcc = acceleration + forceAccum * inverseMass
    //   velocity    += resultingAcc * dt
    //   velocity    *= pow(damping, dt)
    //   position    += velocity * dt
    //   clearAccumulator()
    // Immovable particles (inverseMass <= 0) are not integrated.
    //
    // NB: Cyclone literally advances position BEFORE updating velocity
    //   (position += velocity*dt; then velocity += acc*dt). We use the standard
    //   symplectic order (velocity first, then position with the NEW velocity),
    //   which is what the slice spec and the HCP settle reference require:
    //   "apply forces to acceleration, v += a*dt, v *= pow(damping,dt), x += v*dt".
    // ======================================================================
    inline void Integrate(Particle& p, float dt)
    {
        if (p.inverseMass <= 0.0f) return;   // immovable: skip

        // Acceleration from the base acceleration + accumulated force.
        Vec3 resultingAcc = p.acceleration;
        resultingAcc += p.forceAccum * p.inverseMass;

        // Update velocity, then apply exponential per-second damping.
        p.velocity += resultingAcc * dt;
        p.velocity *= std::pow(p.damping, dt);

        // Update position with the NEW velocity (symplectic/semi-implicit Euler).
        p.position += p.velocity * dt;

        p.clearAccumulator();
    }

} // namespace hcp::forces
