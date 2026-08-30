// SettleKernel.h — portable CPU reference for the HCP particle SETTLE.
//
// This is the AZSL-port slice 1: the settle-step + run-gate + freeze logic,
// expressed as pure C++ (no AZ / PhysX / O3DE deps) so it (a) builds and tests
// standalone, (b) is the DETERMINISTIC ORACLE the AZSL compute shader
// (HCPSettleCompute.azsl) is validated against, and (c) is the portable Stage-1
// fallback for the fabric (see graph claim
// fabric-seam-staging-copy-first-...).
//
// MAPPING (kept line-for-line with HCPSettleCompute.azsl):
//   - Verlet integrate + exp damping        <- AtomTressFX HairSimulationCommon.azsli:292 (corpus 03)
//   - position.w movable flag (bed vs run)  <- IsMovable, TressFX (corpus 03 §2.2)
//   - run gate: settledCount == charCount    <- HCPVocabBed.cpp:431-453 (the readback loop we are porting)
//   - freeze settled run -> w<=0 (joins bed) <- corpus 03 §6 recommendation
//
// CONSTANTS are taken from the live engine so the reference is faithful:
//   gravity -9.81 Y (HCPParticlePipeline.cpp:90, HCPVocabBed.cpp:138)
//   PBD damping 0.5 (HCPVocabBed.cpp:164)   PBD friction 0.2 (idem)
//   DT 1/60, settle budget 60 steps (RC_DT, RC_SETTLE_STEPS — HCPResolutionChamber.h:69-70)
//   gate L1 velocity < 0.5 (WS_VELOCITY_SETTLE_THRESHOLD — HCPVocabBed.h:45)
//
// NB: this reference reproduces the GATE decision (did a run settle within
// budget), NOT bit-exact PhysX trajectories. Per claim 608 finding (2) the
// validation oracle is resolution-equivalence, not float-identity.

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace hcp::settle
{
    // ---- Faithful engine constants (see header note for source file:line) ----
    constexpr float DT                     = 1.0f / 60.0f;
    constexpr float GRAVITY                = 9.81f;   // magnitude, applied -Y
    constexpr float DAMPING                = 0.5f;    // PBD material damping
    constexpr float FRICTION               = 0.2f;    // PBD material friction
    constexpr float VEL_SETTLE_THRESHOLD   = 0.5f;    // L1 velocity (units/sec)
    constexpr int   SETTLE_STEPS           = 60;      // per-cycle step budget

    struct Float4 { float x, y, z, w; };

    // A particle's contact floor for slice 1. Broad-phase (which bed particle a
    // run particle lands on) is DEFERRED — the host supplies restY. Free-fall
    // particles (no contact) use restY = -INF (never clamped).
    constexpr float NO_FLOOR = -1e30f;

    // position.w encodes movability: w > 0 movable (run), w <= 0 immovable (bed).
    inline bool IsMovable(const Float4& p) { return p.w > 0.0f; }

    // L1 (Manhattan) velocity magnitude, matching HCPVocabBed.cpp:436 exactly:
    //   vMag = |vx| + |vy| + |vz|.  velocity = (cur - prev) / DT  (units/sec).
    inline float L1Velocity(const Float4& cur, const Float4& prev)
    {
        const float vx = (cur.x - prev.x) / DT;
        const float vy = (cur.y - prev.y) / DT;
        const float vz = (cur.z - prev.z) / DT;
        return std::fabs(vx) + std::fabs(vy) + std::fabs(vz);
    }

    // One Verlet settle step for a single particle.
    //   cur  : current position (in/out updated to new position)
    //   prev : previous position (in/out updated for history + contact rewrite)
    //   restY: contact floor (NO_FLOOR = none)
    // Bed particles (w<=0) are pinned. Movable particles integrate under gravity
    // with exponential velocity damping; on contact with restY the position is
    // clamped and the velocity history is rewritten (TressFX trick) so contact
    // injects no phantom velocity, with FRICTION bleeding the tangential part.
    inline void SettleStepOne(Float4& cur, Float4& prev, float restY)
    {
        if (!IsMovable(cur))               // bed: immovable anchor, never moves
            return;

        // Per-step velocity decay. NOTE: this is NOT TressFX's
        // exp(-dampingCoeff*dt*60) — that 60x frame-normalization is for a
        // hair-specific [0,1] coefficient and, with PBD's 0.5, over-damps so
        // hard that free-fall terminal velocity drops below the settle gate
        // (caught by test_settle). PhysX PBD `damping` is a per-second velocity
        // damping, so the faithful per-step factor is exp(-damping*dt). Settle
        // energy is bled mainly by CONTACT (prev rewrite below), not by damping.
        const float decay = std::exp(-DAMPING * DT);
        const float ax = 0.0f, ay = -GRAVITY, az = 0.0f;

        Float4 next;
        next.x = cur.x + decay * (cur.x - prev.x) + ax * DT * DT;
        next.y = cur.y + decay * (cur.y - prev.y) + ay * DT * DT;
        next.z = cur.z + decay * (cur.z - prev.z) + az * DT * DT;
        next.w = cur.w;

        Float4 newPrev = cur;              // default Verlet history: prev <- old cur

        if (restY > NO_FLOOR && next.y < restY)
        {
            next.y = restY;
            // Kill the normal (Y) velocity: prev.y = cur.y so (next.y - prev.y) ~ 0.
            newPrev.y = next.y;
            // Friction on the tangential (X,Z) velocity: move prev toward next.
            newPrev.x = next.x - (next.x - cur.x) * (1.0f - FRICTION);
            newPrev.z = next.z - (next.z - cur.z) * (1.0f - FRICTION);
        }

        prev = newPrev;
        cur  = next;
    }

    // A "run" = a contiguous span of particles (one per character of runText).
    struct Run
    {
        uint32_t bufferStart;   // first particle index
        uint32_t charCount;     // number of particles (= chars)
        bool     resolved = false;
    };

    // The settle GATE, mirroring HCPVocabBed.cpp:431-453: a run resolves iff
    // ALL of its charCount particles have L1 velocity below threshold. Returns
    // the settled-particle count for the run (caller compares == charCount).
    inline uint32_t CountSettled(const std::vector<Float4>& cur,
                                 const std::vector<Float4>& prev,
                                 const Run& run)
    {
        uint32_t settled = 0;
        for (uint32_t c = 0; c < run.charCount; ++c)
        {
            const uint32_t idx = run.bufferStart + c;
            if (idx >= cur.size()) break;
            if (L1Velocity(cur[idx], prev[idx]) < VEL_SETTLE_THRESHOLD)
                ++settled;
        }
        return settled;
    }

    inline bool RunGate(const std::vector<Float4>& cur,
                        const std::vector<Float4>& prev,
                        const Run& run)
    {
        return CountSettled(cur, prev, run) == run.charCount;
    }

    // Freeze a settled run: flip each particle's w<=0 so it JOINS the bed
    // (becomes an immovable anchor + drops out of integration). Idempotent.
    inline void FreezeRun(std::vector<Float4>& cur, const Run& run)
    {
        for (uint32_t c = 0; c < run.charCount; ++c)
        {
            const uint32_t idx = run.bufferStart + c;
            if (idx >= cur.size()) break;
            if (cur[idx].w > 0.0f) cur[idx].w = 0.0f;   // run -> bed
        }
    }

    // Advance one settle step over the whole particle array (parallel-safe:
    // each particle is independent — this is the per-thread body the AZSL
    // [numthreads] kernel runs). restY is per-particle (NO_FLOOR for free).
    inline void SettleStepAll(std::vector<Float4>& cur,
                              std::vector<Float4>& prev,
                              const std::vector<float>& restY)
    {
        for (size_t i = 0; i < cur.size(); ++i)
            SettleStepOne(cur[i], prev[i], i < restY.size() ? restY[i] : NO_FLOOR);
    }

} // namespace hcp::settle
