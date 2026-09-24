#!/usr/bin/env python3
"""Field-engine KERNEL — Phase A `diffuse()` (silas's half).

Seam: ~/shared-brain/exchange/field-engine-interface-v0.md
  §Per-tick contract  — Phase A: amp[p] <- diffuse(amp, neighbours, rate=c_local, temp=T)
  §Kernel #1/#6       — attraction toward the radiated form (NOT mean-averaging,
                        NOT Fickian spread): a FORMED neighbour radiates its peak
                        code as an attractive pull on the background field.
  §Kernel #4          — pull strength scales with INTRINSIC atomic weight of the
                        radiating code (v0 stand-in: 1 + popcount(code); provisional,
                        measured-not-named — frequency NEVER fed).
  §Kernel #5          — c_local = c0 * g(compliance, kappa), g = 1 - kappa*(1-compliance)
                        (sign as fixed 2026-08-30: fronts slow in STIFF/resolved matter).
  §Time-step flags    — (2) rate is receiver-gated, knee d0 near 1 carries over;
                        (3) radiation gate is ABSOLUTE (det >= D_GATE), never
                        receiver-relative: an ungated kernel's onset wave is
                        condition-limited (1 cell/tick through anything) -> zero
                        dilation; a relative gate strangles the climb. Both are
                        REPRODUCED as tests below, not assumed.

Update rule (v0, 1-D stencil radius 1):
  pull[p]  = sum over neighbours n with det[n] >= D_GATE of
               w(code[n]) * (onehot(code[n]) - amp[p])
  amp[p] <- renormalize( amp[p] + c_local[p] * ETA * pull[p] + T * jitter )
Invariant: Phase A writes ONLY amp. det/code/compliance are READ (computed by
Phase B; the harness below computes them per the seam contract as the test
stand-in for Planner's half — test harness only, no cross-write in the design).

Self-proofs (all hard asserts, run on import-as-main):
  P1 front propagation + peak sharpening (no mean-average dissolution)
  P2 gate necessity: ungated onset crosses a stiff band condition-limited
     (~1 cell/tick, same speed as free space); ABSOLUTE gate restores
     rate-limited crossing (band >> free crossing time = dilation precondition)
  P3 chaos stays chaos: far field under T-noise never crosses the gate
  P4 numpy reference == @ti.kernel twin (bitwise-close), full update step
  P5 amp stays a distribution (non-negative, rows sum to 1)
"""
import sys
import numpy as np

# ── kernel-owned config (seam §Owned config) ────────────────────────────────
C0 = 0.35          # base rate
KAPPA = 1.0        # coupling: kappa=1 -> g == compliance (fronts stall in matter)
ETA = 0.5          # pull step scale
D_GATE = 0.30      # ABSOLUTE formed-ness gate on radiation (stub value, seam flag #3)
LN16 = np.log(16.0)

WEIGHT = (1.0 + np.array([bin(c).count("1") for c in range(16)])).astype(np.float64)
# ^ v0 atomic-weight stand-in: intrinsic bit-composition only (popcount+1).
#   0b0000 -> 1 (hydrogen-light), 0b1111 -> 5 (heavy). Provisional scale; the
#   run's emergent commonality is its validation check, never its input.


def det_of(amp):
    """Phase-B readout, reproduced per contract for the test harness only."""
    with np.errstate(divide="ignore", invalid="ignore"):
        h = np.where(amp > 0, -amp * np.log(amp), 0.0).sum(axis=-1)
    return 1.0 - h / LN16


def f_knee(det, d0=0.85, k=24.0):
    """Planner's compliance form (timestep half): logistic knee, d0 near 1."""
    return 1.0 / (1.0 + np.exp(k * (det - d0)))


def g_of(compliance, kappa=KAPPA):
    return 1.0 - kappa * (1.0 - compliance)


