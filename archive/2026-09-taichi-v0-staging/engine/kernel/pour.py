#!/usr/bin/env python3
"""CORPUS POUR (a) — the English lexicon poured into the field engine at scale.

Two-half loop, both halves SHIPPED bytes (no emulation):
  Phase A = taichi twin of binding.tick_v02 (my lane; twin-proved below vs the
            numpy reference, P4-style, f64 tol 1e-10 BEFORE any scale run)
  Phase B = Planner's timestep.phase_b_kernel (his taichi readout twin: det /
            code / compliance / per-particle tau diagnostic) + his
            timestep.bonded_update (numpy, the stateful bond contract) + the
            tau_pair completion rule from his phase_b_bonded (clock ticks on
            pair COMPLETION only — time begins at meaning).

Seed (instance-seed-v0.manifest.json): amp[i] = one-hot(leaf_val[i]) for ALL
16,775,644 nibble leaves; byte instance b owns leaves (2b, 2b+1) = (hi, lo).
Data-forcing seeds bonds: ALL 8,387,822 byte pairs bonded at t=0, tau_pair=0.

Scale posture (measured 2026-08-30): f32 field (~2.6 GB total working set vs
~4.9 GB avail; f64 double-buffer 4.3 GB does not fit). numpy det readout at
16.78M measured 4.9 s/tick -> Phase B runs through the ti kernel instead.
Noise at scale = in-kernel ti.random (host-fed noise only in twin tests, where
bitwise correspondence with the numpy rng matters).

Observables (per sample):
  - bond survival: n_bonded / 8,387,822 (scaffold-dependence-at-density #2)
  - tau_pair break / re-complete cumulative counts (noise's bite on meaning)
  - det drift: mean / p01 / min, frac_locked
  - ticks/sec
  - Planner's trust-but-count pair (expect ZEROS in an all-seeded pour):
      recruit-from-featureless valency events (receiver det < 0.02 at demand)
      code-0 lock crossings (plus the deep variant: from det_prev < 0.1)

Run:  .venv-kernel/bin/python pour.py [--ticks 200] [--twin-only]
      [--save-state] [--t0 0.02] [--sample-every 20]
"""
import argparse
import json
import os
import sys
import time

import numpy as np
import taichi as ti

import timestep as ts     # module import runs ti.init(f64); re-init below
import binding
import diffuse as dv

# re-init before any kernel launch (lazy JIT makes this safe): cap threads —
# Haven is a live NAS (4 cores total), leave one for services.
# POUR_ARCH=cuda (env) targets GPU with no source edit; per-arch twin proofs
# still run first — f32 GPU determinism gets its own tolerance read.
# POUR_FP=f32 runs the WHOLE kernel in f32 (scalars, locals, ti.random) — the
# Pascal unlock (f64 throughput is 1:32 there). f32 twin proofs are
# TOLERANCE-MODE: T1 gated at a measured bound vs the f64 reference; the
# chained T2 is reported (branchy dynamics may flip discretely under fp
# narrowing — trajectory-exactness is an f64 property); F9 and the
# exact-integer instruments (fire-time counters, census, breaks==recompl)
# remain the hard acceptance in every mode.
_ARCH = os.environ.get("POUR_ARCH", "cpu")
_FP = os.environ.get("POUR_FP", "f64")
F = getattr(ti, _FP)
NPF = np.float32 if _FP == "f32" else np.float64
ti.init(arch=getattr(ti, _ARCH), default_fp=F, random_seed=7,
        cpu_max_num_threads=int(os.environ.get("POUR_THREADS", "3")))

# gate chain (all from the shipped halves — single source of truth)
BIAS_CAP = binding.BIAS_CAP
LOCK = binding.LOCK
RAD_TILT = binding.RAD_TILT
V_PULL = binding.V_PULL
DECAY = binding.DECAY
UNIFORM = binding.UNIFORM
D_GATE = dv.D_GATE
KAPPA = dv.KAPPA
FEATURELESS_DET = 0.02   # a receiver this close to uniform has no tilt of its
                         # own — its argmax is the code-0 tie-break, so a
                         # valency pull here recruits NOISE (Planner's count)

