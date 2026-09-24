"""Field-engine INTEGRATION v0 — real Phase A x real Phase B, stub retired.

Phase A = silas's diffuse() (exchange/field-engine-diffuse-v0.py, proofs P1-P5
re-run PASS on this seat 2026-08-30). Phase B = planner's timestep.py (its own
checks [1]-[7] PASS). This file couples the two real halves on a toy lattice
per the seam's per-tick contract: Phase A writes ONLY amp from LAST tick's
compliance (one-tick-apart, no races); Phase B writes det/code/compliance/tau.
Both modules' taichi twins are individually proven == their numpy references,
so the coupled loop runs the numpy references; the ti-coupled run happens at
corpus scale on silas's seat.

Run: .venv-snode python integration_v0.py  -> I1-I5 hard asserts.
"""

import importlib.util
import numpy as np


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


import os
_HERE = os.path.dirname(os.path.abspath(__file__))
ts = _load("timestep_v0", os.path.join(_HERE, "timestep.py"))
dv = _load("diffuse_v0", os.path.join(_HERE, "diffuse.py"))
# ^ seat-local adaptation (silas seat): planner's shipped bytes hardcoded his
#   /home/planner paths; modules here are the same shipped bytes copied local.

T0, T_FLOOR, LAM = 0.02, 0.002, 0.995   # time-step-owned (seam §Owned config)
FRONT_THR = 0.05


def coupled(n, ticks, preset=None, seed=(0,), seed_code=9, rng_seed=7):
    """The integrated per-tick loop, exactly the seam contract."""
    rng = np.random.default_rng(rng_seed)
    amp = np.full((n, 16), 1.0 / 16.0)
    for i in seed:
        amp[i] = 1e-9; amp[i, seed_code] = 1.0; amp[i] /= amp[i].sum()
    det_prev = ts.readout_det(amp)
    compliance = ts.f_knee(det_prev) if preset is None else preset(det_prev)
    tau = np.zeros(n, np.int64)
    T = T0
    det_hist, tau_hist, obs = [], [], []
    for t in range(ticks):
        # PHASE A (kernel, silas): writes ONLY amp; reads last tick's compliance
        amp = dv.diffuse_np(amp, compliance, T, rng)
        # PHASE B (time-step, planner): det/code/compliance/tau
        det, code, compliance, _ = ts.phase_b(amp, det_prev, tau, ts.f_knee, FRONT_THR)
        if preset is not None:
            compliance = preset(det)
        det_prev = det
        T = ts.anneal(T, T_FLOOR, LAM)
        det_hist.append(det.copy()); tau_hist.append(tau.copy())
        if t % 100 == 0:
            assert np.all(amp > -1e-12) and np.allclose(amp.sum(1), 1.0, atol=1e-9)
            obs.append((t, ts.observables(det, code)))
    return amp, np.array(det_hist), np.array(tau_hist), code, T, obs


def main():
    rep = []

    # [I1] formation + identity: seeded front resolves the lattice with the seed code.
    n, ticks = 160, 900
    amp, dh, th, code, T_end, obs = coupled(n, ticks)
    det = dh[-1]
    resolved = det > 0.8
    assert resolved.mean() > 0.85, resolved.mean()
    assert (code[resolved] == 9).all()
    rep.append(f"[I1] formation: {resolved.mean()*100:.0f}% of lattice det>0.8, every "
               f"resolved cell carries seed code 9, max det {det.max():.3f} — full "
               f"formation through the coupled loop (his P1, via my readout)")

    # [I2] tau monotone; no noise-induced tau ahead of the front.
    assert (np.diff(th, axis=0) >= 0).all()
    assert th[100, n - 10] == 0
    rep.append("[I2] tau monotone everywhere; far-field tau stays 0 under T-jitter "
               "(noise dtau << front_threshold) — clean throat-count")

    # [I3] dilation with the REAL kernel: stiff band (compliance clamped 0.1,
    # resolved-mass stand-in) vs free lane, my Phase-B readouts.
    n2, t2, m0, m1 = 240, 1600, 80, 140
    def band(det):
        c = ts.f_knee(det); c[m0:m1] = np.minimum(c[m0:m1], 0.1); return c
    _, _, thA, _, _, _ = coupled(n2, t2)
    _, _, thB, _, _, _ = coupled(n2, t2, preset=band)
    probe = m1 + 40
    def arrive(h):
        z = np.nonzero(h[:, probe] > 0)[0]
        return int(z[0]) if len(z) else t2 + 1
    aA, aB = arrive(thA), arrive(thB)
    assert aA <= t2 and aB > aA, (aA, aB)
    tb = thB[-1, m0 + 5:m1 - 5].mean()
    tf = thA[-1, m0 + 5:m1 - 5].mean()
    assert tb < tf, (tb, tf)
    rep.append(f"[I3] dilation: probe arrival tick {aA} free vs {aB} through band "
               f"({aB/max(aA,1):.1f}x); interior clock tau {tb:.2f} vs free {tf:.2f} "
               f"— mass slows time, real kernel + real time-step")

    # [I4] anneal asymptotic above floor.
    assert T_end > T_FLOOR
    rep.append(f"[I4] T annealed {T0} -> {T_end:.5f} > floor {T_FLOOR} (never zero)")

    # [I5] observables trajectory (complexity readout, provisional).
    line = " | ".join(f"t={t}: det {o['mean_det']:.2f} res {o['frac_resolved']:.2f} "
                      f"dom {o['n_domains']}" for t, o in obs[::2])
    rep.append(f"[I5] observables: {line}")

    print("\n".join(rep))
    print("ALL INTEGRATION CHECKS PASS — the two real halves run coupled.")


if __name__ == "__main__":
    main()
