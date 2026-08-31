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
  F6 the wall (v0.3): the pour's drift geometry at toy scale — a FREE cell
                converts (channel live: positive control), an emergent BONDED
                cell holds (b-freeze), a SEALED cell holds (given = bedrock)
  F7 restoration clock (v0.3): a degraded sealed cell is recompleted to its
                GIVEN value (byte template), and the restoration ticks tau_pair
  F8 revision pays time (v0.3): an emergent cell relabels ONLY after its bond
                breaks, and the re-completion ticks tau_pair — revision clocked
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


def tick_v02(amp, compliance, T, rng, bonded=None, given=None, c0=dv.C0, eta=dv.ETA):
    """One v0.3 Phase-A tick. Pair-mate of cell i is i^1 (cells 2k,2k+1 = byte).

    bonded: bool[n] — BOND STATE, carried across ticks (stand-in for the
    Phase-B-owned seam field in Planner's lane split). Binding energy is
    HYSTERESIS: a bond forms only when both mates lock, and breaks only when a
    mate falls below the radiate floor — a momentary noise dip below lock must
    NOT dissolve a bond (measured: the stateless version killed a seeded pair
    in ~10 ticks via a synchronized dip; det trace in session notes).

    given: int[n], -1 = emergent — v0.3 TWO-CLASS IDENTITY WALL (seam §P-STEER;
    the 200-tick lexicon pour measured 1.2M silent identity mutations through
    the standing-node channel, zero clock ticks — a wall violation):
      SEALED (given >= 0): data-seeded cells answer ONLY to their given value.
        Code-writes toward any other code are masked REGARDLESS of bond state;
        the sole completion target is the given value (byte template) —
        restoration, not revision. Existence may waver; identity may not.
      EMERGENT BONDED: code frozen to the cell's current code while the bond
        holds (§POUR RESPONSE (b)) — revision must pass break→relabel→
        re-complete, so tau_pair clocks it. NOTE (flagged to planner): the
        freeze anchors to current argmax, not a committed-code seam field, so
        a pure noise-walk of argmax mid-dip can still relabel unclocked at low
        probability; the durable anchor is Phase-B-owned.
      EMERGENT FREE: unrestricted v0.2 physics (formation machinery intact)."""
    n, K = amp.shape
    if bonded is None:
        bonded = np.zeros(n, dtype=bool)
    if given is None:
        given = np.full(n, -1, dtype=int)
    det = dv.det_of(amp)
    code = amp.argmax(axis=1)
    formed = det >= dv.D_GATE
    locked = det >= LOCK
    onehot = np.eye(K)[code]
    sealed = given >= 0
    restricted = sealed | bonded
    allowed = np.where(sealed, given, code)        # the one code this cell may move toward
    onehot_allowed = np.eye(K)[np.clip(allowed, 0, K - 1)]

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

    # RADIATION — tilts unlocked receivers; capped at BIAS_CAP unless at a node.
    # v0.3: a restricted receiver accepts radiation ONLY from a neighbour
    # radiating its allowed code (foreign radiation still lands on free cells —
    # the formation channel is untouched).
    for shift in (-1, 1):
        nbs, rcs = nb(shift)
        w_n = (dv.WEIGHT[code] * formed)[nbs]
        open_recv = (~locked[rcs]) & ((det[rcs] < BIAS_CAP) | node[rcs])
        keep = (~restricted[rcs]) | (code[nbs] == allowed[rcs])
        pull[rcs] += RAD_TILT * (w_n * open_recv * keep)[:, None] * (onehot[nbs] - amp[rcs])
    # node forcing pulls specifically toward the node's code (label forming).
    # v0.3: fired at a restricted cell it would be meaning-REVISION, not
    # formation — a node has AUTHORITY over a cell only if the cell is free or
    # the node proposes its allowed code.
    node_acts = node & ((~restricted) | (node_code == allowed))
    nf = node_acts & ~locked
    if nf.any():
        pull[nf] += 2.0 * (np.eye(K)[node_code[nf]] - amp[nf])

    # VALENCY — locked cell recruits its unlocked pair-mate. v0.3: the target is
    # the mate's ALLOWED code — for a sealed mate that is the given value (the
    # byte template supplying the completion target: restoration); for emergent
    # mates it remains the mate's own current tilt (v0.2 behaviour).
    # Deference is to node AUTHORITY, not node presence (v0.3.1): the v0.2 rule
    # deferred to any node, but a node the wall has masked has nothing to defer
    # to — the first wall re-pour MEASURED the hole (4.4k pairs broke by t=20,
    # zero re-completions: a sealed light nibble at a matching foreign node had
    # every recovery channel masked and died PERMANENTLY on first deep dip).
    partner = np.arange(n) ^ 1
    valid = partner < n
    demand = valid & locked[np.clip(partner, 0, n - 1)] & ~locked & ~node_acts
    if demand.any():
        pull[demand] += V_PULL * (onehot_allowed[demand] - amp[demand])

    # SUSTAIN — bonded cells consolidate even through a dip (binding-energy
    # hysteresis: the well holds while det stays above the radiate floor);
    # node-held labels consolidate while their flanks hold; a locked cell with
    # neither is a bare singleton and leaks. v0.3: consolidation targets the
    # allowed code, so a sealed cell whose argmax wobbled consolidates HOME.
    supported = (bonded & formed) | (locked & node)
    if supported.any():
        pull[supported] += (dv.WEIGHT[allowed[supported]])[:, None] * (onehot_allowed[supported] - amp[supported])
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


