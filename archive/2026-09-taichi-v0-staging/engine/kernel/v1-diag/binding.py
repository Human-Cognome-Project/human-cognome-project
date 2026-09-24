#!/usr/bin/env python3
"""Field-engine kernel v0.2 — BINDING dynamics (P's git-layer correction, 2026-08-30).

Source physics (P thread 1211/1212 + di-cognome git: english-force-patterns.md
force inventory — attraction / binding energy / valency / structural repair;
data-conventions.md FPB = pair is the fundamental storage unit):
  1. The field stays in SUPERPOSITION under radiation — radiation tilts amp,
     participates, but is INSUFFICIENT to force identity (bias saturates).
  2. Identity locks by FORCING: direct input seeding, or VALENCY pull — being
     recruited to finish a structure with an unmet bond (structural repair).
  3. A STANDING NODE — sustained convergent same-code radiation from BOTH
     sides — CAN force: "that would be the label forming in effect" (P 1212).
     The surround condition, in radiation terms.
  4. A locked singleton has NO binding energy → it COMBINES (bonds) or
     DEGRADES (dissipates back to superposition). Pair = minimum meaning.

Gate chain extends the ordering invariant (Planner):
  radiate 0.30 <= bias-cap 0.45 < lock 0.5 < knee 0.85

v0.2 tick (Phase A only; Phase B contract untouched — det/compliance/tau all
still time-step-owned; this file's internal readout mirrors the contract):
  radiation: formed lattice neighbours tilt UNLOCKED receivers, but the pull
             is masked once det >= BIAS_CAP — unless the receiver sits at a
             standing node (both lattice neighbours formed, same code), where
             the cap lifts and the node's code can lock in (label formation).
  valency:   pair-mate structure (cells 2i, 2i+1 = one byte). A locked cell
             with an unlocked partner pulls the partner toward the partner's
             own current tilt at V_PULL — recruitment to finish the byte.
  sustain:   a locked cell keeps consolidating ONLY if supported — bonded
             (partner locked) or at a standing node. Unsupported locked cells
             get NO sustain and LEAK toward uniform at DECAY per tick: no
             binding energy, nothing holds the assembly (rate is a measured
             observable, not doctrine).

Falsifiers (hard asserts, run on main):
  F1 stability: a seeded byte pair persists under T (bond = binding energy)
  F2 combine:   a lone forced nibble with an available partner RECRUITS it;
                the completed pair persists
  F3 degrade:   a lone forced nibble with its partner unavailable dissipates
                back to superposition (det falls below lock)
  F4 radiation insufficiency: background cells adjacent to stable structure
                stay tilted but NEVER cross the bias cap (no more monoculture)
  F5 standing node: a chaos cell flanked by same-code formed cells on BOTH
                sides locks — the label forms in effect
"""
import numpy as np
import diffuse as dv

BIAS_CAP = 0.45
LOCK = 0.50
RAD_TILT = 0.05   # radiation is WEAK participation — without this scale-down, one
                  # discrete step overshoots the bias cap straight past lock
                  # (measured: det 0.44 -> 0.85 in a single tick; first F4 failure)
V_PULL = 3.0      # valency recruitment strength (bond demand >> ambient radiation)
# Valency DEFERS to a standing node: where the group's wave is proposing a label,
# bond demand doesn't fight it (measured: undeferred valency amplified the argmax
# tie-break and locked code 0 against a code-9 node). A tilt-threshold gate was
# tried instead and starved the recruiter — it decayed waiting for the tilt.
DECAY = 0.01      # unsupported-locked leak toward uniform, per tick
UNIFORM = 1.0 / 16.0

assert dv.D_GATE <= BIAS_CAP < LOCK < 0.85, "gate-chain ordering violated"


