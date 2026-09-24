#!/usr/bin/env python3
"""field_engine_v0 — the balancing engine, CPU deterministic reference (oracle).

One operation, one field, one flow (docs/physics-basis.md, docs/architecture.md):

  matter is a density imbalance in a field that relaxes by diffusion; each point
  sees only its local differential and adapts toward reducing it (single-point
  force balancing); inverse-square attraction between imbalances emerges from the
  field relaxing across three dimensions, as a product of the imbalances, with no
  frequency term and no pairwise force sum.

The flow, per tick, is the whole of it:
  1. deposit  each particle's mass (its imbalance) onto the field grid
  2. relax    the field toward balance by local diffusion (Jacobi) — this IS the
              single-point balancing: every cell moves toward the average of its
              neighbours minus its own source, reducing its local differential
  3. read     the local field gradient at each particle (single point, no pairs)
  4. move     each particle down the gradient (the field draws imbalances together)

Deterministic end to end: no rng, no seed, no noise. Positions are injected as a
deterministic low-discrepancy spread — initial position carries no structure and
no identity, so any structure that appears is the physics, not the injection.
This is the oracle; the Taichi GPU kernel mirrors it and must match (oracle-first,
architecture.md "Engine substrate").

Run:  python3 field/field_engine_v0.py --ticks 200 --grid 48
State is checkpointed each chunk (resumable); the end state is always preserved.
"""
import argparse
import json
import os
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from field_engine_load_v0 import load_particles  # noqa: E402


def halton(i, base):
    """i-th point of the deterministic van der Corput / Halton low-discrepancy
    sequence in base `base`. No randomness; identical every run."""
    f, r = 1.0, 0.0
    while i > 0:
        f /= base
        r += f * (i % base)
        i //= base
    return r


def inject(n, box):
    """Free-float injection: deterministic 3D Halton spread across the box.
    Position means nothing — it is uncorrelated with identity and mass — so the
    only thing that can organise the cloud is the field."""
    b = (2, 3, 5)
    pos = np.empty((n, 3), dtype=np.float64)
    for k in range(3):
        pos[:, k] = np.array([halton(i + 1, b[k]) for i in range(n)])
    return pos * box


def deposit(pos, mass, grid, box):
    """Scatter particle mass onto the grid (nearest cell). Total mass conserved."""
    rho = np.zeros((grid, grid, grid), dtype=np.float64)
    idx = np.clip((pos / box * grid).astype(np.int64), 0, grid - 1)
    np.add.at(rho, (idx[:, 0], idx[:, 1], idx[:, 2]), mass)
    return rho, idx


def relax(phi, rho, iters, h):
    """Relax the field toward balance by local diffusion (Jacobi on the Poisson
    problem lap(phi) = rho). Each cell adapts toward its neighbours' average minus
    its own source — single-point balancing, applied everywhere, always partial in
    a finite pass (time confounds; the vacuum optimum is the direction, not the
    destination). Periodic box. Deterministic."""
    src = rho * (h * h) / 6.0
    for _ in range(iters):
        nb = (np.roll(phi, 1, 0) + np.roll(phi, -1, 0) +
              np.roll(phi, 1, 1) + np.roll(phi, -1, 1) +
              np.roll(phi, 1, 2) + np.roll(phi, -1, 2))
        phi = nb / 6.0 - src
    return phi


def gradient_at(phi, idx, box, grid):
    """Local field gradient at each particle's cell (central difference, periodic).
    Single point: a particle reads only the field where it is."""
    h = box / grid
    gx = (np.roll(phi, -1, 0) - np.roll(phi, 1, 0)) / (2 * h)
    gy = (np.roll(phi, -1, 1) - np.roll(phi, 1, 1)) / (2 * h)
    gz = (np.roll(phi, -1, 2) - np.roll(phi, 1, 2)) / (2 * h)
    i, j, k = idx[:, 0], idx[:, 1], idx[:, 2]
    return np.stack([gx[i, j, k], gy[i, j, k], gz[i, j, k]], axis=1)