def run(n, ticks, seeds, clamp_inert=(), given=None, clamp_until=None,
        T0=0.02, floor=0.002, lam=0.995, rng_seed=7):
    """Coupled toy loop (contract Phase-B stand-in: det->f_knee compliance).
    seeds = [(pos, code)]; clamp_inert = cells held at uniform every tick
    (an UNAVAILABLE partner — e.g. substrate hole).
    given = [(pos, code)] SEALED cells (v0.3 wall; usually == seeds when the
    seed IS data); clamp_until = {pos: release_tick} timed degradation (a cell
    held at uniform until its release tick — forces a bond break, then frees).
    Returns (amp, hist, extra) — extra carries bonded_pairs + tau_pair (the
    completion/restoration ledger, rule per ts.phase_b_bonded)."""
    rng = np.random.default_rng(rng_seed)
    amp = np.full((n, 16), UNIFORM)
    seeded = set()
    for pos, c in seeds:
        amp[pos] = 1e-9; amp[pos, c] = 1.0; amp[pos] /= amp[pos].sum()
        seeded.add(pos)
    given_arr = np.full(n, -1, dtype=int)
    for pos, c in (given or []):
        given_arr[pos] = c
    clamp_until = clamp_until or {}
    # input forcing seeds BONDS too: a byte arriving from data IS a bonded pair
    import timestep as ts
    bonded_pairs = np.zeros(n // 2, dtype=bool)
    for pos in seeded:
        if (pos ^ 1) in seeded:
            bonded_pairs[pos // 2] = True
    tau_pair = np.zeros(n // 2, dtype=np.int64)
    T = T0
    hist = []
    for t in range(ticks):
        det = dv.det_of(amp)
        compliance = dv.f_knee(det)
        # PHASE A (mine): reads the bonded field, writes ONLY amp
        amp = tick_v02(amp, compliance, T, rng, np.repeat(bonded_pairs, 2), given_arr)
        for i in clamp_inert:
            amp[i] = UNIFORM
        for i, rel in clamp_until.items():
            if t < rel:
                amp[i] = UNIFORM
        # PHASE B (Planner's shipped bytes): writes the bonded field
        new_bonded = ts.bonded_update(dv.det_of(amp), bonded_pairs)
        tau_pair[new_bonded & ~bonded_pairs] += 1   # ts.phase_b_bonded rule
        bonded_pairs = new_bonded
        for i in clamp_inert:
            bonded_pairs[i // 2] = False
        T = max(floor, T * lam) if lam < 1 else T
        if t % 200 == 0 or t == ticks - 1:
            hist.append((t, dv.det_of(amp), amp.argmax(1)))
    return amp, hist, {"bonded_pairs": bonded_pairs, "tau_pair": tau_pair}


def main():
    ok = lambda m: print(f"  ok: {m}")
    fail = lambda m: (__import__('sys').stderr.write(f"FAIL: {m}\n"), __import__('sys').exit(1))

    # F1 — seeded byte pair persists (bond = binding energy)
    n = 64
    amp, hist, _ = run(n, 3000, seeds=[(30, 4), (31, 1)])   # byte 0x41 = 'A' (hi=4, lo=1)
    det, code = hist[-1][1], hist[-1][2]
    if not (det[30] >= LOCK and det[31] >= LOCK and code[30] == 4 and code[31] == 1):
        fail(f"F1: bonded pair did not persist (det {det[30]:.2f}/{det[31]:.2f} codes {code[30]}/{code[31]})")
    ok(f"F1 seeded byte pair (4,1)='A' persists 3000 ticks bonded (det {det[30]:.2f}/{det[31]:.2f})")

    # F2 — lone nibble with available partner: combine
    amp, hist, _ = run(n, 3000, seeds=[(30, 4)])
    det, code = hist[-1][1], hist[-1][2]
    if not (det[30] >= LOCK and det[31] >= LOCK):
        fail(f"F2: partner not recruited (det[31]={det[31]:.2f}) — combine failed")
    ok(f"F2 lone forced nibble RECRUITED its partner (valency): pair locked (det {det[30]:.2f}/{det[31]:.2f}), combined and persists")

    # F3 — lone nibble, partner unavailable: degrade
    amp, hist, _ = run(n, 3000, seeds=[(30, 4)], clamp_inert=(31,))
    det = hist[-1][1]
    if det[30] >= LOCK:
        fail(f"F3: unbonded singleton failed to degrade (det[30]={det[30]:.2f})")
    # find when it dropped
    drop = next((t for t, d, _ in hist if d[30] < LOCK), None)
    ok(f"F3 lone forced nibble with NO partner DEGRADED to superposition "
       f"(det[30]={det[30]:.2f} final; fell below lock by t={drop}) — combine or degrade, measured")

    # F4 — radiation insufficiency: background near stable pair never crosses cap
    amp, hist, _ = run(n, 3000, seeds=[(30, 4), (31, 1)])
    background = [i for i in range(n) if i not in (30, 31)]
    maxbg = max(h[1][background].max() for h in hist)
    if maxbg >= LOCK:
        fail(f"F4: background locked from radiation alone (max det {maxbg:.2f})")
    ok(f"F4 radiation participates but never forces: background max det {maxbg:.2f} < lock {LOCK} over 3000 ticks (no monoculture)")

    # F5 — standing node: chaos cell flanked by same-code formed cells locks
    #      seed cells 40 and 42 with the same code; 41 sits at the node
    amp, hist, _ = run(n, 3000, seeds=[(40, 9), (41 ^ 1, 9) if False else (42, 9)])
    det, code = hist[-1][1], hist[-1][2]
    if not (det[41] >= LOCK and code[41] == 9):
        fail(f"F5: standing node did not force (det[41]={det[41]:.2f} code={code[41]})")
    ok(f"F5 standing node forced the flanked cell to code 9 (det[41]={det[41]:.2f}) — the label forming, in effect")

    # ── v0.3 falsifiers (seam §P-STEER two-class wall) ──────────────────────
    # F6 — the pour's drift geometry at toy scale: byte 'a'=(6,1) at (40,41),
    # next byte's hi=6 at 42 (pair (42,43)=(6,12) keeps the flank locked).
    # Constant T (lam=1) keeps noise dips coming — the capture precondition.
    flank = [(42, 6), (43, 12)]
    # F6a positive control: cell 41 FREE (unseeded) at the node — must convert,
    # else the channel is dead and F6b/F6c are vacuous (empty-check discipline).
    amp, hist, ex = run(64, 3000, seeds=[(40, 6)] + flank, lam=1.0, rng_seed=13)
    det, code = hist[-1][1], hist[-1][2]
    if not (det[41] >= LOCK and code[41] == 6):
        fail(f"F6a: free cell at drift geometry did NOT convert (det {det[41]:.2f} "
             f"code {code[41]}) — capture channel dead, wall tests vacuous")
    ok(f"F6a drift channel LIVE on a free cell: node captured 41 to code 6 (det {det[41]:.2f}) — positive control")
    # F6b emergent bonded: same geometry, 41 seeded 1 and bonded to 40 — frozen.
    amp, hist, ex = run(64, 3000, seeds=[(40, 6), (41, 1)] + flank, lam=1.0, rng_seed=13)
    det, code = hist[-1][1], hist[-1][2]
    # identity + bond must hold; det may BREATHE (a weight-2 code's equilibrium
    # under constant T=0.02 sits near the bias cap — existence wavers, identity
    # may not, per §P-STEER; the annealed pour recovers det, this lane doesn't)
    if not (code[41] == 1 and ex["bonded_pairs"][20] and det[41] >= dv.D_GATE):
        fail(f"F6b: emergent BONDED cell was relabeled or lost its bond "
             f"(code {code[41]}, det {det[41]:.2f}, bonded {ex['bonded_pairs'][20]}) — (b)-freeze broken")
    ok(f"F6b emergent bonded cell HELD code 1 under 3000 ticks of node pressure (det {det[41]:.2f}, bond intact) — bond freezes identity")
    # F6c sealed: same, with given == seeds — the wall regardless of bond state.
    amp, hist, ex = run(64, 3000, seeds=[(40, 6), (41, 1)] + flank,
                        given=[(40, 6), (41, 1)] + flank, lam=1.0, rng_seed=13)
    det, code = hist[-1][1], hist[-1][2]
    if not (code[41] == 1 and ex["bonded_pairs"][20] and det[41] >= dv.D_GATE):
        fail(f"F6c: SEALED cell was relabeled or lost its bond (code {code[41]}, "
             f"det {det[41]:.2f}, bonded {ex['bonded_pairs'][20]}) — the wall is broken")
    ok(f"F6c sealed cell HELD its given code 1 (det {det[41]:.2f}, bond intact) — given matter is bedrock")

    # F7 — restoration clock: sealed pair (30,31)=(4,1); 31 clamped to uniform
    # for 5 ticks (bond breaks at t=0), then released. The locked mate must
    # recomplete 31 to its GIVEN value — not to whatever noise tilted — and the
    # restoration must tick tau_pair exactly once (a dated healing event).
    # Calm lane (T0=0.005) + short clamp: a bond-broken survivor is UNSUPPORTED
    # and erodes ~4%/tick at T=0.02 (measured; F3's fast-fall) — the claim under
    # test is restoration-target+clock, not survival of the restorer in a storm.
    amp, hist, ex = run(64, 3000, seeds=[(30, 4), (31, 1)], T0=0.005,
                        given=[(30, 4), (31, 1)], clamp_until={31: 5}, rng_seed=29)
    det, code = hist[-1][1], hist[-1][2]
    tp = ex["tau_pair"][15]
    if not (code[30] == 4 and code[31] == 1 and det[31] >= LOCK):
        fail(f"F7: restoration failed or landed foreign (codes {code[30]}/{code[31]}, det {det[31]:.2f})")
    if tp != 1 or not ex["bonded_pairs"][15]:
        fail(f"F7: restoration not clocked exactly once (tau_pair={tp}, bonded={ex['bonded_pairs'][15]})")
    ok(f"F7 degraded sealed cell RESTORED to given value (byte template), bond re-completed, tau_pair ticked {tp} — restoration is a dated event")

    # F8 — revision pays time: emergent 'a' pair in the drift geometry; 41
    # clamped 5 ticks so its bond BREAKS. Released free at the node, it may
    # legally relabel (revision) — and the re-completion must tick the clock.
    # Same calm lane as F7: the node flank 40 must still be locked at release.
    amp, hist, ex = run(64, 3000, seeds=[(40, 6), (41, 1)] + flank, T0=0.005,
                        clamp_until={41: 5}, lam=1.0, rng_seed=13)
    det, code = hist[-1][1], hist[-1][2]
    tp = ex["tau_pair"][20]
    if not (code[41] == 6 and det[41] >= LOCK):
        fail(f"F8: post-break relabel did not happen (code {code[41]}, det {det[41]:.2f}) — revision path dead")
    if tp < 1 or not ex["bonded_pairs"][20]:
        fail(f"F8: revision not clocked (tau_pair={tp}, bonded={ex['bonded_pairs'][20]})")
    ok(f"F8 emergent cell relabeled 1→6 ONLY after its bond broke, re-completed, tau_pair={tp} — revision paid time")

    # F9 — restoration AT a matching foreign node (v0.3.1; the hole the first
    # wall re-pour measured): sealed 'a' pair with flank 42=6 puts 41 at a 6-6
    # node whose code is FOREIGN to 41's given 1. Break 41's bond (clamp 5),
    # release: node capture is masked (wall) AND valency must NOT defer to the
    # masked node — the locked mate restores 41 to given, clocked.
    amp, hist, ex = run(64, 3000, seeds=[(40, 6), (41, 1)] + flank, T0=0.005,
                        given=[(40, 6), (41, 1)] + flank,
                        clamp_until={41: 5}, lam=1.0, rng_seed=13)
    det, code = hist[-1][1], hist[-1][2]
    tp = ex["tau_pair"][20]
    if not (code[41] == 1 and det[41] >= dv.D_GATE and ex["bonded_pairs"][20] and tp >= 1):
        fail(f"F9: no restoration at masked node (code {code[41]}, det {det[41]:.2f}, "
             f"bonded {ex['bonded_pairs'][20]}, tau {tp}) — the pour's death mechanism stands")
    ok(f"F9 sealed cell at a MATCHING foreign node restored to given after break "
       f"(det {det[41]:.2f}, tau_pair={tp}) — valency defers to node AUTHORITY, not node presence")

    print("ALL v0.3 FALSIFIERS PASS — formation free, bonds freeze, given is bedrock, restoration and revision are clocked events.")


if __name__ == "__main__":
    main()
