#!/usr/bin/env python3
"""Field-engine BOUTS — (b) multi-code seeding on the integrated toy (silas).

Extends integration_v0.coupled() to per-seed codes. Three bouts:
  B1 equal-weight:   code 3 (0011, w=3) vs code 12 (1100, w=3), opposite ends
                     -> boundary character (wall / flip-through / stable interface)
  B2 unequal-weight: code 1 (0001, w=2) vs code 15 (1111, w=5)
                     -> first intrinsic-weight-shadow observable: weight fed as
                        CAUSE, territory read as EFFECT (frequency never fed)
  B3 mosaic:         8 random seeds, random codes -> domain-count + boundary-cell
                     trajectories (complexity-observable candidates)
Control lane ((c) riding along): preset compliance==1 -> c_local==c0 regardless
of kappa == the flat-c cosmos. (Equivalent to kappa=0 without patching the
kernel module; diffuse_np's g() default binds KAPPA at def time — v1 nit: expose
kappa as a diffuse_np param.)

Asserts = MECHANICS only (simplex held, seeds take, tau monotone). Territorial
outcomes are MEASURED AND REPORTED, never asserted — measured-not-named (P):
if the heavy code loses ground, that's a finding, not a failure.
"""
import numpy as np
import integration_v0 as iv

ts, dv = iv.ts, iv.dv


CROSS_GATE = None   # set by main() for the gated variant pass
CROSS_EPS = 0.0     # Kernel #7 hard-vs-impossible: 0.0 = impossible; >0 = hard
D0_KNEE = 0.85      # Planner's stiffen knee (timestep f_knee default) — for the ordering assert


def coupled_multi(n, ticks, seeds, preset=None, rng_seed=7, snap_every=200):
    """integration_v0.coupled() with per-seed codes: seeds = [(pos, code), ...]."""
    if CROSS_GATE is not None:
        # gate-ordering invariant (Planner review): radiate <= lock < stiffen
        assert dv.D_GATE <= CROSS_GATE < D0_KNEE, (
            f"gate ordering violated: need {dv.D_GATE} <= {CROSS_GATE} < {D0_KNEE}")
    rng = np.random.default_rng(rng_seed)
    amp = np.full((n, 16), 1.0 / 16.0)
    for pos, c in seeds:
        amp[pos] = 1e-9; amp[pos, c] = 1.0; amp[pos] /= amp[pos].sum()
    det_prev = ts.readout_det(amp)
    compliance = ts.f_knee(det_prev) if preset is None else preset(det_prev)
    tau = np.zeros(n, np.int64)
    T = iv.T0
    snaps = []
    for t in range(ticks):
        amp = dv.diffuse_np(amp, compliance, T, rng,          # PHASE A (mine)
                            cross_gate=CROSS_GATE, cross_eps=CROSS_EPS)
        det, code, compliance, _ = ts.phase_b(amp, det_prev, tau, ts.f_knee, iv.FRONT_THR)
        if preset is not None:
            compliance = preset(det)
        det_prev = det
        T = ts.anneal(T, iv.T_FLOOR, iv.LAM)
        if t % snap_every == 0 or t == ticks - 1:
            assert np.all(amp > -1e-12) and np.allclose(amp.sum(1), 1.0, atol=1e-9)
            snaps.append((t, det.copy(), code.copy(), tau.copy()))
    return amp, snaps


def territory(det, code, resolved=0.5):
    """cells per code among resolved cells."""
    r = det > resolved
    return {int(c): int((code[r] == c).sum()) for c in np.unique(code[r])}, int((~r).sum())


def boundary_cells(det, resolved=0.5):
    """indices of unresolved cells lying BETWEEN resolved cells of different codes."""
    return int((~(det > resolved)).sum())


def report_bout(name, snaps, seeds, every=False):
    print(f"— {name} —  seeds: {[(p, c, int(dv.WEIGHT[c])) for p, c in seeds]}  (pos, code, weight)")
    shown = snaps if every or len(snaps) <= 3 else snaps[-3:]
    for t, det, code, tau in shown:
        terr, unres = territory(det, code)
        obs = ts.observables(det, code, tau=tau)   # Planner's border clock (re-shipped)
        print(f"  t={t:5d}  territory={terr}  unresolved_cells={unres}  "
              f"max_det={det.max():.3f}  n_boundary={obs['n_boundary']}  "
              f"boundary_tau_mean={obs.get('boundary_tau_mean', 0.0):.1f}")
    t, det, code, tau = snaps[-1]
    return det, code, tau


