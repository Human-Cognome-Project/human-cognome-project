"""Field-engine TIME-STEP half (Planner's lane) — seam contract v0, Phase B.

Implements the seam's Phase B exactly (field-engine-interface-v0.md):
  det/code readout from amp, compliance = f(det), tau accrual on front arrival,
  annealing with T_floor > 0, complexity observables.

Owned knobs (seam §Owned config): f(det) form, front_threshold, anneal schedule
+ T_floor, complexity metric, tau accrual.

Answers seam Open #1: f(det) = THRESHOLD-KNEE (logistic drop at d0). Open #2:
front arrival on Δdet — kept. Open #3: H_max = ln(16) — confirmed here.

The stub kernel below is a TEST HARNESS ONLY (not Silas's diffuse()): the
minimum Phase-A stand-in that respects c_local so Phase B can be proven
end-to-end. ⚠ It uses the intent-corrected coupling g = 1 − κ·(1 − compliance)
(fronts stall in STIFF/resolved matter). The seam's #5 literal formula
g = 1 − κ·compliance stalls fronts in CHAOS instead — flagged to Silas.

Run: .venv-snode python timestep.py  → self-checks, hard asserts.
"""

import numpy as np
import taichi as ti

NCODES = 16
EPS_RADIATE = 0.3
H_MAX = float(np.log(NCODES))          # seam Open #3: both halves use ln(16)

# ---------------- Phase B: readout + compliance + tau (numpy reference) ----

def readout_det(amp):
    """det = 1 - H(amp)/H_max per particle. amp: (N,16) sum-normalized."""
    p = np.clip(amp, 1e-12, 1.0)
    h = -(p * np.log(p)).sum(axis=1)
    return np.clip(1.0 - h / H_MAX, 0.0, 1.0)

def f_knee(det, d0=0.85, k=24.0):
    """Compliance ~1 below the det knee d0, drops toward 0 above it.
    d0 sits NEAR 1 by design: rate is receiver-gated (c = c0*g(compliance[p])),
    so the knee is where matter STOPS resolving - a low knee freezes objects
    half-formed (measured: d0=0.6 stalls det at ~0.7, mushy trailing edge)."""
    return 1.0 / (1.0 + np.exp(k * (det - d0)))

def f_linear(det):
    return 1.0 - det

def anneal(T, T_floor=0.05, lam=0.995):
    """Asymptotic: approaches T_floor, never reaches/undershoots it."""
    return T_floor + (T - T_floor) * lam

def phase_b(amp, det_prev, tau, f, front_threshold=0.05):
    """One Phase-B pass. Returns det, code, compliance; mutates tau."""
    det = readout_det(amp)
    code = amp.argmax(axis=1)
    compliance = f(det)
    crossed = (det - det_prev) > front_threshold   # seam Open #2: Δdet arrival
    tau[crossed] += 1                              # throat-crossing count
    return det, code, compliance, crossed

def observables(det, code, d0=0.8, tau=None):
    """v0 complexity observables (provisional — measured, not named; seam #5(e)).
    Boundary metrics (Kernel-#7 era): a boundary cell = resolved cell adjacent
    to a resolved cell of ANOTHER code (contact interface). A chaos-gap wall
    between domains reads as n_boundary=0 with n_domains>1 — the pair
    distinguishes contact-interface from gap-wall. boundary_tau_mean is the
    INTERFACE CLOCK: churning border ticks it (Silas B1: 157), settled border
    freezes it (1) — the run-both discriminator for hard-vs-impossible
    cross-conversion, and it moves before territory does."""
    resolved = det > d0
    n_dom = 0
    if resolved.any():
        r = np.flatnonzero(resolved)
        n_dom = 1 + int(((np.diff(r) > 1) | (np.diff(code[r]) != 0)).sum())
    out = {"mean_det": float(det.mean()),
           "frac_resolved": float(resolved.mean()),
           "n_domains": n_dom}
    edge = resolved[:-1] & resolved[1:] & (code[:-1] != code[1:])
    b = np.zeros(len(det), dtype=bool)
    b[:-1] |= edge
    b[1:] |= edge
    out["n_boundary"] = int(b.sum())
    if tau is not None:
        out["boundary_tau_mean"] = float(tau[b].mean()) if b.any() else 0.0
    return out

# ---------------- Bond stage (P 1211/1212: pair = minimum meaning) ---------

BOND_DET = 0.5      # lock (kernel gate chain: radiate 0.30 <= cap 0.45 < lock 0.5 < knee 0.85)
RADIATE_DET = 0.30  # break floor == kernel D_GATE