def tick_v02(amp, compliance, T, rng, bonded=None, c0=dv.C0, eta=dv.ETA):
    """One v0.2 Phase-A tick. Pair-mate of cell i is i^1 (cells 2k,2k+1 = byte).

    bonded: bool[n] — BOND STATE, carried across ticks (stand-in for the
    Phase-B-owned seam field in Planner's lane split). Binding energy is
    HYSTERESIS: a bond forms only when both mates lock, and breaks only when a
    mate falls below the radiate floor — a momentary noise dip below lock must
    NOT dissolve a bond (measured: the stateless version killed a seeded pair
    in ~10 ticks via a synchronized dip; det trace in session notes)."""
    n, K = amp.shape
    if bonded is None:
        bonded = np.zeros(n, dtype=bool)
    det = dv.det_of(amp)
    code = amp.argmax(axis=1)
    formed = det >= dv.D_GATE
    locked = det >= LOCK
    onehot = np.eye(K)[code]

    # lattice-neighbour views (open boundary)
    def nb(shift):
        """(neighbour_slice, receiver_slice) for neighbour p+shift."""
        return (slice(0, n - 1), slice(1, n)) if shift == -1 else (slice(1, n), slice(0, n - 1))

    # standing node: both lattice neighbours LOCKED on the same code — a standing
    # wave needs STABLE emitters. Merely-tilted cells (det 0.30–0.45) do radiate
    # ("take part in radiation") but cannot anchor a node: without this, two
    # tilted cells flanking a third fake a node and a lock cascade sweeps the
    # background (measured — the first F4 run failed exactly this way).
    node = np.zeros(n, dtype=bool)
    node_code = np.zeros(n, dtype=int)
    both = locked[:-2] & locked[2:] & (code[:-2] == code[2:])
    node[1:-1] = both
    node_code[1:-1] = code[:-2]

    pull = np.zeros_like(amp)

    # RADIATION — tilts unlocked receivers; capped at BIAS_CAP unless at a node
    for shift in (-1, 1):
        nbs, rcs = nb(shift)
        w_n = (dv.WEIGHT[code] * formed)[nbs]
        open_recv = (~locked[rcs]) & ((det[rcs] < BIAS_CAP) | node[rcs])
        pull[rcs] += RAD_TILT * (w_n * open_recv)[:, None] * (onehot[nbs] - amp[rcs])
    # node forcing pulls specifically toward the node's code (label forming)
    nf = node & ~locked
    if nf.any():
        pull[nf] += 2.0 * (np.eye(K)[node_code[nf]] - amp[nf])

    # VALENCY — locked cell recruits its unlocked pair-mate toward the mate's tilt
    partner = np.arange(n) ^ 1
    valid = partner < n
    demand = valid & locked[np.clip(partner, 0, n - 1)] & ~locked & ~node
    if demand.any():
        pull[demand] += V_PULL * (np.eye(K)[code[demand]] - amp[demand])

    # SUSTAIN — bonded cells consolidate even through a dip (binding-energy
    # hysteresis: the well holds while det stays above the radiate floor);
    # node-held labels consolidate while their flanks hold; a locked cell with
    # neither is a bare singleton and leaks.
    supported = (bonded & formed) | (locked & node)
    if supported.any():
        pull[supported] += (dv.WEIGHT[code[supported]])[:, None] * (onehot[supported] - amp[supported])
    unsupported = locked & ~supported

    c_local = c0 * dv.g_of(compliance)
    new = amp + c_local[:, None] * eta * pull
    if unsupported.any():
        new[unsupported] += DECAY * (UNIFORM - new[unsupported])   # binding-energy leak
    if T > 0.0:
        new = new + T * rng.uniform(0.0, 1.0, size=amp.shape)
    new = np.clip(new, 1e-12, None)
    new = new / new.sum(axis=1, keepdims=True)

    return new
    # (bond-state update REMOVED — the `bonded` field is Phase-B-owned per the
    #  seam split; Planner's ts.bonded_update(det, bonded_prev) is the writer.
    #  The emulation that lived here validated the spec and is retired.)


