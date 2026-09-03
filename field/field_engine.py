#!/usr/bin/env python3
"""field_engine — the one engine: general pull + configuration, in ONE field.

P (2026-09-02): "The total mass is a general pull with specific configuration
of mass forming identity." Like real matter. And: "the step declarations are
the granularity knobs and data integrity rules"; "minting == dedupe: one entry
covers the range"; "position IS identity"; "the engine does it or it does not
happen" — no outside routine reads identity, compares particles across the
population, or defines 'merged'.

WHAT THE FIELD IS. Space is a periodic box on a grid; the medium is undefined
nibbles (docs/architecture.md). A particle is a configuration of defined
nibbles at the DECLARED grain (field_engine_load_v0.load_at_declared_grain:
P's ladder Unicode -> table -> endpoint; the endpoint is the finest declared
step, its configuration is the hex digits of the value it carries). Each
nibble weighs value+1 (P's law). The field carries, per cell, the amount of
each nibble value in each slot: C[slot, value, cell]. That is the ONE deposit.
Total mass density rho = sum over (slot, value) of C — so the general pull is
the same Poisson-relaxed field field_engine_v0 already validated; it falls out
by linearity, nothing is added to it.

THE ONE OPERATION, per tick (a kernel launch = the propagation delay):
  deposit    every live particle's configured mass onto the field
  relax      every channel by the same local Jacobi diffusion (single-point
             balancing); the total potential phi is the channel sum
  read       at the particle's own cell only:
               general pull   -grad phi              (total mass, validated)
               identity pull  -grad sum_s Phi[s, v_p,s]  (the diffused field of
                               matter that matches p slot-wise: identical
                               configurations present no differential, so the
                               field draws them together)
               exclusion      -grad M_p * h^2/6      (the UN-diffused local
                               source of matter that does NOT match p: unlike
                               matter cannot stack; same Poisson-source units,
                               no coefficient)
  move       down the total differential (one step, the v0 discretisation)
  condense   two live particles in the same cell (full adjacency) with ZERO
             amount-differential (identical slot weights) are one entry: the
             later index folds into the resident (min index), masses add
             linearly ("one entry covers the range"), the pool shrinks. This
             is the merge = the dedup = the mint, as an engine event, not a
             threshold anybody picked.

MONITOR: observation only — the live-entry count and the occupied-cell count,
both read straight off engine state, no identity labels, no rates.

CONTROLS (for executed review — Silas's gate): --no-identity-pull,
--no-exclusion, --no-absorb each remove one coupling. With all three off the
engine must reproduce field_engine_v0 (the substrate) exactly; --check asserts
that, plus numpy-oracle == CUDA-kernel with everything on (oracle-first,
docs/architecture.md "Engine substrate").

Deterministic end to end: integer masses, integer atomics, f64 sums of
integers (exact, order-independent), no rng. Two runs are byte-identical.

Run:   python3 field/field_engine.py --ticks 300            (CUDA)
       python3 field/field_engine.py --check                (equivalence)
"""
import argparse
import json
import os
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from field_engine_load_v0 import load_at_declared_grain      # noqa: E402
import field_engine_v0 as v0                                 # noqa: E402

NV = 16  # nibble values


# ----------------------------------------------------------------------------
# The pool: identity bookkeeping lives on the CPU (architecture.md: "identity
# lives on the CPU; the GPU knows position"). Nothing here is read by the
# physics except the slot weights each particle deposits.
# ----------------------------------------------------------------------------
class Pool:
    def __init__(self, particles):
        n = len(particles)
        S = max(len(p["config"]) for p in particles)
        self.n, self.S = n, S
        self.slot_val = np.full((n, S), -1, dtype=np.int32)   # -1 = undefined
        self.slot_w = np.zeros((n, S), dtype=np.int32)         # value+1, 0 if undefined
        for i, p in enumerate(particles):
            for s, v in enumerate(p["config"]):
                self.slot_val[i, s] = v
                self.slot_w[i, s] = v + 1
        self.base_mass = self.slot_w.sum(1).astype(np.int64)
        self.count = np.ones(n, dtype=np.int64)     # copies folded into this entry
        self.alive = np.ones(n, dtype=np.bool_)
        self.parent = np.arange(n, dtype=np.int64)

    def load_state(self, st):
        self.alive = st["alive"].astype(np.bool_)
        self.count = st["count"].astype(np.int64)
        self.parent = st["parent"].astype(np.int64)


def cells_of(pos, box, grid):
    return np.clip((pos / box * grid).astype(np.int64), 0, grid - 1)