def main():
    # ── B1 equal-weight bout ──────────────────────────────────────────────
    n, seeds = 201, [(0, 3), (200, 12)]
    amp, snaps = coupled_multi(n, 4000, seeds)
    det, code, tau = report_bout("B1 equal-weight (3 vs 12, both w=3)", snaps, seeds)
    t3 = (det > 0.5) & (code == 3); t12 = (det > 0.5) & (code == 12)
    assert t3.sum() > 10 and t12.sum() > 10, "B1: a seed failed to take"
    # interface location + stability across the last two snapshots
    edge_now = int(np.nonzero(t3)[0].max())
    _, det_p, code_p, _ = snaps[-2]
    edge_prev = int(np.nonzero((det_p > 0.5) & (code_p == 3))[0].max())
    print(f"  interface: rightmost code-3 cell {edge_now} (prev snap {edge_prev}, "
          f"drift {edge_now - edge_prev:+d}); contested unresolved cells {boundary_cells(det)}")

    # ── B2 unequal-weight bout (weight-shadow) + flat-c control ──────────
    seeds = [(0, 1), (200, 15)]
    amp, snaps = coupled_multi(n, 4000, seeds)
    det, code, tau = report_bout("B2 unequal-weight (1 w=2 vs 15 w=5)", snaps, seeds)
    t1 = ((det > 0.5) & (code == 1)).sum(); t15 = ((det > 0.5) & (code == 15)).sum()
    assert t1 + t15 > 0, "B2: nothing resolved"
    print(f"  WEIGHT SHADOW: heavy(15,w=5) holds {t15} cells vs light(1,w=2) {t1} "
          f"-> {'heavy advantage' if t15 > t1 else 'light advantage' if t1 > t15 else 'draw'} "
          f"(measured, not asserted)")
    flat = lambda det: np.ones_like(det)
    amp, snaps = coupled_multi(n, 4000, seeds, preset=flat)
    det_f, code_f, _ = report_bout("B2-control flat-c (compliance==1 lane)", snaps, seeds)
    f1 = ((det_f > 0.5) & (code_f == 1)).sum(); f15 = ((det_f > 0.5) & (code_f == 15)).sum()
    print(f"  flat-c control: heavy {f15} vs light {f1} — compliance-coupling "
          f"{'changes' if (f15 > f1) != (t15 > t1) or abs(int(f15)-int(f1)) - abs(int(t15)-int(t1)) > 20 else 'does not flip'} the outcome")

    # ── B3 mosaic ─────────────────────────────────────────────────────────
    n3 = 301
    rng = np.random.default_rng(50)
    pos = np.sort(rng.choice(n3, 8, replace=False))
    seeds = [(int(p), int(c)) for p, c in zip(pos, rng.integers(0, 16, 8))]
    amp, snaps = coupled_multi(n3, 4000, seeds, snap_every=400)
    print(f"— B3 mosaic —  seeds: {seeds}")
    for t, det, code, tau in snaps:
        terr, unres = territory(det, code)
        print(f"  t={t:5d}  n_domains={len(terr)}  unresolved={unres}  "
              f"territories={terr}")
    _, det, code, tau = snaps[-1]
    assert np.all(np.diff([s[3].sum() for s in snaps]) >= 0), "B3: total tau not monotone"

    # ── B4 hard-vs-impossible (only meaningful gated; P's open, Planner's
    #    discriminator: border clock frozen ≈1 forever = impossible; ticking at
    #    the transmutation rate = hard/reconsolidation) ────────────────────
    if CROSS_GATE is not None:
        global CROSS_EPS
        seeds = [(0, 1), (200, 15)]
        for eps, label in ((0.0, "IMPOSSIBLE (eps=0)"), (0.02, "HARD (eps=0.02)")):
            CROSS_EPS = eps
            amp, snaps = coupled_multi(201, 12000, seeds, snap_every=2000)
            det, code, tau = report_bout(f"B4 border-clock {label}", snaps, seeds, every=True)
            t1 = ((det > 0.5) & (code == 1)).sum(); t15 = ((det > 0.5) & (code == 15)).sum()
            print(f"  endstate: light(1) {t1} vs heavy(15) {t15} cells "
                  f"{'— light domain CONVERTED (reconsolidation happened)' if t1 == 0 else '— light domain persists'}")
        CROSS_EPS = 0.0

    print("ALL BOUT MECHANICS HELD — outcomes above are the measurements.")


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        CROSS_GATE = float(sys.argv[1])
        print(f"=== GATED VARIANT: receiver cross_gate={CROSS_GATE} "
              f"(formed cells are not background for other forms) ===")
    main()