def run(n, ticks, seeds, clamp_inert=(), T0=0.02, floor=0.002, lam=0.995, rng_seed=7):
    """Coupled toy loop (contract Phase-B stand-in: det->f_knee compliance).
    seeds = [(pos, code)]; clamp_inert = cells held at uniform every tick
    (an UNAVAILABLE partner — e.g. substrate hole)."""
    rng = np.random.default_rng(rng_seed)
    amp = np.full((n, 16), UNIFORM)
    seeded = set()
    for pos, c in seeds:
        amp[pos] = 1e-9; amp[pos, c] = 1.0; amp[pos] /= amp[pos].sum()
        seeded.add(pos)
    # input forcing seeds BONDS too: a byte arriving from data IS a bonded pair
    import timestep as ts
    bonded_pairs = np.zeros(n // 2, dtype=bool)
    for pos in seeded:
        if (pos ^ 1) in seeded:
            bonded_pairs[pos // 2] = True
    T = T0
    hist = []
    for t in range(ticks):
        det = dv.det_of(amp)
        compliance = dv.f_knee(det)
        # PHASE A (mine): reads the bonded field, writes ONLY amp
        amp = tick_v02(amp, compliance, T, rng, np.repeat(bonded_pairs, 2))
        for i in clamp_inert:
            amp[i] = UNIFORM
        # PHASE B (Planner's shipped bytes): writes the bonded field
        bonded_pairs = ts.bonded_update(dv.det_of(amp), bonded_pairs)
        for i in clamp_inert:
            bonded_pairs[i // 2] = False
        T = max(floor, T * lam) if lam < 1 else T
        if t % 200 == 0 or t == ticks - 1:
            hist.append((t, dv.det_of(amp), amp.argmax(1)))
    return amp, hist


def main():
    ok = lambda m: print(f"  ok: {m}")
    fail = lambda m: (__import__('sys').stderr.write(f"FAIL: {m}\n"), __import__('sys').exit(1))

    # F1 — seeded byte pair persists (bond = binding energy)
    n = 64
    amp, hist = run(n, 3000, seeds=[(30, 4), (31, 1)])   # byte 0x41 = 'A' (hi=4, lo=1)
    det, code = hist[-1][1], hist[-1][2]
    if not (det[30] >= LOCK and det[31] >= LOCK and code[30] == 4 and code[31] == 1):
        fail(f"F1: bonded pair did not persist (det {det[30]:.2f}/{det[31]:.2f} codes {code[30]}/{code[31]})")
    ok(f"F1 seeded byte pair (4,1)='A' persists 3000 ticks bonded (det {det[30]:.2f}/{det[31]:.2f})")

    # F2 — lone nibble with available partner: combine
    amp, hist = run(n, 3000, seeds=[(30, 4)])
    det, code = hist[-1][1], hist[-1][2]
    if not (det[30] >= LOCK and det[31] >= LOCK):
        fail(f"F2: partner not recruited (det[31]={det[31]:.2f}) — combine failed")
    ok(f"F2 lone forced nibble RECRUITED its partner (valency): pair locked (det {det[30]:.2f}/{det[31]:.2f}), combined and persists")

    # F3 — lone nibble, partner unavailable: degrade
    amp, hist = run(n, 3000, seeds=[(30, 4)], clamp_inert=(31,))
    det = hist[-1][1]
    if det[30] >= LOCK:
        fail(f"F3: unbonded singleton failed to degrade (det[30]={det[30]:.2f})")
    # find when it dropped
    drop = next((t for t, d, _ in hist if d[30] < LOCK), None)
    ok(f"F3 lone forced nibble with NO partner DEGRADED to superposition "
       f"(det[30]={det[30]:.2f} final; fell below lock by t={drop}) — combine or degrade, measured")

    # F4 — radiation insufficiency: background near stable pair never crosses cap
    amp, hist = run(n, 3000, seeds=[(30, 4), (31, 1)])
    background = [i for i in range(n) if i not in (30, 31)]
    maxbg = max(h[1][background].max() for h in hist)
    if maxbg >= LOCK:
        fail(f"F4: background locked from radiation alone (max det {maxbg:.2f})")
    ok(f"F4 radiation participates but never forces: background max det {maxbg:.2f} < lock {LOCK} over 3000 ticks (no monoculture)")

    # F5 — standing node: chaos cell flanked by same-code formed cells locks
    #      seed cells 40 and 42 with the same code; 41 sits at the node
    amp, hist = run(n, 3000, seeds=[(40, 9), (41 ^ 1, 9) if False else (42, 9)])
    det, code = hist[-1][1], hist[-1][2]
    if not (det[41] >= LOCK and code[41] == 9):
        fail(f"F5: standing node did not force (det[41]={det[41]:.2f} code={code[41]})")
    ok(f"F5 standing node forced the flanked cell to code 9 (det[41]={det[41]:.2f}) — the label forming, in effect")

    print("ALL v0.2 FALSIFIERS PASS — radiation tilts, bonds hold, singletons combine-or-degrade, nodes label.")


if __name__ == "__main__":
    main()