def bonded_update(det, bonded_prev, lock=BOND_DET, floor=RADIATE_DET):
    """Stateful pair-bond update — binding energy IS hysteresis (MEASURED,
    kernel F-series 2026-08-31: a stateless readout let a synchronized noise
    dip kill a seeded pair in ~10 ticks). Pair i = particles (2i, 2i+1), the
    hi/lo seeding convention.
      FORM (asymmetric): one mate LOCKED (>= lock) + the other at least
        FORMED (>= floor) — simultaneous double-lock seesaws under noise and
        the bond never forms (measured).
      HOLD: while both mates stay >= the radiate floor.
      BREAK: only when a mate falls below the radiate floor — not at lock.
    v0 legal-byte = any hi/lo pair (all 256 exist at the byte rung;
    char_byte attestation = the NEXT rung's upward binding). This is the
    `bonded` seam field: written by Phase B, read by Phase A to gate
    persistence-vs-degrade."""
    assert len(det) % 2 == 0, "nibble lattice must be pair-aligned (byte = 2 particles)"
    hi, lo = det[0::2], det[1::2]
    form = ((hi >= lock) & (lo >= floor)) | ((lo >= lock) & (hi >= floor))
    hold = (hi >= floor) & (lo >= floor)
    return np.where(bonded_prev, hold, form)

def phase_b_bonded(amp, tau_pair, bonded_prev, f):
    """Bond-stage Phase B: per-particle readouts as before; the CLOCK moves to
    the pair rung — time begins at meaning. tau_pair ticks on pair COMPLETION
    (a bond forming = the throat event; re-formation after a break ticks
    again, and a standing bond is not a repeated crossing). Lone-nibble
    sharpening ticks NO clock — pre-temporal (P 1211/1212); per-particle Δdet
    front-tracking remains available as a sub-meaning diagnostic via phase_b()."""
    det = readout_det(amp)
    code = amp.argmax(axis=1)
    compliance = f(det)
    bonded = bonded_update(det, bonded_prev)
    completed = bonded & ~bonded_prev
    tau_pair[completed] += 1
    return det, code, compliance, bonded, completed

# ---------------- Phase B: Taichi kernel twin ------------------------------

ti.init(arch=ti.cpu, default_fp=ti.f64)

@ti.kernel
def phase_b_kernel(amp: ti.types.ndarray(), det_prev: ti.types.ndarray(),
                   det: ti.types.ndarray(), code: ti.types.ndarray(),
                   compliance: ti.types.ndarray(), tau: ti.types.ndarray(),
                   d0: ti.f64, k: ti.f64, front_threshold: ti.f64):
    for i in range(amp.shape[0]):
        h = 0.0
        best, bestp = 0, 0.0
        for c in range(NCODES):
            p = amp[i, c]
            if p > 1e-12:
                h += -p * ti.log(p)
            if p > bestp:
                bestp, best = p, c
        d = 1.0 - h / H_MAX
        d = ti.min(ti.max(d, 0.0), 1.0)
        det[i] = d
        code[i] = best
        compliance[i] = 1.0 / (1.0 + ti.exp(k * (d - d0)))
        if d - det_prev[i] > front_threshold:
            tau[i] += 1

# ---------------- Stub Phase A (test harness, NOT Silas's kernel) ----------

def stub_kernel_step(amp, det, code, compliance, c0=0.35, kappa=1.0):
    """Pull each particle toward its strongest higher-det neighbour's peak at
    rate c_local — seam-literal Phase A: rate = c0*g(compliance[p]) of the
    RECEIVING particle only (the medium yields; the source radiates freely,
    Kernel #6 — a source-gated rate would stop resolved matter radiating).
    Coupling g = 1 - kappa*(1 - compliance), the intent-corrected form
    (fronts stall in STIFF matter; the seam #5 literal g = 1 - k*compliance
    stalls them in chaos instead — flagged). Deterministic (T at floor, no
    noise) so the self-checks are exact."""
    n = len(det)
    c_local = c0 * (1.0 - kappa * (1.0 - compliance))
    # EPS_RADIATE (absolute): a particle radiates only once substantially FORMED
    # (Kernel #6: force is sourced at a resolved boundary). Without it the onset
    # wave crosses any medium at 1 cell/tick (condition-limited) and c_local
    # never limits front speed (measured: stiff band gave ZERO dilation). Gate
    # is absolute, not receiver-relative: a relative gate strangles the climb
    # (a cell needs a source 0.15 above itself forever - measured stall).
    new = amp.copy()
    for i in range(n):
        q = -1
        for j in (i - 1, i + 1):
            if 0 <= j < n and det[j] > det[i] and det[j] > EPS_RADIATE and (q < 0 or det[j] > det[q]):
                q = j
        if q >= 0:
            target = np.zeros(NCODES); target[code[q]] = 1.0
            new[i] += c_local[i] * (target - amp[i])
    return new / new.sum(axis=1, keepdims=True)

# ---------------- Toy runs / self-checks -----------------------------------

