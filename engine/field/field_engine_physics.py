#!/usr/bin/env python3
"""Field-engine physics and internal state.

This module contains the actual field dynamics and the state those dynamics own:
particle configuration/state, deposition, field relaxation, force reads, movement,
resident selection, condensation, exclusion, the deterministic NumPy oracle, and
the Taichi accelerated twin.

It deliberately contains no run lifecycle, checkpoint policy, reporting, CLI,
or analyst-facing controls. Those belong to the engine harness.
"""
import numpy as np

import field_engine_v0 as v0

NV = 16  # nibble values


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
def nbr(a, axis, d, periodic):
    """Array whose [i] is a[i+d] along `axis`; zero outside an open box."""
    if periodic:
        return np.roll(a, -d, axis)
    out = np.zeros_like(a)
    n = a.shape[axis]
    src = [slice(None)] * a.ndim
    dst = [slice(None)] * a.ndim
    if d > 0:
        src[axis], dst[axis] = slice(d, n), slice(0, n - d)
    else:
        src[axis], dst[axis] = slice(0, n + d), slice(-d, n)
    out[tuple(dst)] = a[tuple(src)]
    return out


def relax_channels(Phi, src, iters, f, periodic):
    """Jacobi on every channel at once (last three axes are the cells)."""
    s6 = src * f
    for _ in range(iters):
        nb = (nbr(Phi, -3, 1, periodic) + nbr(Phi, -3, -1, periodic) +
              nbr(Phi, -2, 1, periodic) + nbr(Phi, -2, -1, periodic) +
              nbr(Phi, -1, 1, periodic) + nbr(Phi, -1, -1, periodic))
        Phi = nb / 6.0 - s6
    return Phi


class Oracle:
    def __init__(self, pool, grid, box, relax_iters, step, pull, excl, absorb,
                 periodic=False, cap=True):
        self.pool, self.grid, self.box = pool, grid, box
        self.iters, self.step = relax_iters, step
        self.pull, self.excl, self.absorb = pull, excl, absorb
        self.periodic, self.cap = periodic, cap
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

        # relax: general pull from the total; identity pull from every channel.
        # Periodic (v0 regression only): the source is demeaned. Open: the full
        # mass is the source, the field is zero outside the box.
        per = self.periodic
        if self.pull:
            src = C - C.mean(axis=(2, 3, 4), keepdims=True) if per else C
            self.Phi = relax_channels(self.Phi, src, self.iters, f, per)
            phi = self.Phi.sum(axis=(0, 1))
        elif per:
            phi = v0.relax(self.phi, rho - rho.mean(), self.iters, h)
            self.phi = phi
        else:
            phi = relax_channels(self.phi, rho, self.iters, f, per)
            self.phi = phi

        # read the local differential at each particle's own cell
        F = np.zeros((pool.n, 3), dtype=np.float64)
        for a in range(3):
            d_phi = nbr(phi, a, 1, per) - nbr(phi, a, -1, per)
            F[:, a] = d_phi[i, j, k] * inv
            if self.pull:
                dP = nbr(self.Phi, a + 2, 1, per) - nbr(self.Phi, a + 2, -1, per)
                for s in range(S):
                    d = ai[pool.slot_val[ai, s] >= 0]
                    F[d, a] += dP[s, pool.slot_val[d, s], i[d], j[d], k[d]] * inv

        pos = pos.copy()
        delta = -self.step * F[ai]
        if self.cap:                       # at most one cell per tick
            ln = np.sqrt((delta * delta).sum(1))
            over = ln > h
            delta[over] *= (h / ln[over])[:, None]
        moved = pos[ai] + delta
        pos[ai] = moved % self.box if per else np.clip(moved, 0.0, np.nextafter(self.box, 0.0))

        n_abs = 0
        if self.absorb or self.excl:
            res = self.residents(pos)
            if self.absorb:
                n_abs = self.condense(res)
            if self.excl:
                self.exclude(pos, res, rho, C)
        return pos, n_abs

    def residents(self, pos):
        """The heaviest live entry holds the cell (ties: lowest index)."""
        pool, g = self.pool, self.grid
        ai = np.nonzero(pool.alive)[0]
        idx = cells_of(pos[ai], self.box, g)
        flat = (idx[:, 0] * g + idx[:, 1]) * g + idx[:, 2]
        n = pool.n
        key = (pool.base_mass[ai] * pool.count[ai]) * n + (n - 1 - ai)
        best = np.full(g ** 3, -1, dtype=np.int64)
        np.maximum.at(best, flat, key)
        r = n - 1 - (best[flat] % n)
        return ai, idx, r

    def condense(self, res):
        pool = self.pool
        ai, idx, r = res
        same = (pool.slot_w[ai] == pool.slot_w[r]).all(1)
        ab = (r != ai) & same
        if ab.any():
            src, dst = ai[ab], r[ab]
            np.add.at(pool.count, dst, pool.count[src])
            pool.alive[src] = False
            pool.parent[src] = dst
        return int(ab.sum())

    def exclude(self, pos, res, rho, C):
        """Unlike matter on an occupied point shifts one cell toward the least
        foreign matter around it (6-neighbourhood, inside the box)."""
        pool, g, h, S, per = self.pool, self.grid, self.h, self.pool.S, self.periodic
        ai, idx, r = res
        keep = pool.alive[ai] & (r != ai)          # still live, not the resident
        if not keep.any():
            return
        p = ai[keep]
        c = idx[keep]
        best_m = np.full(len(p), np.inf)
        best_d = np.full(len(p), -1, dtype=np.int64)
        for d in range(6):
            a, sgn = d // 2, (1 if d % 2 == 0 else -1)
            nc = c.copy()
            nc[:, a] += sgn
            if per:
                nc[:, a] %= g
                ok = np.ones(len(p), dtype=bool)
            else:
                ok = (nc[:, a] >= 0) & (nc[:, a] < g)
            ncc = np.clip(nc, 0, g - 1)
            m = rho[ncc[:, 0], ncc[:, 1], ncc[:, 2]].copy()
            for s in range(S):
                v = pool.slot_val[p, s]
                has = v >= 0
                m[has] -= C[s, v[has], ncc[has, 0], ncc[has, 1], ncc[has, 2]]
            m[~ok] = np.inf
            better = m < best_m                       # strict: first axis wins ties
            best_m[better] = m[better]
            best_d[better] = d
        mv = best_d >= 0
        for d in range(6):
            a, sgn = d // 2, (1 if d % 2 == 0 else -1)
            sel = mv & (best_d == d)
            pos[p[sel], a] += sgn * h
        if per:
            pos[p] %= self.box
        else:
            np.clip(pos[p], 0.0, np.nextafter(self.box, 0.0), out=pos[p])