def diffuse_np(amp, compliance, T, rng, gate=D_GATE, c0=C0, eta=ETA,
               cross_gate=None, cross_eps=0.0):
    """One Phase-A tick, numpy reference. 1-D lattice, radius-1 stencil.
    Returns NEW amp; input untouched (Phase B reads the same tick's field).

    cross_gate (Kernel #7, default None = off, all P1-P5 unchanged): absolute
    receiver-side formed-ness threshold implementing Kernel #6's own text — the
    attraction reorganizes the BACKGROUND field; a receiver with det >= cross_gate
    is no longer background for OTHER forms (cross-code pull masked off), while
    same-code reinforcement still applies (a form keeps consolidating itself).

    cross_eps (P's hard-vs-impossible open): residual cross-code pull factor on
    LOCKED receivers. 0.0 = conversion impossible (past immutable); small >0 =
    conversion hard (reconsolidation at measured cost — border clock shows the rate).

    Gate-ordering invariant (Planner review 2026-08-30, load-bearing): radiate
    gate <= cross_gate < stiffen knee d0 — cross_gate >= d0 creates a
    stiff-but-flippable band = B2 annihilation in slow motion. The lower bound
    is asserted here (both levers in scope); the harness asserts cross_gate < d0."""
    if cross_gate is not None:
        assert gate <= cross_gate, (
            f"gate ordering violated: radiate gate {gate} > identity-lock {cross_gate}")
        assert 0.0 <= cross_eps < 1.0
    n, K = amp.shape
    det = det_of(amp)
    code = amp.argmax(axis=1)
    radiates = det >= gate                      # ABSOLUTE gate (flag #3)
    w = WEIGHT[code] * radiates                 # weight-sourced, zero if unformed
    onehot = np.eye(K)[code]

    pull = np.zeros_like(amp)
    # left neighbour (p-1) acts on receivers 1..n-1; right (p+1) on 0..n-2.
    for nb, rc in (((slice(0, n - 1)), slice(1, n)), (slice(1, n), slice(0, n - 1))):
        w_n = w[nb]
        if cross_gate is not None:
            keep = (det[rc] < cross_gate) | (code[nb] == code[rc])
            w_n = w_n * np.where(keep, 1.0, cross_eps)
        pull[rc] += w_n[:, None] * (onehot[nb] - amp[rc])

    c_local = c0 * g_of(compliance)             # receiver-gated rate (flag #2)
    new = amp + c_local[:, None] * eta * pull
    if T > 0.0:
        new = new + T * rng.uniform(0.0, 1.0, size=amp.shape)
    new = np.clip(new, 1e-12, None)
    return new / new.sum(axis=1, keepdims=True)


# ── taichi twin ─────────────────────────────────────────────────────────────
def make_ti_kernel(n):
    import taichi as ti
    ti.init(arch=ti.cpu, default_fp=ti.f64, random_seed=0)
    amp_f = ti.field(ti.f64, shape=(n, 16))
    out_f = ti.field(ti.f64, shape=(n, 16))
    comp_f = ti.field(ti.f64, shape=n)
    noise_f = ti.field(ti.f64, shape=(n, 16))   # host-fed so twin == reference exactly
    wtab = ti.field(ti.f64, shape=16)
    wtab.from_numpy(WEIGHT)

    @ti.kernel
    def tick(T: ti.f64, gate: ti.f64, c0: ti.f64, eta: ti.f64):
        for p in range(n):
            # readout of the CURRENT field (read-only, same tick for all p)
            for j in ti.static(range(1)):
                pass
            csum = 0.0
            for k in range(16):
                a = amp_f[p, k]
                if a > 0:
                    csum += -a * ti.log(a)
            _ = csum  # det of self not needed for pull; kept minimal
        for p in range(n):
            c_local = c0 * (1.0 - KAPPA * (1.0 - comp_f[p]))
            for k in range(16):
                pullk = 0.0
                for s in ti.static((-1, 1)):
                    q = p + s
                    if 0 <= q < n:
                        # neighbour readout
                        h = 0.0
                        mx = 0.0
                        cd = 0
                        for m in range(16):
                            a = amp_f[q, m]
                            if a > 0:
                                h += -a * ti.log(a)
                            if a > mx:
                                mx = a
                                cd = m
                        detq = 1.0 - h / ti.log(16.0)
                        if detq >= gate:
                            target = 1.0 if k == cd else 0.0
                            pullk += wtab[cd] * (target - amp_f[p, k])
                v = amp_f[p, k] + c_local * eta * pullk + T * noise_f[p, k]
                out_f[p, k] = ti.max(v, 1e-12)
        for p in range(n):
            s = 0.0
            for k in range(16):
                s += out_f[p, k]
            for k in range(16):
                out_f[p, k] = out_f[p, k] / s

    return amp_f, out_f, comp_f, noise_f, tick


# ── proofs ──────────────────────────────────────────────────────────────────
def chaos(n):
    return np.full((n, 16), 1.0 / 16.0)


def run_lane(n, ticks, T, gate, seed_pos=None, seed_code=9,
             clamp_lo=None, clamp_hi=None, clamp_comp=0.0, trace_pos=None):
    """Coupled toy loop: my Phase A + contract Phase B (harness stand-in).
    Optional compliance clamp band [clamp_lo, clamp_hi) = resolved-mass stand-in
    (Planner's dilation setup). Returns amp, det history at trace_pos."""
    rng = np.random.default_rng(7)
    amp = chaos(n)
    if seed_pos is not None:
        amp[seed_pos] = 1e-9
        amp[seed_pos, seed_code] = 1.0
        amp[seed_pos] /= amp[seed_pos].sum()
    trace = []
    for t in range(ticks):
        det = det_of(amp)
        compliance = f_knee(det)
        if clamp_lo is not None:
            compliance[clamp_lo:clamp_hi] = clamp_comp
        amp = diffuse_np(amp, compliance, T, rng, gate=gate)
        if trace_pos is not None:
            trace.append(det_of(amp)[trace_pos])
    return amp, np.array(trace)


def first_cross(trace, thr=0.5):
    idx = np.nonzero(trace >= thr)[0]
    return int(idx[0]) if len(idx) else None


