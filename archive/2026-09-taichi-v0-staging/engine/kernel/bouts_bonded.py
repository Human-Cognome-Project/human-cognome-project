#!/usr/bin/env python3
"""B-series at the BONDED level — the bout questions re-asked where meaning lives.

Loop = the real two-half contract: Phase A tick_v02 (binding.py, reads bonded,
writes ONLY amp) x Phase B phase_b_bonded (Planner's shipped timestep — writes
det/code/compliance/bonded and ticks tau_pair on pair COMPLETION only; time
begins at meaning). Per Planner: tau_pair trajectories ride with
n_boundary/boundary_tau_mean per snap.

PREDICTION (stated before the run): at the bonded level the old territory war
is STRUCTURALLY impossible — radiation reaches only unlocked receivers and
valency travels only through pair-mates, so locked matter cannot be converted
by neighbouring matter at all. Bouts measure what remains alive: persistence,
interfaces, and the meaning-clock. (If conversion DOES appear, the prediction
is falsified and that is the finding.)

  BB1 adjacent equal-weight byte pairs   -> interface + clock honesty
  BB2 adjacent unequal-weight byte pairs -> conversion structurally absent?
  BB3 mosaic: 4 full pairs + 4 lone nibbles -> combines tick BIRTHS on
      tau_pair; lone-vs-paired fates in one field
Mechanical asserts only; outcomes reported. Sparse-toy caveat stands (P):
gates are scaffolding, density is the real stabilizer.
"""
import numpy as np
import binding as b
import diffuse as dv
import timestep as ts


def coupled_bonded(n, ticks, seeds, rng_seed=7, snap_every=500,
                   T0=0.02, floor=0.002, lam=0.995):
    """seeds = [(pos, code)]; even-aligned full pairs start BONDED (data byte)."""
    rng = np.random.default_rng(rng_seed)
    amp = np.full((n, 16), b.UNIFORM)
    seeded = set()
    for pos, c in seeds:
        amp[pos] = 1e-9; amp[pos, c] = 1.0; amp[pos] /= amp[pos].sum()
        seeded.add(pos)
    bonded = np.zeros(n // 2, dtype=bool)
    for pos in seeded:
        if (pos ^ 1) in seeded:
            bonded[pos // 2] = True
    tau_pair = np.zeros(n // 2, dtype=np.int64)
    T = T0
    snaps = []
    for t in range(ticks):
        det = dv.det_of(amp)
        compliance = ts.f_knee(det)
        amp = b.tick_v02(amp, compliance, T, rng, np.repeat(bonded, 2))
        det, code, compliance, bonded, completed = ts.phase_b_bonded(
            amp, tau_pair, bonded, ts.f_knee)
        T = max(floor, T * lam)
        if t % snap_every == 0 or t == ticks - 1:
            assert np.all(amp > -1e-12) and np.allclose(amp.sum(1), 1.0, atol=1e-9)
            obs = ts.observables(det, code, tau=np.repeat(tau_pair, 2))
            snaps.append((t, det.copy(), code.copy(), bonded.copy(), tau_pair.copy(), obs))
    return snaps


def report(name, snaps, seeds):
    print(f"— {name} —  seeds: {[(p, c, int(dv.WEIGHT[c])) for p, c in seeds]}")
    for t, det, code, bonded, tau_pair, obs in (snaps[0], snaps[len(snaps)//2], snaps[-1]):
        locked = det >= b.LOCK
        terr = {int(c): int((code[locked] == c).sum()) for c in np.unique(code[locked])} if locked.any() else {}
        tp = tau_pair[tau_pair > 0]
        print(f"  t={t:5d}  locked_terr={terr}  n_bonded_pairs={int(bonded.sum())}  "
              f"n_boundary={obs['n_boundary']}  btau={obs.get('boundary_tau_mean', 0.0):.1f}  "
              f"tau_pair: ticked={len(tp)} max={tau_pair.max()} "
              f"(births+recompletions on the meaning clock)")
    return snaps[-1]


def main():
    n = 64

    # BB1 — adjacent equal-weight byte pairs: 0x33 (3,3) | 0xCC (12,12)
    seeds = [(28, 3), (29, 3), (30, 12), (31, 12)]
    t, det, code, bonded, tau_pair, obs = report(
        "BB1 adjacent equal pairs 0x33|0xCC", coupled_bonded(n, 4000, seeds), seeds)
    assert bonded[14] and bonded[15], "BB1: a seeded pair lost its bond"
    print(f"  interface verdict: both pairs bonded at t=3999, codes "
          f"{code[28]}/{code[29]} | {code[30]}/{code[31]} — "
          f"{'IDENTITIES INTACT (no conversion across the contact)' if (code[28]==3 and code[31]==12) else 'CONVERSION OCCURRED'}; "
          f"stable-bond clock honesty: max tau_pair={tau_pair.max()} (seeded bonds never re-tick unless broken)")

    # BB2 — adjacent unequal pairs: 0x11 (1,1) w=2 | 0xFF (15,15) w=5
    seeds = [(28, 1), (29, 1), (30, 15), (31, 15)]
    t, det, code, bonded, tau_pair, obs = report(
        "BB2 adjacent unequal pairs 0x11|0xFF", coupled_bonded(n, 4000, seeds), seeds)
    light_alive = bonded[14] and code[28] == 1 and code[29] == 1
    print(f"  weight-war verdict: light pair {'SURVIVES INTACT beside w=5 neighbour' if light_alive else 'LOST — prediction falsified'} "
          f"(det {det[28]:.2f}/{det[29]:.2f} vs heavy {det[30]:.2f}/{det[31]:.2f})")

    # BB3 — mosaic: 4 full pairs + 4 lone nibbles
    full = [(8, 2), (9, 2), (20, 7), (21, 7), (40, 11), (41, 11), (52, 15), (53, 15)]
    lone = [(14, 5), (26, 9), (34, 6), (46, 13)]
    seeds = full + lone
    snaps = coupled_bonded(n, 4000, seeds)
    t, det, code, bonded, tau_pair, obs = report("BB3 mosaic 4 pairs + 4 lone nibbles", snaps, seeds)
    lone_fates = []
    for pos, c in lone:
        pair_i = pos // 2
        if bonded[pair_i]:
            lone_fates.append(f"{pos}:COMBINED(tau_pair={tau_pair[pair_i]})")
        else:
            lone_fates.append(f"{pos}:{'degraded' if det[pos] < b.LOCK else 'lone-locked?'}")
    print(f"  lone-nibble fates: {lone_fates}")
    births = int((tau_pair > 0).sum())
    print(f"  meaning-clock births this run: {births} pair-completions ticked "
          f"(seeded bonds are silent — only new meaning ticks time)")

    print("ALL BONDED-BOUT MECHANICS HELD — outcomes above are the measurements.")


if __name__ == "__main__":
    main()