# ----------------------------------------------------------------------------
# Taichi kernel — the GPU twin. Same flow, same arithmetic.
# ----------------------------------------------------------------------------
def make_kernel(pool, grid, box, relax_iters, step, pull, excl, absorb, arch,
                periodic=False, cap=True):
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
            self.box_in = float(np.nextafter(box, 0.0))
            self.iters, self.step = relax_iters, float(step)
            self.pull, self.excl, self.absorb = int(pull), int(excl), int(absorb)
            self.periodic = bool(periodic)
            self.cap = bool(cap)
            self.demean = 1.0 if periodic else 0.0
            self.pos = ti.Vector.field(3, ti.f64, shape=n)
            self.idx = ti.Vector.field(3, ti.i32, shape=n)
            self.slot_val = ti.field(ti.i32, shape=(n, S))
            self.slot_w = ti.field(ti.i32, shape=(n, S))
            self.count = ti.field(ti.i64, shape=n)
            self.base_mass = ti.field(ti.i64, shape=n)
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
            self.resident = ti.field(ti.i64, shape=g * g * g)   # key of the holder
            # upload the pool
            self.slot_val.from_numpy(pool.slot_val)
            self.slot_w.from_numpy(pool.slot_w)
            self.count.from_numpy(pool.count)
            self.base_mass.from_numpy(pool.base_mass)
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

        @ti.func
        def at3(self, fld: ti.template(), i, j, k):
            """Cell read with the boundary: wrap (periodic) or zero outside (open)."""
            g = self.grid
            r = 0.0
            if ti.static(self.periodic):
                r = fld[(i + g) % g, (j + g) % g, (k + g) % g]
            else:
                if i >= 0 and i < g and j >= 0 and j < g and k >= 0 and k < g:
                    r = fld[i, j, k]
            return r

        @ti.func
        def at5(self, fld: ti.template(), s, v, i, j, k):
            g = self.grid
            r = 0.0
            if ti.static(self.periodic):
                r = fld[s, v, (i + g) % g, (j + g) % g, (k + g) % g]
            else:
                if i >= 0 and i < g and j >= 0 and j < g and k >= 0 and k < g:
                    r = fld[s, v, i, j, k]
            return r

        @ti.kernel
        def jacobi_total(self):
            f = self.h * self.h / 6.0
            for i, j, k in self.phi:
                nb = (self.at3(self.phi, i + 1, j, k) + self.at3(self.phi, i - 1, j, k) +
                      self.at3(self.phi, i, j + 1, k) + self.at3(self.phi, i, j - 1, k) +
                      self.at3(self.phi, i, j, k + 1) + self.at3(self.phi, i, j, k - 1))
                self.phi2[i, j, k] = nb / 6.0 - (self.rho[i, j, k] - self.rho_mean[None] * self.demean) * f
            for I in ti.grouped(self.phi):
                self.phi[I] = self.phi2[I]

        @ti.kernel
        def jacobi_channels(self):
            f = self.h * self.h / 6.0
            for s, v, i, j, k in self.Phi:
                nb = (self.at5(self.Phi, s, v, i + 1, j, k) + self.at5(self.Phi, s, v, i - 1, j, k) +
                      self.at5(self.Phi, s, v, i, j + 1, k) + self.at5(self.Phi, s, v, i, j - 1, k) +
                      self.at5(self.Phi, s, v, i, j, k + 1) + self.at5(self.Phi, s, v, i, j, k - 1))
                self.Phi2[s, v, i, j, k] = nb / 6.0 - (self.C[s, v, i, j, k] - self.mean[s, v] * self.demean) * f
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
        def move(self, step: ti.f64, pull: ti.i32):
            inv = 1.0 / (2.0 * self.h)
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = self.idx[p]
                    i, j, k = c[0], c[1], c[2]
                    gx = (self.at3(self.phi, i + 1, j, k) - self.at3(self.phi, i - 1, j, k)) * inv
                    gy = (self.at3(self.phi, i, j + 1, k) - self.at3(self.phi, i, j - 1, k)) * inv
                    gz = (self.at3(self.phi, i, j, k + 1) - self.at3(self.phi, i, j, k - 1)) * inv
                    if pull == 1:
                        for s in range(self.S):
                            v = self.slot_val[p, s]
                            if v >= 0:
                                gx += (self.at5(self.Phi, s, v, i + 1, j, k) - self.at5(self.Phi, s, v, i - 1, j, k)) * inv
                                gy += (self.at5(self.Phi, s, v, i, j + 1, k) - self.at5(self.Phi, s, v, i, j - 1, k)) * inv
                                gz += (self.at5(self.Phi, s, v, i, j, k + 1) - self.at5(self.Phi, s, v, i, j, k - 1)) * inv
                    delta = -step * ti.Vector([gx, gy, gz])
                    if ti.static(self.cap):
                        ln = delta.norm()
                        if ln > self.h:
                            delta = delta * (self.h / ln)
                    np_ = self.pos[p] + delta
                    if ti.static(self.periodic):
                        self.pos[p] = np_ - ti.floor(np_ / self.box) * self.box
                    else:
                        self.pos[p] = ti.min(ti.max(np_, 0.0), self.box_in)

        @ti.kernel
        def residents(self):
            n = ti.cast(self.n, ti.i64)
            for c in self.resident:
                self.resident[c] = -1
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = ti.cast(self.pos[p] / self.box * self.grid, ti.i32)
                    c = ti.min(ti.max(c, 0), self.grid - 1)
                    self.idx[p] = c
                    flat = (c[0] * self.grid + c[1]) * self.grid + c[2]
                    mass = ti.cast(self.base_mass[p], ti.i64) * self.count[p]
                    key = mass * n + (n - 1 - ti.cast(p, ti.i64))
                    ti.atomic_max(self.resident[flat], key)

        @ti.func
        def holder(self, flat):
            n = ti.cast(self.n, ti.i64)
            return ti.cast(n - 1 - (self.resident[flat] % n), ti.i32)

        @ti.kernel
        def condense(self) -> ti.i32:
            n_abs = 0
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = self.idx[p]
                    flat = (c[0] * self.grid + c[1]) * self.grid + c[2]
                    r = self.holder(flat)
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

        @ti.kernel
        def exclude(self):
            g = self.grid
            for p in range(self.n):
                if self.alive[p] == 1:
                    c = self.idx[p]
                    flat = (c[0] * g + c[1]) * g + c[2]
                    if self.holder(flat) != p:
                        best_m = 1e300
                        best_d = -1
                        for d in ti.static(range(6)):
                            a = d // 2
                            sgn = 1 if d % 2 == 0 else -1
                            nc = c
                            nc[a] += sgn
                            ok = True
                            if ti.static(self.periodic):
                                nc[a] = (nc[a] + g) % g
                            else:
                                ok = nc[a] >= 0 and nc[a] < g
                            if ok:
                                m = self.rho[nc[0], nc[1], nc[2]]
                                for s in range(self.S):
                                    v = self.slot_val[p, s]
                                    if v >= 0:
                                        m -= self.C[s, v, nc[0], nc[1], nc[2]]
                                if m < best_m:
                                    best_m = m
                                    best_d = d
                        if best_d >= 0:
                            a = best_d // 2
                            sgn = 1.0 if best_d % 2 == 0 else -1.0
                            np_ = self.pos[p]
                            np_[a] += sgn * self.h
                            if ti.static(self.periodic):
                                self.pos[p] = np_ - ti.floor(np_ / self.box) * self.box
                            else:
                                self.pos[p] = ti.min(ti.max(np_, 0.0), self.box_in)

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
            self.move(self.step, self.pull)
            n_abs = 0
            if self.absorb or self.excl:
                self.residents()
                if self.absorb:
                    n_abs = int(self.condense())
                if self.excl:
                    self.exclude()
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