def occupied_cells(idx, alive, grid):
    """Positional read: how many distinct cells hold a live entry. No identity."""
    i = idx[alive]
    flat = (i[:, 0] * grid + i[:, 1]) * grid + i[:, 2]
    return int(np.unique(flat).size)


# ----------------------------------------------------------------------------
# CPU oracle (numpy) — the deterministic reference the kernel must match.
# ----------------------------------------------------------------------------
def relax_channels(Phi, src, iters, f):
    """Jacobi on every channel at once (last three axes are the cells)."""
    s6 = src * f
    for _ in range(iters):
        nb = (np.roll(Phi, 1, -3) + np.roll(Phi, -1, -3) +
              np.roll(Phi, 1, -2) + np.roll(Phi, -1, -2) +
              np.roll(Phi, 1, -1) + np.roll(Phi, -1, -1))
        Phi = nb / 6.0 - s6
    return Phi


class Oracle:
    def __init__(self, pool, grid, box, relax_iters, step, pull, excl, absorb):
        self.pool, self.grid, self.box = pool, grid, box
        self.iters, self.step = relax_iters, step
        self.pull, self.excl, self.absorb = pull, excl, absorb
        self.h = box / grid
        g, S = grid, pool.S
        self.phi = np.zeros((g, g, g), dtype=np.float64)
        self.Phi = np.zeros((S, NV, g, g, g), dtype=np.float64) if pull else None

    def tick(self, pos):
        pool, g, h, S = self.pool, self.grid, self.h, self.pool.S
        f = h * h / 6.0
        inv = 1.0 / (2.0 * h)
        ai = np.nonzero(pool.alive)[0]
        idx = cells_of(pos, self.box, g)
        i, j, k = idx[:, 0], idx[:, 1], idx[:, 2]

        # deposit: the configured mass, per (slot, value) channel
        C = np.zeros((S, NV, g, g, g), dtype=np.float64)
        for s in range(S):
            d = ai[pool.slot_val[ai, s] >= 0]
            np.add.at(C[s], (pool.slot_val[d, s], i[d], j[d], k[d]),
                      (pool.slot_w[d, s] * pool.count[d]).astype(np.float64))
        rho = C.sum(axis=(0, 1))

        # relax: general pull from the total; identity pull from every channel
        if self.pull:
            self.Phi = relax_channels(
                self.Phi, C - C.mean(axis=(2, 3, 4), keepdims=True), self.iters, f)
            phi = self.Phi.sum(axis=(0, 1))
        else:
            phi = v0.relax(self.phi, rho - rho.mean(), self.iters, h)
            self.phi = phi

        # read the local differential at each particle's own cell
        F = np.zeros((pool.n, 3), dtype=np.float64)
        for a in range(3):
            d_phi = np.roll(phi, -1, a) - np.roll(phi, 1, a)
            F[:, a] = d_phi[i, j, k] * inv
            if self.pull:
                dP = np.roll(self.Phi, -1, a + 2) - np.roll(self.Phi, 1, a + 2)
                for s in range(S):
                    d = ai[pool.slot_val[ai, s] >= 0]
                    F[d, a] += dP[s, pool.slot_val[d, s], i[d], j[d], k[d]] * inv
            if self.excl:
                d_rho = np.roll(rho, -1, a) - np.roll(rho, 1, a)
                dC = np.roll(C, -1, a + 2) - np.roll(C, 1, a + 2)
                m = d_rho[i, j, k].copy()
                for s in range(S):
                    d = ai[pool.slot_val[ai, s] >= 0]
                    m[d] -= dC[s, pool.slot_val[d, s], i[d], j[d], k[d]]
                F[:, a] += m * f * inv

        pos = pos.copy()
        pos[ai] = (pos[ai] - self.step * F[ai]) % self.box
        n_abs = self.condense(pos) if self.absorb else 0
        return pos, n_abs

    def condense(self, pos):
        pool, g = self.pool, self.grid
        ai = np.nonzero(pool.alive)[0]
        idx = cells_of(pos[ai], self.box, g)
        flat = (idx[:, 0] * g + idx[:, 1]) * g + idx[:, 2]
        resident = np.full(g ** 3, pool.n, dtype=np.int64)
        np.minimum.at(resident, flat, ai)
        r = resident[flat]
        same = (pool.slot_w[ai] == pool.slot_w[r]).all(1)
        ab = (r != ai) & same
        if ab.any():
            src, dst = ai[ab], r[ab]
            np.add.at(pool.count, dst, pool.count[src])
            pool.alive[src] = False
            pool.parent[src] = dst
        return int(ab.sum())


