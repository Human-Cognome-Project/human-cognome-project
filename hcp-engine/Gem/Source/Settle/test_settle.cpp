// test_settle.cpp — deterministic, GPU-free tests for the HCP settle reference.
//
// Follows the project's standalone test idiom (tools/byte-floor/test_bytefloor.cpp):
// manual checks + printf, process exit code = number of failures.
//
// These pin the slice-1 port behaviour that HCPSettleCompute.azsl must match:
//   1. Bed (w<=0) particles are immovable.
//   2. A dropped run particle settles within the step budget and freezes (w<=0).
//   3. The settle is deterministic (bit-identical across repeated runs).
//   4. The run GATE mirrors HCPVocabBed.cpp: resolves iff ALL chars settled.
//   5. Freeze is idempotent and a frozen run thereafter behaves as bed.

#include "SettleKernel.h"

#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

using namespace hcp::settle;

static int g_pass = 0, g_fail = 0;

static void Check(const char* label, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    if (ok) ++g_pass; else ++g_fail;
}

static bool BitEqual(const Float4& a, const Float4& b)
{
    return std::memcmp(&a, &b, sizeof(Float4)) == 0;
}

// Run a particle array forward `steps` settle steps.
static void Advance(std::vector<Float4>& cur, std::vector<Float4>& prev,
                    const std::vector<float>& restY, int steps)
{
    for (int s = 0; s < steps; ++s)
        SettleStepAll(cur, prev, restY);
}

// ---- 1. Bed immobility ----------------------------------------------------
static void TestBedImmovable()
{
    std::vector<Float4> cur  = {{3.0f, 5.0f, 7.0f, 0.0f}};   // w=0 -> bed
    std::vector<Float4> prev = cur;
    std::vector<float>  restY = {NO_FLOOR};
    const Float4 original = cur[0];

    Advance(cur, prev, restY, SETTLE_STEPS);
    Check("bed particle (w<=0) never moves over full budget", BitEqual(cur[0], original));
}

// ---- 2. A dropped run particle settles and freezes ------------------------
static void TestRunSettlesAndFreezes()
{
    // Movable particle (w=1) released just above a contact floor (the bed it
    // lands on), like a stream run loaded just over its vocab particles. The
    // gate is evaluated at END of the step budget, mirroring the engine which
    // calls CheckSettlement after simulating (HCPVocabBed.cpp), not mid-flight.
    std::vector<Float4> cur  = {{0.0f, 0.5f, 0.0f, 1.0f}};
    std::vector<Float4> prev = cur;
    std::vector<float>  restY = {0.0f};

    Run run{0, 1};
    Advance(cur, prev, restY, SETTLE_STEPS);

    Check("dropped run particle is settled at end of step budget", RunGate(cur, prev, run));
    std::printf("       (final y=%.4f, L1 vel=%.4f, threshold=%.2f)\n",
                cur[0].y, L1Velocity(cur[0], prev[0]), VEL_SETTLE_THRESHOLD);

    // Freeze it: should flip to bed (w<=0) and then never move again.
    FreezeRun(cur, run);
    Check("freeze flips settled run to bed (w<=0)", cur[0].w <= 0.0f);

    Float4 afterFreeze = cur[0];
    Advance(cur, prev, restY, SETTLE_STEPS);
    Check("frozen run thereafter behaves as bed (immovable)", BitEqual(cur[0], afterFreeze));
}

// ---- 3. Determinism (the property the oracle depends on) ------------------
static void TestDeterminism()
{
    auto build = [](std::vector<Float4>& cur, std::vector<Float4>& prev, std::vector<float>& restY) {
        cur.clear(); prev.clear(); restY.clear();
        for (int i = 0; i < 16; ++i)
        {
            // mix of bed (even, w=0) and run (odd, w=1) particles at varied heights
            float w = (i % 2 == 0) ? 0.0f : 1.0f;
            cur.push_back({ (float)i, 8.0f + 0.5f * i, (float)(i % 3), w });
            restY.push_back(w > 0.0f ? 0.0f : NO_FLOOR);
        }
        prev = cur;
    };

    std::vector<Float4> curA, prevA; std::vector<float> restA; build(curA, prevA, restA);
    std::vector<Float4> curB, prevB; std::vector<float> restB; build(curB, prevB, restB);

    Advance(curA, prevA, restA, SETTLE_STEPS);
    Advance(curB, prevB, restB, SETTLE_STEPS);

    bool identical = curA.size() == curB.size();
    for (size_t i = 0; identical && i < curA.size(); ++i)
        identical = BitEqual(curA[i], curB[i]) && BitEqual(prevA[i], prevB[i]);

    Check("two runs of identical input are bit-identical (deterministic)", identical);
}

// ---- 4. Run gate mirrors VocabBed: ALL chars must settle ------------------
// This tests the GATE LOGIC in isolation by constructing per-particle
// velocities directly (cur - prev = vel*DT), independent of settle dynamics —
// dynamics calibration vs the live engine is a separate slice.
static void TestRunGateAllOrNothing()
{
    // velocity v (units/sec) encoded as a -Y displacement over one step.
    auto withVel = [](float x, float vy) -> std::pair<Float4, Float4> {
        Float4 c{ x, 0.0f, 0.0f, 1.0f };
        Float4 p{ x, vy * DT, 0.0f, 1.0f };   // cur - prev = -vy*DT  -> L1 vel = |vy|
        return { c, p };
    };

    // 3-char run: two below the gate (0.1/sec), one above it (5.0/sec).
    auto a = withVel(0.0f, 0.1f);
    auto b = withVel(1.0f, 0.1f);
    auto cc = withVel(2.0f, 5.0f);
    std::vector<Float4> cur  = { a.first,  b.first,  cc.first  };
    std::vector<Float4> prev = { a.second, b.second, cc.second };
    Run run{0, 3};

    uint32_t settled = CountSettled(cur, prev, run);
    Check("partial settle does NOT gate the run (mirror VocabBed all-or-nothing)",
          settled == 2 && !RunGate(cur, prev, run));
    std::printf("       (%u of %u chars below gate)\n", settled, run.charCount);

    // Drop the fast particle below the gate -> all three settled -> run gates.
    auto cc2 = withVel(2.0f, 0.1f);
    cur[2] = cc2.first; prev[2] = cc2.second;
    Check("all chars below gate -> run gates",
          CountSettled(cur, prev, run) == 3 && RunGate(cur, prev, run));
}

// ---- 5. Freeze idempotence ------------------------------------------------
static void TestFreezeIdempotent()
{
    std::vector<Float4> cur = {{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
    Run run{0, 2};
    FreezeRun(cur, run);
    Float4 a0 = cur[0], a1 = cur[1];
    FreezeRun(cur, run);   // second freeze must be a no-op
    Check("freeze is idempotent", BitEqual(cur[0], a0) && BitEqual(cur[1], a1));
}

int main()
{
    std::printf("=== HCP settle reference tests ===\n");
    TestBedImmovable();
    TestRunSettlesAndFreezes();
    TestDeterminism();
    TestRunGateAllOrNothing();
    TestFreezeIdempotent();
    std::printf("=== %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail;
}
