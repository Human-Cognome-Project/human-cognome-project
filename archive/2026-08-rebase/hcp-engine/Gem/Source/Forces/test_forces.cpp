// test_forces.cpp — deterministic, GPU-free tests for the Section-1 Newtonian
// force-generator library (ForceKernel.h).
//
// Follows the project's standalone test idiom (Settle/test_settle.cpp): manual
// Check(label, bool) + printf, process exit code = number of failures. No gtest.
//
// Every check is analytic: a closed-form expected value compared against the
// generator output, testing DIRECTION and MAGNITUDE and SIGN. The two-particle
// Spring compression check is the one that would FAIL Cyclone's real_abs quirk;
// it is included deliberately to pin the signed deviation.

#include "ForceKernel.h"

#include <cmath>
#include <cstdio>

using namespace hcp::forces;

static int g_pass = 0, g_fail = 0;

static void Check(const char* label, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    if (ok) ++g_pass; else ++g_fail;
}

static bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

static bool NearVec(const Vec3& a, const Vec3& b, float eps = 1e-4f)
{
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}

// ---- Gravity --------------------------------------------------------------
static void TestGravity()
{
    const Vec3 g{0.0f, -9.81f, 0.0f};

    // Unit-mass particle: F == m*g == g (m=1).
    Particle p;
    p.inverseMass = 1.0f;
    Gravity(g).updateForce(p, 1.0f / 60.0f);
    Check("gravity: unit mass force == m*g", NearVec(p.forceAccum, g));

    // 3 kg particle: F == 3*g.
    Particle p3;
    p3.inverseMass = 1.0f / 3.0f;
    Gravity(g).updateForce(p3, 1.0f / 60.0f);
    Check("gravity: 3 kg force == 3*g", NearVec(p3.forceAccum, g * 3.0f, 1e-3f));

    // Infinite mass (inverseMass == 0): ZERO force.
    Particle pinf;
    pinf.inverseMass = 0.0f;
    Gravity(g).updateForce(pinf, 1.0f / 60.0f);
    Check("gravity: infinite-mass (inverseMass 0) gets zero force",
          NearVec(pinf.forceAccum, Vec3{0, 0, 0}));
}