# ----------------------------------------------------------------------------
# Taichi kernel — the GPU twin. Same flow, same arithmetic.
# ----------------------------------------------------------------------------
def make_kernel(pool, grid, box, relax_iters, step, pull, excl, absorb, arch):
    import taichi as ti
    ti.init(arch=getattr(ti, arch), default_fp=ti.f64, random_seed=0,
            offline_cache=False)

    @ti.data_oriented
    class Kernel:
        def __init__(self):
            n, S, g = pool.n, pool.S, grid
            self.pool = pool
            self.n, self.S, self.grid, self.box = n, S, g, float(box)
            self.h = box / g
            self.iters, self.step = relax_iters, float(step)
            self.pull, self.excl, self.absorb = int(pull), int(excl), int(absorb)
            self.pos = ti.Vector.field(3, ti.f64, shape=n)
            self.idx = ti.Vector.field(3, ti.i32, shape=n)
            self.slot_val = ti.field(ti.i32, shape=(n, S))
            self.slot_w = ti.field(ti.i32, shape=(n, S))
            self.count = ti.field(ti.i64, shape=n)
            self.alive = ti.field(ti.i32, shape=n)
            self.parent = ti.field(ti.i64, shape=n)
            self.C = ti.field(ti.f64, shape=(S, NV, g, g, g))
            self.rho = ti.field(ti.f64, shape=(g, g, g))
            self.mean = ti.field(ti.f64, shape=(S, NV))
            self.rho_mean = ti.field(ti.f64, shape=())
            self.phi = ti.field(ti.f64, shape=(g, g, g))
            self.phi2 = ti.field(ti.f64, shape=(g, g, g))
            shp = (S, NV, g, g, g) if pull else (1, 1, 1, 1, 1)
            self.Phi = ti.field(ti.f64, shape=shp)
            self.Phi2 = ti.field(ti.f64, shape=shp)
            self.resident = ti.field(ti.i32, shape=g * g * g)
            # upload the pool
            self.slot_val.from_numpy(pool.slot_val)
            self.slot_w.from_numpy(pool.slot_w)
            self.count.from_numpy(pool.count)
            self.alive.from_numpy(pool.alive.astype(np.int32))
            self.parent.from_numpy(pool.parent)
            self.phi.fill(0.0)
            self.Phi.fill(0.0)

        @ti.kernel
        def deposit(self):
            for I in ti.grouped(self.C):
                self.C[I] = 0.0
            for I in ti.grouped(self.rho):
                self.rho[I] = 0.0
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = ti.cast(self.pos[p] / self.box * self.grid, ti.i32)
                    c = ti.min(ti.max(c, 0), self.grid - 1)
                    self.idx[p] = c
                    for s in range(self.S):
                        v = self.slot_val[p, s]
                        if v >= 0:
                            w = ti.cast(self.slot_w[p, s], ti.f64) * ti.cast(self.count[p], ti.f64)
                            ti.atomic_add(self.C[s, v, c[0], c[1], c[2]], w)
                            ti.atomic_add(self.rho[c[0], c[1], c[2]], w)

        @ti.kernel
        def means(self):
            self.rho_mean[None] = 0.0
            for I in ti.grouped(self.rho):
                self.rho_mean[None] += self.rho[I]
            self.rho_mean[None] /= (self.grid ** 3)
            for s, v in self.mean:
                self.mean[s, v] = 0.0
            for s, v, i, j, k in self.C:
                self.mean[s, v] += self.C[s, v, i, j, k]
            for s, v in self.mean:
                self.mean[s, v] /= (self.grid ** 3)

        @ti.kernel
        def jacobi_total(self):
            g = self.grid
            f = self.h * self.h / 6.0
            for i, j, k in self.phi:
                nb = (self.phi[(i + 1) % g, j, k] + self.phi[(i - 1 + g) % g, j, k] +
                      self.phi[i, (j + 1) % g, k] + self.phi[i, (j - 1 + g) % g, k] +
                      self.phi[i, j, (k + 1) % g] + self.phi[i, j, (k - 1 + g) % g])
                self.phi2[i, j, k] = nb / 6.0 - (self.rho[i, j, k] - self.rho_mean[None]) * f
            for I in ti.grouped(self.phi):
                self.phi[I] = self.phi2[I]

        @ti.kernel
        def jacobi_channels(self):
            g = self.grid
            f = self.h * self.h / 6.0
            for s, v, i, j, k in self.Phi:
                nb = (self.Phi[s, v, (i + 1) % g, j, k] + self.Phi[s, v, (i - 1 + g) % g, j, k] +
                      self.Phi[s, v, i, (j + 1) % g, k] + self.Phi[s, v, i, (j - 1 + g) % g, k] +
                      self.Phi[s, v, i, j, (k + 1) % g] + self.Phi[s, v, i, j, (k - 1 + g) % g])
                self.Phi2[s, v, i, j, k] = nb / 6.0 - (self.C[s, v, i, j, k] - self.mean[s, v]) * f
            for I in ti.grouped(self.Phi):
                self.Phi[I] = self.Phi2[I]

        @ti.kernel
        def sum_channels(self):
            for i, j, k in self.phi:
                acc = 0.0
                for s in range(self.S):
                    for v in range(NV):
                        acc += self.Phi[s, v, i, j, k]
                self.phi[i, j, k] = acc

        @ti.kernel
        def move(self, step: ti.f64, pull: ti.i32, excl: ti.i32):
            g = self.grid
            inv = 1.0 / (2.0 * self.h)
            f = self.h * self.h / 6.0
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = self.idx[p]
                    i, j, k = c[0], c[1], c[2]
                    ip, im = (i + 1) % g, (i - 1 + g) % g
                    jp, jm = (j + 1) % g, (j - 1 + g) % g
                    kp, km = (k + 1) % g, (k - 1 + g) % g
                    gx = (self.phi[ip, j, k] - self.phi[im, j, k]) * inv
                    gy = (self.phi[i, jp, k] - self.phi[i, jm, k]) * inv
                    gz = (self.phi[i, j, kp] - self.phi[i, j, km]) * inv
                    if pull == 1:
                        for s in range(self.S):
                            v = self.slot_val[p, s]
                            if v >= 0:
                                gx += (self.Phi[s, v, ip, j, k] - self.Phi[s, v, im, j, k]) * inv
                                gy += (self.Phi[s, v, i, jp, k] - self.Phi[s, v, i, jm, k]) * inv
                                gz += (self.Phi[s, v, i, j, kp] - self.Phi[s, v, i, j, km]) * inv
                    if excl == 1:
                        mx = self.rho[ip, j, k] - self.rho[im, j, k]
                        my = self.rho[i, jp, k] - self.rho[i, jm, k]
                        mz = self.rho[i, j, kp] - self.rho[i, j, km]
                        for s in range(self.S):
                            v = self.slot_val[p, s]
                            if v >= 0:
                                mx -= self.C[s, v, ip, j, k] - self.C[s, v, im, j, k]
                                my -= self.C[s, v, i, jp, k] - self.C[s, v, i, jm, k]
                                mz -= self.C[s, v, i, j, kp] - self.C[s, v, i, j, km]
                        gx += mx * f * inv
                        gy += my * f * inv
                        gz += mz * f * inv
                    np_ = self.pos[p] - step * ti.Vector([gx, gy, gz])
                    self.pos[p] = np_ - ti.floor(np_ / self.box) * self.box

        @ti.kernel
        def residents(self):
            for c in self.resident:
                self.resident[c] = self.n
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = ti.cast(self.pos[p] / self.box * self.grid, ti.i32)
                    c = ti.min(ti.max(c, 0), self.grid - 1)
                    self.idx[p] = c
                    flat = (c[0] * self.grid + c[1]) * self.grid + c[2]
                    ti.atomic_min(self.resident[flat], p)

        @ti.kernel
        def condense(self) -> ti.i32:
            n_abs = 0
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = self.idx[p]
                    flat = (c[0] * self.grid + c[1]) * self.grid + c[2]
                    r = self.resident[flat]
                    if r != p:
                        same = 1
                        for s in range(self.S):
                            if self.slot_w[p, s] != self.slot_w[r, s]:
                                same = 0
                        if same == 1:
                            self.alive[p] = 0
                            self.parent[p] = r
                            ti.atomic_add(self.count[r], self.count[p])
                            n_abs += 1
            return n_abs

        def tick(self):
            self.deposit()
            self.means()
            if self.pull:
                for _ in range(self.iters):
                    self.jacobi_channels()
                self.sum_channels()
            else:
                for _ in range(self.iters):
                    self.jacobi_total()
            self.move(self.step, self.pull, self.excl)
            n_abs = 0
            if self.absorb:
                self.residents()
                n_abs = int(self.condense())
            return n_abs

        def sync_pool(self):
            """Pull the engine's own state back to the CPU pool (read, not compute)."""
            self.pool.alive = self.alive.to_numpy().astype(np.bool_)
            self.pool.count = self.count.to_numpy().astype(np.int64)
            self.pool.parent = self.parent.to_numpy().astype(np.int64)

    return Kernel()


