#!/usr/bin/env python3
"""field_engine_kernel_v0 — the balancing engine on Taichi/CUDA (GPU mirror).

Mirrors field_engine_v0.py (the CPU oracle) exactly, on the GPU, per the
oracle-first mandate (docs/architecture.md "Engine substrate"): the portable CPU
reference is the deterministic oracle, this is its GPU twin, and the harness
asserts they agree on real hardware. Same one flow per tick:

    deposit imbalances -> relax the field by local diffusion -> read local
    gradient -> move down it.

Substrate is Taichi (the project's chosen kernel), f64, arch=cuda (GTX 1070).
No rng, no noise. Run:  python3 field/field_engine_kernel_v0.py --check
"""
import argparse
import json
import os
import sys
import time

import numpy as np
import taichi as ti

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from field_engine_load_v0 import load_particles          # noqa: E402
import field_engine_v0 as oracle                          # noqa: E402


@ti.data_oriented
class Engine:
    def __init__(self, n, grid, box, arch=ti.cuda):
        ti.init(arch=arch, default_fp=ti.f64, random_seed=0,
                offline_cache=False)
        self.n, self.grid, self.box = n, grid, box
        self.h = box / grid
        self.pos = ti.Vector.field(3, ti.f64, shape=n)
        self.mass = ti.field(ti.f64, shape=n)
        self.idx = ti.Vector.field(3, ti.i32, shape=n)
        self.rho = ti.field(ti.f64, shape=(grid, grid, grid))
        self.phi = ti.field(ti.f64, shape=(grid, grid, grid))
        self.phi2 = ti.field(ti.f64, shape=(grid, grid, grid))
        self.grad = ti.Vector.field(3, ti.f64, shape=n)

    @ti.kernel
    def deposit(self):
        for I in ti.grouped(self.rho):
            self.rho[I] = 0.0
        for p in range(self.n):
            c = ti.cast(self.pos[p] / self.box * self.grid, ti.i32)
            c = ti.min(ti.max(c, 0), self.grid - 1)
            self.idx[p] = c
            ti.atomic_add(self.rho[c[0], c[1], c[2]], self.mass[p])

    @ti.kernel
    def demean(self) -> ti.f64:
        s = 0.0
        for I in ti.grouped(self.rho):
            s += self.rho[I]
        m = s / (self.grid ** 3)
        for I in ti.grouped(self.rho):
            self.rho[I] -= m
        return m

    @ti.kernel
    def jacobi(self):
        g = self.grid
        src = self.h * self.h / 6.0
        for i, j, k in self.phi:
            nb = (self.phi[(i + 1) % g, j, k] + self.phi[(i - 1 + g) % g, j, k] +
                  self.phi[i, (j + 1) % g, k] + self.phi[i, (j - 1 + g) % g, k] +
                  self.phi[i, j, (k + 1) % g] + self.phi[i, j, (k - 1 + g) % g])
            self.phi2[i, j, k] = nb / 6.0 - self.rho[i, j, k] * src
        for I in ti.grouped(self.phi):
            self.phi[I] = self.phi2[I]

    @ti.kernel
    def gather_move(self, step: ti.f64):
        g = self.grid
        inv = 1.0 / (2.0 * self.h)
        for p in range(self.n):
            c = self.idx[p]
            i, j, k = c[0], c[1], c[2]
            gx = (self.phi[(i + 1) % g, j, k] - self.phi[(i - 1 + g) % g, j, k]) * inv
            gy = (self.phi[i, (j + 1) % g, k] - self.phi[i, (j - 1 + g) % g, k]) * inv
            gz = (self.phi[i, j, (k + 1) % g] - self.phi[i, j, (k - 1 + g) % g]) * inv
            np_ = self.pos[p] - step * ti.Vector([gx, gy, gz])
            self.pos[p] = np_ - ti.floor(np_ / self.box) * self.box

    def tick(self, relax_iters, step):
        self.deposit()
        self.demean()
        for _ in range(relax_iters):
            self.jacobi()
        self.gather_move(step)


def check(grid=32, relax_iters=40, step=0.5, box=100.0, ticks=8):
    """Equivalence: run both the numpy oracle and the CUDA twin from the same
    injected state for a few ticks; assert positions agree on real hardware."""
    particles = load_particles()
    n = len(particles)
    mass = np.array([p["mass"] for p in particles], dtype=np.float64)
    pos0 = oracle.inject(n, box)
    h = box / grid

    # numpy oracle
    pos_c = pos0.copy()
    phi_c = np.zeros((grid, grid, grid))
    for _ in range(ticks):
        rho, idx = oracle.deposit(pos_c, mass, grid, box)
        rho = rho - rho.mean()
        phi_c = oracle.relax(phi_c, rho, relax_iters, h)
        gc = oracle.gradient_at(phi_c, idx, box, grid)
        pos_c = (pos_c - step * gc) % box

    # CUDA twin
    eng = Engine(n, grid, box, arch=ti.cuda)
    eng.pos.from_numpy(pos0)
    eng.mass.from_numpy(mass)
    eng.phi.fill(0.0)
    t0 = time.time()
    for _ in range(ticks):
        eng.tick(relax_iters, step)
    ti.sync()
    gpu_s = time.time() - t0
    pos_g = eng.pos.to_numpy()

    # periodic-aware position difference
    d = np.abs(pos_g - pos_c)
    d = np.minimum(d, box - d)
    max_pos = float(d.max())
    rep = {"artifact": "field-engine-kernel-v0-equivalence", "n_particles": n,
           "grid": grid, "ticks": ticks, "arch": "cuda (GTX 1070)",
           "max_position_diff": max_pos,
           "gpu_seconds": round(gpu_s, 2),
           "gpu_ms_per_tick": round(1000 * gpu_s / ticks, 1),
           "equivalent": max_pos < 1e-6}
    print(json.dumps(rep, indent=1))
    return 0 if rep["equivalent"] else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--grid", type=int, default=32)
    ap.add_argument("--ticks", type=int, default=8)
    a = ap.parse_args()
    if a.check:
        return check(grid=a.grid, ticks=a.ticks)
    print("use --check to run the CPU==GPU equivalence harness")
    return 0


if __name__ == "__main__":
    sys.exit(main())