def observables(pos, mass, box):
    """What the monitor reads: is the cloud organising, and is it settling?
    Reported, never used to drive the physics. No pairwise computation. All
    metrics are periodic-box-safe (they bin positions, so a wrapped particle is
    measured where it actually is, not flung across the box)."""
    g = 32
    cells = np.clip((pos / box * g).astype(np.int64), 0, g - 1)
    flat = (cells[:, 0] * g + cells[:, 1]) * g + cells[:, 2]
    cell_mass = np.bincount(flat, weights=mass, minlength=g ** 3)
    total = cell_mass.sum()
    # occupancy: fraction of cells holding any mass — falls as mass gathers
    occ = float((cell_mass > 0).sum()) / float(g ** 3)
    # concentration: fraction of total mass in the densest 1% of cells — rises as
    # imbalances merge into clumps. This is the gathering, read straight off the
    # field, no pairwise distance.
    k = max(1, (g ** 3) // 100)
    top = np.sort(cell_mass)[-k:].sum()
    conc = float(top / total)
    return {"occupancy": occ, "top1pct_mass_fraction": conc,
            "n_occupied_cells": int((cell_mass > 0).sum())}


def run(ticks, grid, relax_iters, step, box, chunk, state_path, report_path):
    particles = load_particles()
    n = len(particles)
    mass = np.array([p["mass"] for p in particles], dtype=np.float64)

    # resume if a checkpoint exists; else free-float inject
    start = 0
    hist = []
    if os.path.exists(state_path):
        st = np.load(state_path, allow_pickle=True)
        pos = st["pos"]
        start = int(st["tick"])
        hist = list(st["hist"])
        assert len(pos) == n, "checkpoint particle count != current data"
    else:
        pos = inject(n, box)

    h = box / grid
    phi = np.zeros((grid, grid, grid), dtype=np.float64)
    t0 = time.time()
    for t in range(start, ticks):
        rho, idx = deposit(pos, mass, grid, box)
        rho = rho - rho.mean()                 # only differentials matter
        phi = relax(phi, rho, relax_iters, h)  # warm-started each tick
        g = gradient_at(phi, idx, box, grid)
        pos = (pos - step * g) % box           # move down-gradient; periodic box
        if (t + 1) % chunk == 0 or t + 1 == ticks:
            obs = observables(pos, mass, box)
            obs["tick"] = t + 1
            obs["elapsed_s"] = round(time.time() - t0, 1)
            hist.append(obs)
            np.savez(state_path, pos=pos, tick=t + 1,
                     hist=np.array(hist, dtype=object))
            print(json.dumps(obs), flush=True)

    # settling verdict: is the concentration still moving?
    cs = [hstep["top1pct_mass_fraction"] for hstep in hist]
    settling = None
    if len(cs) >= 3:
        recent = abs(cs[-1] - cs[-2]) / (abs(cs[-2]) + 1e-12)
        settling = {"last_rel_change": recent, "stabilising": recent < 1e-3}
    rep = {"artifact": "field-engine-v0-oracle", "n_particles": n,
           "ticks": ticks, "grid": grid, "deterministic": True,
           "conc_first": cs[0] if cs else None, "conc_last": cs[-1] if cs else None,
           "settling": settling, "history": hist}
    with open(report_path, "w") as f:
        json.dump(rep, f, indent=1, default=str)
    return rep


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--grid", type=int, default=48)
    ap.add_argument("--relax-iters", type=int, default=40)
    ap.add_argument("--step", type=float, default=0.5)
    ap.add_argument("--box", type=float, default=100.0)
    ap.add_argument("--chunk", type=int, default=10)
    ap.add_argument("--state", default=os.path.join(HERE, "field-engine-v0-state.npz"))
    ap.add_argument("--report", default=os.path.join(HERE, "field-engine-v0-report.json"))
    ap.add_argument("--fresh", action="store_true", help="ignore any checkpoint")
    a = ap.parse_args()
    if a.fresh and os.path.exists(a.state):
        os.remove(a.state)
    rep = run(a.ticks, a.grid, a.relax_iters, a.step, a.box, a.chunk,
              a.state, a.report)
    print(json.dumps({k: rep[k] for k in
                      ("n_particles", "conc_first", "conc_last", "settling")},
                     indent=1, default=str))
    return 0


if __name__ == "__main__":
    sys.exit(main())