# ----------------------------------------------------------------------------
# The run + the monitor (observation only) + the mint read-out.
# ----------------------------------------------------------------------------
def resolve(parent, p):
    while parent[p] != p:
        p = parent[p]
    return p


def mint_readout(pool, particles, info):
    """What the engine condensed: each surviving entry with count>1 and the
    range it covers. Read from the engine's own parent map; the identity
    labels are attached only so a reviewer can check them, the engine never
    read them."""
    members = {}
    for p in range(pool.n):
        r = resolve(pool.parent, p)
        members.setdefault(int(r), []).append(p)
    entries = []
    for r, ms in members.items():
        if len(ms) > 1:
            cfg = "".join(f"{v:X}" for v in particles[r]["config"])
            entries.append({
                "entry": r, "config": cfg, "count": int(pool.count[r]),
                "covers": [{"token": ".".join(particles[m]["token"]),
                            "encoding": particles[m]["encoding"],
                            "identity": list(particles[m]["identity"])} for m in ms],
            })
    entries.sort(key=lambda e: e["entry"])
    return {"artifact": "field-engine-mint", "grain": info["grain"],
            "entries_live": int(pool.alive.sum()), "n_particles": pool.n,
            "entries_covering_more_than_one": len(entries), "entries": entries}


def run(a):
    particles, info = load_at_declared_grain(a.grain)
    pool = Pool(particles)
    n, g = pool.n, a.grid
    start, hist = 0, []
    if os.path.exists(a.state):
        st = np.load(a.state, allow_pickle=True)
        pos, start, hist = st["pos"], int(st["tick"]), list(st["hist"])
        assert len(pos) == n
        pool.load_state(st)
    else:
        pos = v0.inject(n, a.box)

    pull, excl, absorb = not a.no_identity_pull, not a.no_exclusion, not a.no_absorb
    if a.arch == "numpy":
        eng = Oracle(pool, g, a.box, a.relax_iters, a.step, pull, excl, absorb)
    else:
        eng = make_kernel(pool, g, a.box, a.relax_iters, a.step, pull, excl, absorb, a.arch)
        eng.pos.from_numpy(pos)

    t0 = time.time()
    still = 0
    for t in range(start, a.ticks):
        if a.arch == "numpy":
            pos, n_abs = eng.tick(pos)
        else:
            n_abs = eng.tick()
        still = still + 1 if n_abs == 0 else 0
        last = (t + 1 == a.ticks) or (a.until_still and still >= a.until_still)
        if (t + 1) % a.chunk == 0 or last:
            if a.arch != "numpy":
                pos = eng.pos.to_numpy()
                eng.sync_pool()
            idx = cells_of(pos, a.box, g)
            obs = {"tick": t + 1, "entries_live": int(pool.alive.sum()),
                   "occupied_cells": occupied_cells(idx, pool.alive, g),
                   "elapsed_s": round(time.time() - t0, 1)}
            hist.append(obs)
            np.savez(a.state, pos=pos, tick=t + 1, alive=pool.alive,
                     count=pool.count, parent=pool.parent,
                     hist=np.array(hist, dtype=object))
            print(json.dumps(obs), flush=True)
        if last:
            break

    mint = mint_readout(pool, particles, info)
    with open(a.mint, "w") as fh:
        json.dump(mint, fh, indent=1)
    rep = {"artifact": "field-engine", "substrate": "field_engine_v0 (validated)",
           "load": info, "arch": a.arch, "grid": g, "box": a.box,
           "relax_iters": a.relax_iters, "step": a.step,
           "couplings": {"identity_pull": pull, "exclusion": excl, "condense": absorb},
           "deterministic": True,
           "entries_first": hist[0]["entries_live"] if hist else None,
           "entries_last": hist[-1]["entries_live"] if hist else None,
           "occupied_cells_last": hist[-1]["occupied_cells"] if hist else None,
           "entries_covering_more_than_one": mint["entries_covering_more_than_one"],
           "history": hist}
    with open(a.report, "w") as fh:
        json.dump(rep, fh, indent=1, default=str)
    print(json.dumps({k: rep[k] for k in ("entries_first", "entries_last",
                                          "occupied_cells_last",
                                          "entries_covering_more_than_one",
                                          "couplings")}, indent=1))
    return 0