def one_hot_field(n, seed_idx=(), seed_code=3, peak=None):
    amp = np.full((n, NCODES), 1.0 / NCODES)
    for i in seed_idx:
        if peak is None:
            amp[i] = 0.0; amp[i, seed_code] = 1.0
        else:
            amp[i] = (1.0 - peak) / (NCODES - 1); amp[i, seed_code] = peak
    return amp

def run(amp, f, ticks, front_threshold=0.05, preset_compliance=None):
    n = amp.shape[0]
    det_prev = readout_det(amp)
    tau = np.zeros(n, dtype=np.int64)
    tau_hist, det_hist, T = [], [], 1.0
    for _ in range(ticks):
        det = readout_det(amp); code = amp.argmax(axis=1)
        compliance = f(det) if preset_compliance is None else preset_compliance(det)
        amp = stub_kernel_step(amp, det, code, compliance)          # Phase A
        det, code, compliance, _ = phase_b(amp, det_prev, tau, f,   # Phase B
                                           front_threshold)
        det_prev = det
        T = anneal(T)
        tau_hist.append(tau.copy()); det_hist.append(det.copy())
    return amp, det_prev, tau, np.array(tau_hist), np.array(det_hist), T

def main():
    rep = []

    # [1] Readout analytics: uniform -> det 0; one-hot -> det 1.
    a = one_hot_field(2, seed_idx=[1])
    d = readout_det(a)
    assert abs(d[0]) < 1e-9 and abs(d[1] - 1.0) < 1e-9
    rep.append("[1] det readout: uniform->0, one-hot->1  OK")

    # [2] Taichi kernel twin == numpy reference (same field, same knobs).
    rng = np.random.default_rng(7)
    amp = rng.random((500, NCODES)); amp /= amp.sum(1, keepdims=True)
    det_prev = np.full(500, 0.0)
    tau_np = np.zeros(500, dtype=np.int64)
    det_n, code_n, comp_n, _ = phase_b(amp, det_prev, tau_np, f_knee)
    det_t = np.zeros(500); code_t = np.zeros(500, dtype=np.int32)
    comp_t = np.zeros(500); tau_t = np.zeros(500, dtype=np.int64)
    phase_b_kernel(amp, det_prev, det_t, code_t, comp_t, tau_t, 0.85, 24.0, 0.05)
    assert np.allclose(det_n, det_t, atol=1e-9) and np.allclose(comp_n, comp_t, atol=1e-9)
    assert (code_n == code_t).all() and (tau_np == tau_t).all()
    rep.append("[2] taichi phase_b_kernel == numpy reference (500 random particles)  OK")

    # [3] Knee self-focuses the front; linear stays mushy. Compared at EQUAL
    # FRONT POSITION (each run's own tick when its front reaches mid-lattice),
    # not at equal tick — end-state comparison is vacuous once both saturate.
    n, ticks, mid = 300, 900, 150
    seed = one_hot_field(n, seed_idx=[0])
    _, det_k, tau_k, hist_k, dets_k, T_end = run(seed.copy(), f_knee, ticks)
    _, det_l, tau_l, hist_l, dets_l, _ = run(seed.copy(), f_linear, ticks)
    def at_front_mid(dets):
        for t in range(len(dets)):
            r = np.flatnonzero(dets[t] > 0.8)
            if len(r) and r.max() >= mid:
                return t, int(((dets[t] > 0.2) & (dets[t] < 0.8)).sum())
        return None, None
    tick_k, edge_k = at_front_mid(dets_k)
    tick_l, edge_l = at_front_mid(dets_l)
    assert tick_k is not None and tick_l is not None, (tick_k, tick_l)
    assert edge_k < edge_l, (edge_k, edge_l)
    rep.append(f"[3] self-focus at equal front position (index {mid}): edge width "
               f"knee={edge_k} (tick {tick_k}) < linear={edge_l} (tick {tick_l})  OK")

    # [4] tau monotone everywhere, all runs (can't un-cross the throat).
    for hist in (hist_k, hist_l):
        assert (np.diff(hist, axis=0) >= 0).all()
    rep.append("[4] tau monotone non-decreasing (both runs, every particle)  OK")

    # [5] Anneal asymptotic: above floor, converging to it.
    assert T_end > 0.05 and T_end - 0.05 < 0.1
    rep.append(f"[5] T annealed to {T_end:.4f} > floor 0.05, asymptotic  OK")

    # [6] DILATION: identical lanes + fronts; lane B carries a stiff band
    # (compliance clamped low over [m0,m1) — the stand-in for resolved mass).
    # The clamp isolates the compliance->c channel Phase B feeds the kernel;
    # a pre-RESOLVED band would also radiate its own front in this stub
    # (det-biased pull), which would measure the wrong thing.
    n, ticks, m0, m1 = 240, 1600, 80, 140
    C_BAND = 0.1
    def stiff_band(det):
        c = f_knee(det); c[m0:m1] = np.minimum(c[m0:m1], C_BAND); return c
    laneA = one_hot_field(n, seed_idx=[0])
    laneB = one_hot_field(n, seed_idx=[0])
    _, _, tauA, histA, _, _ = run(laneA, f_knee, ticks)
    _, _, tauB, histB, _, _ = run(laneB, f_knee, ticks,
                                  preset_compliance=stiff_band)
    probe = m1 + 40                                   # beyond the band
    def arrival(hist, i):
        t = np.flatnonzero(hist[:, i] > 0)
        return int(t[0]) if len(t) else ticks + 1
    artA, artB = arrival(histA, probe), arrival(histB, probe)
    assert artA <= ticks and artB > artA, (artA, artB)
    # near-frozen interior clock: total throat-crossings accrued in the band
    # vs the same indices in the free lane, full run.
    tau_band = histB[-1, m0 + 5:m1 - 5].mean()
    tau_free = histA[-1, m0 + 5:m1 - 5].mean()
    assert tau_band < tau_free, (tau_band, tau_free)
    rep.append(f"[6] dilation: front reaches probe at tick {artA} (free lane) vs "
               f"{artB} (through stiff band); band tau/particle {tau_band:.2f} < "
               f"free-lane {tau_free:.2f} — mass slows time  OK")

    # [7] observables sane on the focused field.
    obs = observables(det_k, np.argmax(seed, axis=1))
    rep.append(f"[7] observables v0: {obs}")

    # [8] Bond stage (measured spec, kernel F-series): asymmetric formation,
    # hysteresis hold, break only at radiate floor; completion is the throat event.
    bu = lambda d, p: bonded_update(np.array(d, float), np.array(p, bool))
    assert bu([0.9, 0.35], [False])[0]                     # locked + formed -> FORM (asymmetric)
    assert not bu([0.45, 0.45], [False])[0]                # no lock -> no bond (seesaw guard)
    assert not bu([0.9, 0.2], [False])[0]                  # mate below radiate floor -> no form
    assert bu([0.40, 0.35], [True])[0]                     # hysteresis: holds below lock
    assert not bu([0.29, 0.9], [True])[0]                  # break only below radiate floor
    a8 = one_hot_field(6, seed_idx=[0, 1, 2], seed_code=3)   # pair0 full, pair1 half, pair2 chaos
    tau_pair = np.zeros(3, dtype=np.int64)
    det8, code8, comp8, bonded, completed = phase_b_bonded(a8, tau_pair, np.zeros(3, bool), f_knee)
    assert list(bonded) == [True, False, False], bonded    # pair1 mate at chaos < floor: no bond
    assert list(tau_pair) == [1, 0, 0], tau_pair           # lone nibble (pair1): no clock
    _, _, _, bonded2, completed2 = phase_b_bonded(a8, tau_pair, bonded, f_knee)
    assert list(tau_pair) == [1, 0, 0] and not completed2.any()   # standing bond: no re-tick
    a8[1] = 1.0 / NCODES                                   # collapse pair0's lo mate below floor
    _, _, _, bonded3, _ = phase_b_bonded(a8, tau_pair, bonded2, f_knee)
    assert not bonded3[0]                                  # broken
    a8[1] = 0.0; a8[1, 3] = 1.0                            # re-complete
    _, _, _, bonded4, completed4 = phase_b_bonded(a8, tau_pair, bonded3, f_knee)
    assert bonded4[0] and tau_pair[0] == 2                 # re-formation IS a crossing
    # noise persistence (mirrors kernel F1 @ T=0.02): synchronized dips below
    # LOCK (0.42) but above the radiate floor — the hysteretic bond must ride
    # through with ZERO re-ticks; a memoryless rule flaps on every dip and
    # would tick tau_pair spuriously (the measured stateless kill).
    prev = np.array([True]); reticks = 0; stateless_flaps = 0
    for t in range(3000):
        d = [0.42, 0.42] if t % 100 == 50 else [0.84, 0.84]
        now = bu(d, prev)
        if now[0] and not prev[0]:
            reticks += 1
        prev = now
        if not bu(d, [False])[0]:
            stateless_flaps += 1
    assert prev[0] and reticks == 0 and stateless_flaps == 30, (prev, reticks, stateless_flaps)
    rep.append("[8] bond stage (measured spec): asymmetric form, hysteresis hold, break at "
               "radiate floor; tau_pair ticks completions only, lone nibble pre-temporal; "
               "3000-tick synced-dip persistence: bond holds, 0 spurious re-ticks "
               "(memoryless flaps 30/30)  OK")

    print("\n".join(rep))
    print("ALL SELF-CHECKS PASS")

if __name__ == "__main__":
    main()