// ---- Drag -----------------------------------------------------------------
static void TestDrag()
{
    const float k1 = 0.5f, k2 = 0.1f;

    // At rest: zero force.
    {
        Particle p;
        p.velocity = {0, 0, 0};
        Drag(k1, k2).updateForce(p, 0.016f);
        Check("drag: zero force at rest", NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Moving: anti-parallel to velocity, magnitude k1*|v| + k2*|v|^2.
    {
        Particle p;
        p.velocity = {3.0f, 4.0f, 0.0f};      // |v| == 5
        Drag(k1, k2).updateForce(p, 0.016f);

        const float speed = 5.0f;
        const float expectedMag = k1 * speed + k2 * speed * speed;   // 2.5 + 2.5 = 5
        const Vec3 expected = p.velocity.normalized() * (-expectedMag);

        Check("drag: force anti-parallel to velocity, exact magnitude",
              NearVec(p.forceAccum, expected));
        Check("drag: magnitude == k1*|v| + k2*|v|^2",
              Near(p.forceAccum.magnitude(), expectedMag));
        Check("drag: force opposes motion (dot(force,v) < 0)",
              p.forceAccum.dot(p.velocity) < 0.0f);
    }
}

// ---- Spring (two-particle, SIGNED deviation) ------------------------------
static void TestSpring()
{
    const float k = 2.0f, l0 = 1.0f;

    Particle other;
    other.position = {0.0f, 0.0f, 0.0f};

    // At rest length: zero force.
    {
        Particle p;
        p.position = {l0, 0.0f, 0.0f};         // |d| == l0
        Spring(&other, k, l0).updateForce(p, 0.016f);
        Check("spring: zero force at rest length", NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Stretched (|d| > l0): pulls p toward `other` (i.e. -X direction).
    {
        Particle p;
        p.position = {2.0f, 0.0f, 0.0f};       // |d| == 2, extension == 1
        Spring(&other, k, l0).updateForce(p, 0.016f);
        // |F| = k*(|d|-l0) = 2*1 = 2, directed -X (toward other).
        Check("spring: stretched pulls toward other (exact)",
              NearVec(p.forceAccum, Vec3{-2.0f, 0.0f, 0.0f}));
    }

    // Compressed (|d| < l0): pushes p AWAY from `other` (+X). This is the check
    // that fails Cyclone's real_abs quirk (which would also pull here).
    {
        Particle p;
        p.position = {0.5f, 0.0f, 0.0f};       // |d| == 0.5, compression == 0.5
        Spring(&other, k, l0).updateForce(p, 0.016f);
        // |F| = k*|0.5-1| = 2*0.5 = 1, directed +X (away from other).
        Check("spring: COMPRESSED pushes away from other (signed, not abs)",
              NearVec(p.forceAccum, Vec3{1.0f, 0.0f, 0.0f}));
        Check("spring: compressed force points away (+X), not toward",
              p.forceAccum.x > 0.0f);
    }
}

// ---- AnchoredSpring -------------------------------------------------------
static void TestAnchoredSpring()
{
    const Vec3 anchor{0.0f, 0.0f, 0.0f};
    const float k = 3.0f, l0 = 1.0f;

    // At rest length: zero force.
    {
        Particle p;
        p.position = {0.0f, l0, 0.0f};
        AnchoredSpring(anchor, k, l0).updateForce(p, 0.016f);
        Check("anchored spring: zero force at rest length",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Stretched: pulls toward anchor (-Y).
    {
        Particle p;
        p.position = {0.0f, 2.0f, 0.0f};       // |d|=2, extension=1
        AnchoredSpring(anchor, k, l0).updateForce(p, 0.016f);
        // magnitude = (l0 - |d|)*k = (1-2)*3 = -3; force = magnitude*d_hat = -3*(+Y) = -3 Y
        Check("anchored spring: stretched pulls toward anchor (exact)",
              NearVec(p.forceAccum, Vec3{0.0f, -3.0f, 0.0f}));
    }

    // Compressed: pushes away from anchor (+Y).
    {
        Particle p;
        p.position = {0.0f, 0.5f, 0.0f};       // |d|=0.5, compression=0.5
        AnchoredSpring(anchor, k, l0).updateForce(p, 0.016f);
        // magnitude = (1-0.5)*3 = 1.5; force = 1.5 * (+Y) = +1.5 Y
        Check("anchored spring: compressed pushes away from anchor (exact)",
              NearVec(p.forceAccum, Vec3{0.0f, 1.5f, 0.0f}));
    }
}

// ---- Bungee ---------------------------------------------------------------
static void TestBungee()
{
    const float k = 2.0f, l0 = 1.0f;
    Particle other;
    other.position = {0.0f, 0.0f, 0.0f};

    // Slack (|d| < l0): zero force.
    {
        Particle p;
        p.position = {0.5f, 0.0f, 0.0f};
        Bungee(&other, k, l0).updateForce(p, 0.016f);
        Check("bungee: slack (|d|<l0) -> zero force", NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Exactly at rest length: zero force (|d| <= l0 branch).
    {
        Particle p;
        p.position = {1.0f, 0.0f, 0.0f};
        Bungee(&other, k, l0).updateForce(p, 0.016f);
        Check("bungee: at rest length -> zero force", NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Taut (|d| > l0): pulls toward other.
    {
        Particle p;
        p.position = {2.0f, 0.0f, 0.0f};       // |d|=2, extension=1
        Bungee(&other, k, l0).updateForce(p, 0.016f);
        // magnitude = k*(l0-|d|) = 2*(1-2) = -2; force = -magnitude*d_hat = 2*(-X) = -2 X
        Check("bungee: taut pulls toward other (exact)",
              NearVec(p.forceAccum, Vec3{-2.0f, 0.0f, 0.0f}));
    }
}

// ---- Buoyancy -------------------------------------------------------------
static void TestBuoyancy()
{
    const float maxDepth = 1.0f, volume = 2.0f, waterHeight = 0.0f, rho = 1000.0f;
    const float full = rho * volume;   // fully submerged force magnitude
    Buoyancy b(maxDepth, volume, waterHeight, rho);

    // Above water (depth >= waterHeight + maxDepth): zero.
    {
        Particle p; p.position = {0.0f, 1.0f, 0.0f};   // exactly at the top boundary
        b.updateForce(p, 0.016f);
        Check("buoyancy: at/above top boundary -> zero force",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }
    {
        Particle p; p.position = {0.0f, 5.0f, 0.0f};   // well above water
        b.updateForce(p, 0.016f);
        Check("buoyancy: above water -> zero force",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Fully submerged (depth <= waterHeight - maxDepth): +rho*V upward.
    {
        Particle p; p.position = {0.0f, -1.0f, 0.0f};  // at the fully-submerged boundary
        b.updateForce(p, 0.016f);
        Check("buoyancy: fully submerged -> +rho*V upward",
              NearVec(p.forceAccum, Vec3{0.0f, full, 0.0f}));
    }
    {
        Particle p; p.position = {0.0f, -5.0f, 0.0f};  // deep
        b.updateForce(p, 0.016f);
        Check("buoyancy: deep submerged -> +rho*V upward (clamped)",
              NearVec(p.forceAccum, Vec3{0.0f, full, 0.0f}));
    }

    // Partial submersion (between boundaries): between 0 and rho*V, monotonic.
    // submersion = (waterHeight + maxDepth - depth)/(2*maxDepth)
    //   depth=0.5 -> (0+1-0.5)/2 = 0.25 -> force = 0.25*full
    //   depth=0.0 -> (0+1-0  )/2 = 0.5  -> force = 0.5 *full
    //   depth=-0.5-> (0+1+0.5)/2 = 0.75 -> force = 0.75*full
    {
        Particle pa; pa.position = {0.0f,  0.5f, 0.0f}; b.updateForce(pa, 0.016f);
        Particle pb; pb.position = {0.0f,  0.0f, 0.0f}; b.updateForce(pb, 0.016f);
        Particle pc; pc.position = {0.0f, -0.5f, 0.0f}; b.updateForce(pc, 0.016f);

        const float fa = pa.forceAccum.y, fb = pb.forceAccum.y, fc = pc.forceAccum.y;

        Check("buoyancy: mid value at depth 0.5 == 0.25*rho*V", Near(fa, 0.25f * full, 1e-1f));
        Check("buoyancy: mid value at depth 0.0 == 0.50*rho*V", Near(fb, 0.50f * full, 1e-1f));
        Check("buoyancy: mid value at depth -0.5 == 0.75*rho*V", Near(fc, 0.75f * full, 1e-1f));

        Check("buoyancy: partial values strictly between 0 and rho*V",
              fa > 0.0f && fa < full && fb > 0.0f && fb < full && fc > 0.0f && fc < full);
        Check("buoyancy: monotonically increasing as particle sinks", fa < fb && fb < fc);

        // Continuity at the two boundaries: limit of the ramp matches the
        // adjacent constant branches (0 above, full below).
        Particle pTop; pTop.position = {0.0f, 1.0f - 1e-4f, 0.0f}; b.updateForce(pTop, 0.016f);
        Check("buoyancy: continuous at top boundary (ramp -> 0)",
              Near(pTop.forceAccum.y, 0.0f, 1.0f));
        Particle pBot; pBot.position = {0.0f, -1.0f + 1e-4f, 0.0f}; b.updateForce(pBot, 0.016f);
        Check("buoyancy: continuous at bottom boundary (ramp -> rho*V)",
              Near(pBot.forceAccum.y, full, 1.0f));
    }
}

// ---- StiffSpring ----------------------------------------------------------
static void TestStiffSpring()
{
    const Vec3 anchor{0.0f, 0.0f, 0.0f};
    StiffSpring s(anchor, /*springConstant*/ 100.0f, /*damping*/ 1.0f);

    // Displaced from anchor: the resulting force pulls back toward the anchor,
    // is finite, and does not NaN over a few integration steps.
    Particle p;
    p.position = {1.0f, 0.0f, 0.0f};
    p.velocity = {0.0f, 0.0f, 0.0f};
    p.inverseMass = 1.0f;
    p.damping = 0.99f;

    const float dt = 1.0f / 60.0f;

    s.updateForce(p, dt);
    Check("stiff spring: initial force pulls back toward anchor (-X)", p.forceAccum.x < 0.0f);

    bool finite = true;
    for (int i = 0; i < 8; ++i)
    {
        p.clearAccumulator();
        s.updateForce(p, dt);
        Integrate(p, dt);
        finite = finite
              && std::isfinite(p.position.x) && std::isfinite(p.position.y) && std::isfinite(p.position.z)
              && std::isfinite(p.velocity.x) && std::isfinite(p.velocity.y) && std::isfinite(p.velocity.z);
    }
    Check("stiff spring: stays finite (no NaN) over 8 steps", finite);
    Check("stiff spring: immovable particle gets no force", [] {
        Particle pinf; pinf.position = {1, 0, 0}; pinf.inverseMass = 0.0f;
        StiffSpring(Vec3{0,0,0}, 100.0f, 1.0f).updateForce(pinf, 1.0f / 60.0f);
        return NearVec(pinf.forceAccum, Vec3{0, 0, 0});
    }());
}

// ---- Degenerate / numerical-edge regimes (pin the adversarial-review fixes) -
static void TestDegenerate()
{
    const float dt = 1.0f / 60.0f;

    // StiffSpring over/critically damped (4k <= damping^2): the discriminant
    // goes negative, gamma is NaN. The guard must DROP the force, not let NaN
    // poison the particle. (4*1 = 4 <= 16 = 4^2.)
    {
        Particle p; p.position = {1.0f, 0.0f, 0.0f}; p.inverseMass = 1.0f;
        StiffSpring(Vec3{0, 0, 0}, /*k*/ 1.0f, /*damping*/ 4.0f).updateForce(p, dt);
        Check("stiff spring: over-damped (4k<=c^2) -> zero force, not NaN",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // StiffSpring at dt == 0: the closed form divides by dt and dt*dt; the
    // guard must prevent the 0*inf -> NaN.
    {
        Particle p; p.position = {1.0f, 0.0f, 0.0f}; p.inverseMass = 1.0f;
        StiffSpring(Vec3{0, 0, 0}, 100.0f, 1.0f).updateForce(p, 0.0f);
        Check("stiff spring: dt==0 -> zero force, not NaN",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }

    // Drag at an extreme (finite) speed whose square overflows float to +inf:
    // the non-finite guard drops the force rather than emitting NaN.
    {
        Particle p; p.velocity = {1e20f, 0.0f, 0.0f};
        Drag(0.5f, 0.1f).updateForce(p, dt);
        Check("drag: extreme speed (|v|^2 overflow) -> finite force, not NaN",
              std::isfinite(p.forceAccum.x) && std::isfinite(p.forceAccum.y) &&
              std::isfinite(p.forceAccum.z));
    }

    // Coincident spring (d == 0): direction is undefined -> zero force. This is
    // a documented limitation; the check pins it as intentional, not accidental.
    {
        Particle other; other.position = {0.0f, 0.0f, 0.0f};
        Particle p;     p.position     = {0.0f, 0.0f, 0.0f};
        Spring(&other, 2.0f, 1.0f).updateForce(p, dt);
        Check("spring: coincident positions (d=0) -> zero force (documented limit)",
              NearVec(p.forceAccum, Vec3{0, 0, 0}));
    }
}

// ---- ConstantForce --------------------------------------------------------
static void TestConstantForce()
{
    const Vec3 f{1.5f, -2.0f, 3.25f};
    Particle p;
    ConstantForce(f).updateForce(p, 0.016f);
    Check("constant force: exact constant added", NearVec(p.forceAccum, f));

    // Added every step it is called (accumulates).
    ConstantForce(f).updateForce(p, 0.016f);
    Check("constant force: accumulates across calls", NearVec(p.forceAccum, f * 2.0f));
}

// ---- Integrator -----------------------------------------------------------
static void TestIntegrator()
{
    // Unit mass under constant gravity, damping == 1 (no velocity loss) so the
    // semi-implicit Euler closed form is exact:
    //   v_n = v_0 + a*dt*n
    //   x_n = x_0 + dt*sum_{k=1..n} v_k = x_0 + dt*( n*v_0 + a*dt*n*(n+1)/2 )
    const float dt = 1.0f / 60.0f;
    const Vec3  g{0.0f, -9.81f, 0.0f};
    const int   N = 100;

    Particle p;
    p.position    = {0.0f, 0.0f, 0.0f};
    p.velocity    = {0.0f, 0.0f, 0.0f};
    p.inverseMass = 1.0f;
    p.damping     = 1.0f;                 // no damping -> closed form is exact

    for (int n = 0; n < N; ++n)
    {
        Gravity(g).updateForce(p, dt);
        Integrate(p, dt);
    }

    // Closed form (v0 = 0):
    const float a  = g.y;                                   // -9.81
    const float vN = a * dt * N;                            // v after N steps
    const float xN = dt * (a * dt * (float)N * (N + 1) / 2.0f);  // x after N steps

    Check("integrator: velocity matches symplectic-Euler closed form",
          Near(p.velocity.y, vN, 1e-2f));
    Check("integrator: position matches symplectic-Euler closed form",
          Near(p.position.y, xN, 1e-1f));
    std::printf("       (after %d steps: y=%.4f expected=%.4f, vy=%.4f expected=%.4f)\n",
                N, p.position.y, xN, p.velocity.y, vN);

    // Immovable particle is not integrated.
    {
        Particle pinf;
        pinf.position    = {1.0f, 2.0f, 3.0f};
        pinf.velocity    = {9.0f, 9.0f, 9.0f};
        pinf.inverseMass = 0.0f;
        const Vec3 pos0 = pinf.position;
        Integrate(pinf, dt);
        Check("integrator: immovable (inverseMass 0) particle does not move",
              NearVec(pinf.position, pos0));
    }

    // Determinism: two identical runs are bit-identical.
    {
        auto run = [&](Particle& q) {
            q.position = {0, 0, 0}; q.velocity = {0, 0, 0};
            q.inverseMass = 1.0f; q.damping = 0.95f; q.acceleration = {0, 0, 0};
            for (int n = 0; n < N; ++n) { Gravity(g).updateForce(q, dt); Integrate(q, dt); }
        };
        Particle a1, a2;
        run(a1); run(a2);
        const bool identical =
            a1.position.x == a2.position.x && a1.position.y == a2.position.y &&
            a1.position.z == a2.position.z && a1.velocity.x == a2.velocity.x &&
            a1.velocity.y == a2.velocity.y && a1.velocity.z == a2.velocity.z;
        Check("integrator: two identical runs are bit-identical (deterministic)", identical);
    }
}

int main()
{
    std::printf("=== HCP Section-1 Newtonian force-generator tests ===\n");
    TestGravity();
    TestDrag();
    TestSpring();
    TestAnchoredSpring();
    TestBungee();
    TestBuoyancy();
    TestStiffSpring();
    TestDegenerate();
    TestConstantForce();
    TestIntegrator();
    std::printf("=== %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail;
}