# ----------------------------------------------------------------------------
# --check: (A) kernel with every coupling off == field_engine_v0 oracle;
#          (B) numpy oracle == kernel with every coupling on;
#          (C) numpy oracle with every coupling off == field_engine_v0.
# ----------------------------------------------------------------------------
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

    orc = Oracle(pool, g, box, iters, step, False, False, False)
    pos_c = pos0.copy()
    for _ in range(T):
        pos_c, _ = orc.tick(pos_c)
    out["checks"]["C_oracle_off_vs_v0"] = pdiff(pos_c, pos_v0, box)

    if a.arch != "numpy":
        ker = make_kernel(pool, g, box, iters, step, False, False, False, a.arch)
        ker.pos.from_numpy(pos0)
        t0 = time.time()
        for _ in range(T):
            ker.tick()
        out["checks"]["A_kernel_off_vs_v0"] = pdiff(ker.pos.to_numpy(), pos_v0, box)
        out["kernel_off_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)

    # ---- (B): everything on, DECLARED grain, oracle vs kernel ----
    particles, info = load_at_declared_grain(a.grain)
    pool_o, pool_k = Pool(particles), Pool(particles)
    pos0 = v0.inject(pool_o.n, box)
    orc = Oracle(pool_o, g, box, iters, step, True, True, True)
    pos_o = pos0.copy()
    t0 = time.time()
    n_abs_o = 0
    for _ in range(T):
        pos_o, k_ = orc.tick(pos_o)
        n_abs_o += k_
    out["oracle_on_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)
    out["oracle_on_condensed"] = n_abs_o
    if a.arch != "numpy":
        ker = make_kernel(pool_k, g, box, iters, step, True, True, True, a.arch)
        ker.pos.from_numpy(pos0)
        t0 = time.time()
        n_abs_k = 0
        for _ in range(T):
            n_abs_k += ker.tick()
        ker.sync_pool()
        out["kernel_on_ms_per_tick"] = round(1000 * (time.time() - t0) / T, 1)
        out["kernel_on_condensed"] = n_abs_k
        live = pool_o.alive & pool_k.alive
        out["checks"]["B_oracle_on_vs_kernel_on_pos"] = pdiff(pos_o[live], ker.pos.to_numpy()[live], box)
        out["checks"]["B_alive_equal"] = bool((pool_o.alive == pool_k.alive).all())
        out["checks"]["B_count_equal"] = bool((pool_o.count == pool_k.count).all())
        out["checks"]["B_parent_equal"] = bool((pool_o.parent == pool_k.parent).all())

    tol = 1e-6
    ok = all((v < tol) if isinstance(v, float) else v for v in out["checks"].values())
    out["equivalent"] = ok
    print(json.dumps(out, indent=1))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ticks", type=int, default=300)
    ap.add_argument("--grid", type=int, default=48, help="v0 discretisation")
    ap.add_argument("--relax-iters", type=int, default=40, help="v0 discretisation")
    ap.add_argument("--step", type=float, default=0.5, help="v0 discretisation")
    ap.add_argument("--box", type=float, default=100.0, help="v0 discretisation")
    ap.add_argument("--chunk", type=int, default=20)
    ap.add_argument("--grain", default=None,
                    help="endpoint|resolution; default = read from the declarations")
    ap.add_argument("--arch", default="cuda", help="cuda | cpu (taichi) | numpy (oracle)")
    ap.add_argument("--no-identity-pull", action="store_true")
    ap.add_argument("--no-exclusion", action="store_true")
    ap.add_argument("--no-absorb", action="store_true")
    ap.add_argument("--until-still", type=int, default=0,
                    help="stop after this many consecutive ticks with no condensation (0 = fixed ticks)")
    ap.add_argument("--state", default=os.path.join(HERE, "field-engine-state.npz"))
    ap.add_argument("--report", default=os.path.join(HERE, "field-engine-report.json"))
    ap.add_argument("--mint", default=os.path.join(HERE, "field-engine-mint.json"))
    ap.add_argument("--fresh", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--check-grid", type=int, default=32)
    ap.add_argument("--check-ticks", type=int, default=4)
    a = ap.parse_args()
    if a.check:
        return check(a)
    if a.fresh and os.path.exists(a.state):
        os.remove(a.state)
    return run(a)


if __name__ == "__main__":
    sys.exit(main())
