#!/usr/bin/env python3
"""Equivalence and regression checks for the field engine.

Validation is kept outside both physics and harness behaviour. It compares the
current oracle/kernel paths against the validated v0 substrate and against each
other without becoming part of engine operation.
"""
import json
import time

import numpy as np

from field_engine_load_v0 import load_at_declared_grain
import field_engine_v0 as v0
from field_engine_physics import Pool, Oracle, make_kernel


def pdiff(a, b, box):
    d = np.abs(a - b)
    return float(np.minimum(d, box - d).max())


def check(a):
    g, T, box, iters, step = a.check_grid, a.check_ticks, a.box, a.relax_iters, a.step
    out = {"artifact": "field-engine-equivalence", "grid": g, "ticks": T, "checks": {}}

    # ---- (A)+(C): couplings off, RESOLUTION grain (v0's mass law) vs v0 ----
    particles, info = load_at_declared_grain("resolution")
    pool = Pool(particles)
    n = pool.n
    mass = np.array([p["mass"] for p in particles], dtype=np.float64)
    assert (mass == pool.base_mass).all()
    pos0 = v0.inject(n, box)
    h = box / g
    pos_v0, phi_v0 = pos0.copy(), np.zeros((g, g, g))
    for _ in range(T):
        rho, idx = v0.deposit(pos_v0, mass, g, box)
        rho = rho - rho.mean()
        phi_v0 = v0.relax(phi_v0, rho, iters, h)
        gr = v0.gradient_at(phi_v0, idx, box, g)
        pos_v0 = (pos_v0 - step * gr) % box

    orc = Oracle(pool, g, box, iters, step, False, False, False, periodic=True, cap=False)
    pos_c = pos0.copy()
    for _ in range(T):
        pos_c, _ = orc.tick(pos_c)
    out["checks"]["C_oracle_off_vs_v0"] = pdiff(pos_c, pos_v0, box)

    if a.arch != "numpy":
        ker = make_kernel(pool, g, box, iters, step, False, False, False, a.arch, periodic=True, cap=False)
        ker.pos.from_numpy(pos0)
        t0 = time.time()
        for _ in range(T):
            ker.tick()
        out["checks"]["A_kernel_off_vs_v0"] = pdiff(ker.pos.to_numpy(), pos_v0, box)
        out["kernel_off_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)

    # ---- (B): everything on, OPEN box, DECLARED grain, oracle vs kernel ----
    particles, info = load_at_declared_grain(a.grain)
    pool_o, pool_k = Pool(particles), Pool(particles)
    pos0 = v0.inject(pool_o.n, box)
    orc = Oracle(pool_o, g, box, iters, step, True, True, True, periodic=False)
    pos_o = pos0.copy()
    t0 = time.time()
    n_abs_o = 0
    for _ in range(T):
        pos_o, k_ = orc.tick(pos_o)
        n_abs_o += k_
    out["oracle_on_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)
    out["oracle_on_condensed"] = n_abs_o
    if a.arch != "numpy":
        ker = make_kernel(pool_k, g, box, iters, step, True, True, True, a.arch, periodic=False)
        ker.pos.from_numpy(pos0)
        t0 = time.time()
        n_abs_k = 0
        for _ in range(T):
            n_abs_k += ker.tick()
        ker.sync_pool()
        out["kernel_on_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)
        out["kernel_on_condensed"] = n_abs_k
        live = pool_o.alive & pool_k.alive
        out["checks"]["B_oracle_on_vs_kernel_on_pos"] = float(np.abs(pos_o[live] - ker.pos.to_numpy()[live]).max())
        out["checks"]["B_alive_equal"] = bool((pool_o.alive == pool_k.alive).all())
        out["checks"]["B_count_equal"] = bool((pool_o.count == pool_k.count).all())
        out["checks"]["B_parent_equal"] = bool((pool_o.parent == pool_k.parent).all())

    tol = 1e-6
    ok = all((v < tol) if isinstance(v, float) else v for v in out["checks"].values())
    out["equivalent"] = ok
    print(json.dumps(out, indent=1))
    return 0 if ok else 1