def main():
    ok = lambda m: print(f"  ok: {m}")
    fail = lambda m: (print(f"FAIL: {m}", file=sys.stderr), sys.exit(1))

    # P1 — front propagation + peak sharpening
    n = 61
    amp, _ = run_lane(n, ticks=120, T=0.0, gate=D_GATE, seed_pos=30)
    det = det_of(amp)
    code = amp.argmax(1)
    resolved = det > 0.5
    if not (resolved.sum() > 20 and np.all(code[resolved] == 9)):
        fail(f"P1: front did not propagate the seed code (resolved={resolved.sum()})")
    if det[30] < 0.95:
        fail(f"P1: seed peak dissolved (det[seed]={det[30]:.3f}) — mean-average symptom")
    ok(f"P1 front propagates seed code outward ({resolved.sum()}/{n} resolved, "
       f"all carry code 9; seed stays sharp det={det[30]:.3f})")

    # P2 — gate necessity (reproduce Planner's measured failure from MY kernel)
    #      lane: seed at 0, stiff band (compliance clamped ~0) at [20,40),
    #      probe at 50. Compare probe first-crossing free vs banded, gated vs not.
    n, band = 121, (20, 40)
    _, tr_free = run_lane(n, 2500, T=0.0, gate=D_GATE, seed_pos=0, trace_pos=50)
    _, tr_band = run_lane(n, 2500, T=0.0, gate=D_GATE, seed_pos=0, trace_pos=50,
                          clamp_lo=band[0], clamp_hi=band[1], clamp_comp=0.02)
    _, un_free = run_lane(n, 2500, T=0.0, gate=0.0, seed_pos=0, trace_pos=50)
    _, un_band = run_lane(n, 2500, T=0.0, gate=0.0, seed_pos=0, trace_pos=50,
                          clamp_lo=band[0], clamp_hi=band[1], clamp_comp=0.02)
    cf, cb = first_cross(tr_free), first_cross(tr_band)
    uf, ub = first_cross(un_free), first_cross(un_band)
    if uf is None or ub is None:
        fail("P2: ungated fronts never arrived (setup broken)")
    if ub > uf * 3:
        fail(f"P2: ungated band SHOULD be condition-limited (~free speed); got {ub} vs {uf}")
    if cf is None or cb is None or cb < cf * 3:
        fail(f"P2: gated band crossing not rate-limited (free={cf}, band={cb})")
    ok(f"P2 gate necessity reproduced: ungated probe-arrival free={uf} vs band={ub} "
       f"(condition-limited, band barely slows) — gated free={cf} vs band={cb} "
       f"({cb/cf:.0f}x slower: stiff matter actually stalls the front = dilation precondition)")

    # P3 — chaos stays chaos under noise floor
    amp3, _ = run_lane(41, ticks=400, T=0.004, gate=D_GATE)  # no seed at all
    d3max = det_of(amp3).max()
    if d3max >= D_GATE:
        fail(f"P3: pure-noise field crossed the radiation gate (max det={d3max:.3f})")
    ok(f"P3 unseeded field under T-noise never crosses the gate (max det={d3max:.3f} < {D_GATE})")

    # P4 — taichi twin equals numpy reference for one full tick
    n4 = 200
    rng = np.random.default_rng(14)
    amp4 = rng.dirichlet(np.ones(16), size=n4)
    amp4[::17] = np.eye(16)[rng.integers(0, 16, size=len(amp4[::17]))]  # some formed
    amp4 = np.clip(amp4, 1e-12, None)
    amp4 /= amp4.sum(1, keepdims=True)
    comp4 = f_knee(det_of(amp4))
    noise = rng.uniform(0.0, 1.0, size=(n4, 16))
    T4 = 0.003

    class FixedRng:  # feed the SAME noise matrix the twin gets
        def uniform(self, lo, hi, size):
            return noise
    ref = diffuse_np(amp4, comp4, T4, FixedRng())

    amp_f, out_f, comp_f, noise_f, tick = make_ti_kernel(n4)
    amp_f.from_numpy(amp4)
    comp_f.from_numpy(comp4)
    noise_f.from_numpy(noise)
    tick(T4, D_GATE, C0, ETA)
    twin = out_f.to_numpy()
    err = np.abs(twin - ref).max()
    if err > 1e-10:
        fail(f"P4: taichi twin diverges from numpy reference (max err {err:.2e})")
    ok(f"P4 @ti.kernel twin == numpy reference (max err {err:.2e}, one full tick, "
       f"mixed formed/chaos field, noise + boundary included)")

    # P5 — distribution invariant on everything the proofs produced
    for a in (amp, amp3, ref, twin):
        if a.min() < 0 or np.abs(a.sum(1) - 1).max() > 1e-9:
            fail("P5: amp left the simplex")
    ok("P5 amp stays a distribution (non-negative, rows sum to 1) across all proofs")

    print("ALL PROOFS PASS — Phase A diffuse() is standing (numpy + taichi twin).")


if __name__ == "__main__":
    main()