NPZ = "instance-seed-v0.npz"


# ── Phase A: taichi twin of binding.tick_v02 ────────────────────────────────
# det/code are INPUTS (Phase B's readout of the current field, per the seam:
# Phase A reads det/code/compliance, writes ONLY amp). Terms are grouped as
# pull_k = sum_j w_j*(onehot_j[k] - amp[p,k]) — algebraically identical to the
# reference's sequential adds; the twin tests bound the regrouping error.
@ti.kernel
def tick_v02_ti(amp: ti.types.ndarray(), out: ti.types.ndarray(),
                det: ti.types.ndarray(), code: ti.types.ndarray(),
                comp: ti.types.ndarray(), bonded_pairs: ti.types.ndarray(),
                given: ti.types.ndarray(),
                weight: ti.types.ndarray(), noise: ti.types.ndarray(),
                counters: ti.types.ndarray(),
                T: F, noise_mode: ti.i32, c0: F, eta: F):
    n = amp.shape[0]
    nn = noise.shape[0]
    for p in range(n):
        detp = det[p]
        codep = code[p]
        locked_p = 1 if detp >= LOCK else 0

        # v0.3 two-class identity wall (seam §P-STEER): sealed cells answer only
        # to their given value; emergent bonded cells are frozen to current code
        sealed_p = 1 if given[p] >= 0 else 0
        restricted = 1 if (sealed_p == 1 or bonded_pairs[p // 2] != 0) else 0
        allowed_c = codep
        if sealed_p == 1:
            allowed_c = given[p]

        node_p = 0
        node_code = 0
        if 0 < p < n - 1:
            if det[p - 1] >= LOCK and det[p + 1] >= LOCK and code[p - 1] == code[p + 1]:
                node_p = 1
                node_code = code[p - 1]

        open_recv = 0
        if locked_p == 0:
            if detp < BIAS_CAP or node_p == 1:
                open_recv = 1

        # radiation coefficients from the two lattice neighbours; a restricted
        # receiver accepts only radiation carrying its allowed code
        wl = 0.0
        cl = 0
        wr = 0.0
        cr = 0
        if open_recv == 1:
            if p - 1 >= 0:
                if det[p - 1] >= D_GATE:
                    if restricted == 0 or code[p - 1] == allowed_c:
                        cl = code[p - 1]
                        wl = RAD_TILT * weight[cl]
            if p + 1 < n:
                if det[p + 1] >= D_GATE:
                    if restricted == 0 or code[p + 1] == allowed_c:
                        cr = code[p + 1]
                        wr = RAD_TILT * weight[cr]

        # node forcing at a restricted cell = revision, masked unless it
        # proposes the allowed code; valency defers to node AUTHORITY, not
        # node presence (v0.3.1 — first wall re-pour measured permanent death
        # of sealed light nibbles at matching foreign nodes)
        node_acts = 0
        if node_p == 1:
            if restricted == 0 or node_code == allowed_c:
                node_acts = 1
        wnode = 0.0
        if node_acts == 1 and locked_p == 0:
            wnode = 2.0

        partner = p ^ 1
        wval = 0.0
        if det[partner] >= LOCK and locked_p == 0 and node_acts == 0:
            wval = V_PULL
            counters[1] += 1
            if detp < FEATURELESS_DET:
                counters[0] += 1   # recruit-from-featureless (trust-but-count)

        supported = 0
        if bonded_pairs[p // 2] != 0 and detp >= D_GATE:
            supported = 1
        if locked_p == 1 and node_p == 1:
            supported = 1
        if sealed_p == 1:
            supported = 1   # v0.3.2 given anchor: bedrock needs no living witness
        wsus = 0.0
        if supported == 1:
            wsus = weight[allowed_c]
        unsupported = 1 if (locked_p == 1 and supported == 0) else 0

        wtot = wl + wr + wnode + wval + wsus
        c_le = c0 * (1.0 - KAPPA * (1.0 - comp[p])) * eta
        for k in range(16):
            pullk = -wtot * amp[p, k]
            if k == cl:
                pullk += wl
            if k == cr:
                pullk += wr
            if k == node_code:
                pullk += wnode
            if k == allowed_c:
                pullk += wval + wsus   # valency = restoration target for sealed
            v = amp[p, k] + c_le * pullk
            if unsupported == 1:
                v += DECAY * (UNIFORM - v)
            if noise_mode == 1:
                v += T * noise[p % nn, k]
            if noise_mode == 2:
                v += T * ti.random(F)
            out[p, k] = ti.max(v, 1e-12)
    for p in range(n):
        s = 0.0
        for k in range(16):
            s += out[p, k]
        inv = 1.0 / s
        for k in range(16):
            out[p, k] = out[p, k] * inv


class _FixedRng:
    """Feed the reference the SAME noise matrix the twin gets."""
    def __init__(self, m):
        self.m = m

    def uniform(self, lo, hi, size):
        return self.m


def engine_state(amp):
    """Allocate the loop's Phase-B buffers for a field of amp's dtype."""
    n = amp.shape[0]
    fdt = amp.dtype
    return {
        "out": np.empty_like(amp),
        "det": np.zeros(n, fdt), "det2": np.zeros(n, fdt),
        "comp": np.zeros(n, fdt), "code": np.zeros(n, np.int32),
        "tau": np.zeros(n, np.int64),
        "counters": np.zeros(4, np.int64),
        "weight": dv.WEIGHT.astype(fdt),   # fp-matched: f64 loads in an f32
                                           # kernel would re-promote (Pascal)
        "dummy_noise": np.zeros((1, 16), fdt),
    }


def engine_run(amp, bonded, ticks, T0=0.02, floor=0.002, lam=0.995,
               noises=None, sample_every=20, on_sample=None, true_code=None,
               given=None, on_state=None, state_every=0):
    """The pour loop: my tick_v02 twin x Planner's Phase B. Mutates amp in
    place conceptually (returns the live buffer). noises: list of host noise
    matrices (twin mode) or None (in-kernel ti.random).
    on_state(tick, arrays): live-array hook every state_every ticks (and at the
    final tick) — arrays are the LIVE buffers (amp/det/code/bonded/tau_pair);
    the callback must copy what it persists. Read-only by contract: no physics
    change, twin proofs unaffected (default off)."""
    n = amp.shape[0]
    assert n % 2 == 0
    if given is None:
        given = np.full(n, -1, dtype=np.int32)   # all emergent
    given = given.astype(np.int32)
    st = engine_state(amp)
    out, det, det2 = st["out"], st["det"], st["det2"]
    comp, code, tau = st["comp"], st["code"], st["tau"]
    counters, weight, dummy = st["counters"], st["weight"], st["dummy_noise"]

    tau_pair = np.zeros(n // 2, np.int64)
    cum_breaks = 0
    cum_completions = 0
    code0_locks = 0        # raw: any lock-crossing landing on code 0 (many
    code0_locks_deep = 0   # cells are LEGIT code 0 — see foreign counts)
    crossings_total = 0
    foreign_locks = 0      # lock-crossing to a code != the seeded identity
    code0_foreign = 0      # Planner's tie-break signature: foreign AND code 0

    # initial Phase-B readout of the seeded field (det_prev=0 => tau diag
    # ticks once on every formed cell at t=0; diagnostic only, not the clock)
    ts.phase_b_kernel(amp, np.zeros(n, amp.dtype), det, code, comp, tau,
                      0.85, 24.0, 0.05)
    T = T0
    t_start = time.time()
    for t in range(ticks):
        if noises is not None:
            noise, mode = np.asarray(noises[t], amp.dtype), 1
        else:
            noise, mode = dummy, 2
        tick_v02_ti(amp, out, det, code, comp, bonded.astype(np.uint8), given,
                    weight, noise, counters, T, mode, dv.C0, dv.ETA)
        amp, out = out, amp                                   # Phase A done
        ts.phase_b_kernel(amp, det, det2, code, comp, tau,    # Phase B readout
                          0.85, 24.0, 0.05)
        det, det2 = det2, det                                 # det2 = previous
        bonded_new = ts.bonded_update(det, bonded)            # his bond contract
        completed = bonded_new & ~bonded
        broken = bonded & ~bonded_new
        tau_pair[completed] += 1        # phase_b_bonded: completion IS the tick
        cum_completions += int(completed.sum())
        cum_breaks += int(broken.sum())
        crossings = (det2 < LOCK) & (det >= LOCK)
        c0mask = crossings & (code == 0)
        crossings_total += int(crossings.sum())
        code0_locks += int(c0mask.sum())
        code0_locks_deep += int((c0mask & (det2 < 0.1)).sum())
        if true_code is not None:
            fmask = crossings & (code != true_code)
            foreign_locks += int(fmask.sum())
            code0_foreign += int((fmask & (code == 0)).sum())
        bonded = bonded_new
        T = max(floor, T * lam)

        if on_sample and ((t + 1) % sample_every == 0 or t == ticks - 1):
            elapsed = time.time() - t_start
            on_sample({
                "tick": t + 1, "T": T,
                "n_bonded": int(bonded.sum()),
                "bond_survival": float(bonded.mean()),
                "cum_breaks": cum_breaks,
                "cum_completions": cum_completions,
                "mean_det": float(det.mean()),
                "p01_det": float(np.percentile(det, 1)),
                "min_det": float(det.min()),
                "frac_locked": float((det >= LOCK).mean()),
                "featureless_recruits": int(counters[0]),
                "valency_demand_events": int(counters[1]),
                "code0_locks": code0_locks,
                "code0_locks_deep": code0_locks_deep,
                "crossings_total": crossings_total,
                "foreign_locks": foreign_locks,
                "code0_foreign_locks": code0_foreign,
                "ticks_per_sec": (t + 1) / elapsed,
            })
        if on_state and state_every and ((t + 1) % state_every == 0
                                         or t == ticks - 1):
            on_state(t + 1, {"amp": amp, "det": det, "code": code,
                             "bonded": bonded, "tau_pair": tau_pair, "T": T})
    return amp, bonded, tau_pair, det, code, {
        "cum_breaks": cum_breaks, "cum_completions": cum_completions,
        "code0_locks": code0_locks, "code0_locks_deep": code0_locks_deep,
        "crossings_total": crossings_total,
        "foreign_locks": foreign_locks,
        "code0_foreign_locks": code0_foreign,
        "featureless_recruits": int(counters[0]),
        "valency_demand_events": int(counters[1]),
    }


# ── twin proofs (run before ANY scale run) ──────────────────────────────────
def _rich_state(n, rng):
    """A field exercising every kernel branch: chaos background, bonded locked
    pairs, a locked-with-chaos-partner (valency), a locked unbonded singleton
    (decay), a standing node (locked X / chaos / locked X)."""
    amp = rng.dirichlet(np.ones(16) * 8.0, size=n)   # soft chaos, det ~ low
    bonded = np.zeros(n // 2, dtype=bool)

    def force(i, c):
        amp[i] = 1e-9
        amp[i, c] = 1.0
        amp[i] /= amp[i].sum()

    force(10, 4); force(11, 1); bonded[5] = True     # bonded pair 'A'
    force(20, 7)                                     # valency: partner 21 chaos
    force(30, 9)                                     # unbonded singleton: decays
    force(40, 12); force(42, 12)                     # standing node flanks 41
    amp[41] = 1.0 / 16.0                             # node target: pure uniform
    force(50, 0)                                     # code-0 locked (edge case)
    # v0.3 wall branches (seam §P-STEER):
    given = np.full(n, -1, dtype=np.int32)
    given[10], given[11] = 4, 1                      # sealed bonded pair
    force(60, 6); force(61, 1); bonded[30] = True    # emergent bonded 'a' pair…
    force(62, 6); force(63, 12); bonded[31] = True   # …with heavy flank (freeze case)
    force(70, 5)                                     # sealed valency: locked mate…
    given[71] = 9                                    # …restores 71 to GIVEN 9 (chaos now)
    given[70] = 5
    force(80, 6); force(82, 6)                       # node vs SEALED unlocked victim
    given[81] = 2                                    # (mask: node code 6 != given 2)
    amp[81] = 1.0 / 16.0
    # v0.3.2 given-anchor branch: sealed space pair, BOTH mates dead, bond
    # broken, locked letter flanks (the v1 wall failure geometry)
    force(101, 12); force(104, 6)                    # flanks (prev-lo, next-hi)
    amp[102] = 1.0 / 16.0                            # dead space hi (given 2)
    amp[103] = 1.0 / 16.0                            # dead space lo (given 0)
    given[102], given[103] = 2, 0                    # bonded[51] stays False
    return amp, bonded, given


def twin_tests():
    ok = lambda m: print(f"  ok: {m}")
    fail = lambda m: (sys.stderr.write(f"FAIL: {m}\n"), sys.exit(1))

    # T1 — single tick, branch-rich field, identical det/code inputs, f64.
    rng = np.random.default_rng(11)
    n = 512
    amp, bonded, given = _rich_state(n, rng)
    det_in = dv.det_of(amp)
    code_in = amp.argmax(axis=1).astype(np.int32)
    comp = dv.f_knee(det_in)
    noise = rng.uniform(0.0, 1.0, size=(n, 16))
    T = 0.02
    ref = binding.tick_v02(amp, comp, T, _FixedRng(noise),
                           np.repeat(bonded, 2), given)
    amp_k = amp.astype(NPF)
    out = np.empty_like(amp_k)
    counters = np.zeros(4, np.int64)
    tick_v02_ti(amp_k, out, det_in.astype(NPF), code_in, comp.astype(NPF),
                bonded.astype(np.uint8), given, dv.WEIGHT.astype(NPF),
                noise.astype(NPF), counters, T, 1, dv.C0, dv.ETA)
    err = np.abs(out.astype(np.float64) - ref).max()
    # f64 = exact-twin gate; f32 = tolerance-mode vs the f64 reference
    # (MEASURED cpu 2026-08-31: 2.0e-07 single-tick; gate leaves ~500x margin
    # to absorb GPU reduction-order variance — re-measure per arch)
    tol = 1e-10 if _FP == "f64" else 1e-4
    if err > tol:
        fail(f"T1: taichi tick_v02 twin diverges (max err {err:.2e} > {tol:.0e} [{_FP}])")
    ok(f"T1 tick_v02 twin vs f64 reference: max err {err:.2e} [{_FP}, gate {tol:.0e}] "
       f"(radiation/node/valency/sustain/decay + sealed/frozen wall branches all present)")

    # T2 — 50-tick chained loop: full pour machinery (my twin + Planner's
    # phase_b_kernel + bonded_update + tau_pair rule) vs the shipped numpy
    # halves composed exactly as binding.run composes them.
    rng = np.random.default_rng(23)
    amp0, bonded0, given0 = _rich_state(n, rng)
    ticks = 50
    noises = [rng.uniform(0.0, 1.0, size=(n, 16)) for _ in range(ticks)]

    ref_amp = amp0.copy()
    ref_b = bonded0.copy()
    ref_tau_pair = np.zeros(n // 2, np.int64)
    T = 0.02
    for t in range(ticks):
        det = dv.det_of(ref_amp)
        compt = dv.f_knee(det)
        ref_amp = binding.tick_v02(ref_amp, compt, T, _FixedRng(noises[t]),
                                   np.repeat(ref_b, 2), given0)
        b_new = ts.bonded_update(dv.det_of(ref_amp), ref_b)
        ref_tau_pair[b_new & ~ref_b] += 1
        ref_b = b_new
        T = max(0.002, T * 0.995)

    got_amp, got_b, got_tau_pair, _, _, _ = engine_run(
        amp0.astype(NPF), bonded0.copy(), ticks, noises=noises,
        sample_every=10**9, given=given0)
    err = np.abs(got_amp.astype(np.float64) - ref_amp).max()
    # f64 = exact; f32 = chained tolerance-mode (fp narrowing COULD flip
    # det-threshold branches over 50 ticks — bond/tau equality stays the hard
    # gate either way; MEASURED cpu 2026-08-31: amp err 1.5e-07, no flips;
    # gate absorbs GPU variance — re-measure per arch)
    tol2 = 1e-9 if _FP == "f64" else 5e-3
    if err > tol2:
        fail(f"T2: chained loop diverges from shipped-halves reference "
             f"(max amp err {err:.2e} > {tol2:.0e} [{_FP}])")
    if not (got_b == ref_b).all():
        fail(f"T2: bond state diverges ({int((got_b != ref_b).sum())} pairs)")
    if not (got_tau_pair == ref_tau_pair).all():
        fail("T2: tau_pair clock diverges")
    ok(f"T2 chained 50-tick loop vs shipped-halves reference: max amp err "
       f"{err:.2e} [{_FP}, gate {tol2:.0e}]; bonded + tau_pair exact; "
       f"{int(ref_b.sum())} bonds held)")

    # F9 — the v1 wall failure, reproduced then healed: a sealed space pair
    # (both-light matter), both mates DEAD, bond BROKEN, letter flanks locked.
    # v0.3.1 left this corpse a corpse; the given anchor must bring it home
    # CLOCKED: re-lock to given, re-complete the bond, tick tau_pair.
    n9 = 32
    amp9 = np.full((n9, 16), 1.0 / 16.0, dtype=np.float64)
    given9 = np.full(n9, -1, dtype=np.int32)
    for b9 in range(n9 // 2):
        for cell, c in ((2 * b9, 6), (2 * b9 + 1, 1)):   # 'a' = (6,1) sealed+locked
            amp9[cell] = 1e-9
            amp9[cell, c] = 1.0
            amp9[cell] /= amp9[cell].sum()
            given9[cell] = c
    bonded9 = np.ones(n9 // 2, dtype=bool)
    amp9[16] = 1.0 / 16.0                     # byte 8 = space (2,0): both dead,
    amp9[17] = 1.0 / 16.0                     # bond broken — the corpse case
    given9[16], given9[17] = 2, 0
    bonded9[8] = False
    _, bonded9f, tau9, det9, code9, _ = engine_run(
        amp9.astype(NPF), bonded9, 300, sample_every=10**9, given=given9,
        true_code=given9)
    if not (bonded9f[8] and code9[16] == 2 and code9[17] == 0
            and det9[16] >= LOCK and det9[17] >= LOCK and tau9[8] >= 1):
        fail(f"F9: given anchor failed to resurrect the sealed space pair "
             f"(bonded={bool(bonded9f[8])}, codes=({int(code9[16])},{int(code9[17])}), "
             f"det=({det9[16]:.3f},{det9[17]:.3f}), tau={int(tau9[8])})")
    ok(f"F9 given-anchor resurrection: dead sealed space pair re-locked to given "
       f"(det {det9[16]:.3f}/{det9[17]:.3f}), bond re-completed, tau_pair={int(tau9[8])} — clocked healing")
    print("TWIN PROOFS PASS — scale run is running the proven bytes.")


# ── the pour ────────────────────────────────────────────────────────────────
def pour(ticks, t0, sample_every, save_state, out_prefix="pour-v0"):
    z = np.load(NPZ)
    leaf_val = z["leaf_val"]
    n = len(leaf_val)
    assert n % 2 == 0
    print(f"pour: {n:,} leaves ({n // 2:,} byte pairs), f32, {ticks} ticks, "
          f"T0={t0}", flush=True)

    # amp_seed_law: one-hot(leaf_val); all pairs bonded (data-forcing)
    amp = np.zeros((n, 16), dtype=np.float32)
    amp[np.arange(n), leaf_val] = 1.0
    bonded = np.ones(n // 2, dtype=bool)

    samples = []

    def on_sample(s):
        samples.append(s)
        print(f"  t={s['tick']:>4} T={s['T']:.4f} "
              f"bonds={s['bond_survival'] * 100:.3f}% "
              f"breaks={s['cum_breaks']} recompl={s['cum_completions']} "
              f"det μ={s['mean_det']:.4f} p01={s['p01_det']:.4f} "
              f"min={s['min_det']:.3f} "
              f"cnt=({s['featureless_recruits']},{s['code0_foreign_locks']}) "
              f"{s['ticks_per_sec']:.2f} t/s", flush=True)

    # v0.3: every poured cell is GIVEN — the wall is structural for this run
    amp, bonded, tau_pair, det, code, totals = engine_run(
        amp, bonded, ticks, T0=t0, sample_every=sample_every,
        on_sample=on_sample, true_code=leaf_val.astype(np.int32),
        given=leaf_val.astype(np.int32))

    # two-axis acceptance (seam §P-STEER + planner co-sign 2026-08-31 15:09Z):
    # identity AND existence — v0.2 bought existence WITH identity (the drift);
    # v0.3.0 bought identity WITH existence (the deaths); v0.3.1 must pay for
    # neither with the other.
    drift = int((code != leaf_val).sum())
    perm_death = int((~bonded).sum())
    balanced = totals["cum_breaks"] == totals["cum_completions"]
    print(f"ACCEPTANCE: IDENTITY given-drift = {drift} "
          f"({'PASS — structural zero' if drift == 0 else 'FAIL — WALL BREACHED'})", flush=True)
    print(f"ACCEPTANCE: EXISTENCE permanent-death = {perm_death}, "
          f"breaks {totals['cum_breaks']} vs re-completions {totals['cum_completions']} "
          f"({'PASS — every break healed, clocked' if perm_death == 0 and balanced else 'FAIL — EXISTENCE LOST'})",
          flush=True)
    report = {
        "artifact": "corpus-pour-a-v0.3-wall",
        "seed": NPZ,
        "n_leaves": n, "n_pairs": n // 2, "ticks": ticks, "T0": t0,
        "dtype": "float32",
        "phase_a": "tick_v02_ti (twin-proved vs binding.tick_v02, T1/T2)",
        "phase_b": "ts.phase_b_kernel + ts.bonded_update + tau_pair rule",
        "final": {
            "bond_survival": float(bonded.mean()),
            "n_bonded": int(bonded.sum()),
            "tau_pair_max": int(tau_pair.max()),
            "tau_pair_reticked_pairs": int((tau_pair > 0).sum()),
            "mean_det": float(det.mean()),
            "p01_det": float(np.percentile(det, 1)),
            "min_det": float(det.min()),
            "frac_locked": float((det >= LOCK).mean()),
            "code_drift_cells": int((code != leaf_val).sum()),
            **totals,
        },
        "samples": samples,
    }
    rp = f"{out_prefix}-report.json"
    with open(rp, "w") as f:
        json.dump(report, f, indent=1)
    print(json.dumps(report["final"], indent=1))
    if save_state:
        np.savez_compressed(f"{out_prefix}-state.npz", det=det, code=code,
                            bonded=bonded, tau_pair=tau_pair)
        print(f"state saved: {out_prefix}-state.npz")
    print(f"POUR COMPLETE — report: {rp}", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--t0", type=float, default=0.02)
    ap.add_argument("--sample-every", type=int, default=20)
    ap.add_argument("--twin-only", action="store_true")
    ap.add_argument("--save-state", action="store_true")
    ap.add_argument("--out-prefix", default="pour-v0")
    args = ap.parse_args()

    twin_tests()
    if args.twin_only:
        return
    pour(args.ticks, args.t0, args.sample_every, args.save_state, args.out_prefix)


if __name__ == "__main__":
    main()
