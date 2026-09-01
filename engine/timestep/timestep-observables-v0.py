#!/usr/bin/env python3
"""timestep-observables-v0 — the time-step lane's reader over a poured state.

Planner's half of the lane split (temporality/time-step): given any pour-state
npz (det, code, bonded, tau_pair, optional leaf_val), emit the three reads the
seam owes, instrument-first so the machinery is validated on the small states
BEFORE silas's corpus pour arrives:

  1. TAU LEDGER — tau_pair is the honest healing clock (each break that
     re-completes ticks once). Totals, per-pair histogram, healed-pair byte
     classes. On states with an acceptance report this is a POSITIVE-CONTROL
     GATE: Moby raw must read exactly 22 (19 space + 3 other); the v0.3.1
     lexicon must read exactly 0.
  2. DILATION READ — det as local compliance: distribution, per-byte-class
     means, depressed-det tail (slow zones), run-length structure of
     contiguous low-det leaves. "Black-hole dilation" at corpus scale will be
     the tail of this read; here we establish the instrument + baseline shape.
  3. SCAFFOLD-DEPENDENCE — det conditioned on byte class and on the pair-mate's
     class: does a leaf's compliance depend on what it is bonded TO? This is
     the read that must NOT be fed frequency (frequency stays emergent —
     sampler-shadow is a separate, later check).

Byte classes: space (0x20), newline/CR, letter (A-Za-z), digit, utf8-cont
(0x80-0xBF), punct/other. Bytes come from code nibbles (hi even, lo odd);
where leaf_val exists, code-vs-given intactness is asserted first.

No verdicts baked in: numbers + gates + explicit caveats. Runtime: seconds on
Moby (2.55M leaves), ~1 min on the 16.78M lexicon. CPU only.
Planner, 2026-09-01 (FP seat).
"""
import json
import sys
from pathlib import Path

import numpy as np

STATES = {
    "moby-raw-fpcuda": {
        "path": Path.home() / "engine/hcp/engine/kernel/pour-raw-mobydick-02701-fpcuda-state.npz",
        "gate": {"tau_total": 22, "healed_by_class_space": 19, "healed_by_class_other": 3},
    },
    "lexicon-v031": {
        "path": Path.home() / "shared-brain/exchange/pour-state-v031-acceptance.npz",
        "gate": {"tau_total": 0},
    },
}


def byte_class(b):
    cls = np.full(b.shape, 5, dtype=np.int8)              # punct/other
    cls[b == 0x20] = 0                                     # space
    cls[(b == 0x0A) | (b == 0x0D)] = 1                     # newline/cr
    letter = ((b >= 0x41) & (b <= 0x5A)) | ((b >= 0x61) & (b <= 0x7A))
    cls[letter] = 2                                        # letter
    cls[(b >= 0x30) & (b <= 0x39)] = 3                     # digit
    cls[(b >= 0x80) & (b <= 0xBF)] = 4                     # utf8-cont
    return cls


CLASS_NAMES = ["space", "newline", "letter", "digit", "utf8-cont", "punct/other"]


def read_state(tag, spec):
    z = np.load(spec["path"])
    det = z["det"].astype(np.float64)
    code = z["code"].astype(np.int64)
    tau = z["tau_pair"].astype(np.int64)
    bonded = z["bonded"]
    n_leaf, n_pair = len(det), len(tau)
    assert n_leaf == 2 * n_pair

    intact = None
    if "leaf_val" in z.files:
        intact = bool((code == z["leaf_val"].astype(np.int64)).all())

    byte_hi, byte_lo = code[0::2], code[1::2]
    bytes_ = (byte_hi * 16 + byte_lo).astype(np.int64)     # one byte per PAIR
    bcls = byte_class(bytes_)

    # --- 1. tau ledger ---
    tau_total = int(tau.sum())
    healed = np.flatnonzero(tau > 0)
    healed_cls = bcls[healed]
    healed_by_class = {CLASS_NAMES[c]: int((healed_cls == c).sum())
                       for c in range(6) if (healed_cls == c).any()}
    tau_hist = {int(v): int(n) for v, n in
                zip(*np.unique(tau, return_counts=True))}

    gate = {}
    g = spec.get("gate", {})
    if "tau_total" in g:
        gate["tau_total"] = {"expect": g["tau_total"], "got": tau_total,
                             "pass": tau_total == g["tau_total"]}
    if "healed_by_class_space" in g:
        got_space = healed_by_class.get("space", 0)
        got_other = tau_total - got_space
        gate["healed_class_split"] = {
            "expect": {"space": g["healed_by_class_space"], "other": g["healed_by_class_other"]},
            "got": {"space": got_space, "other": got_other},
            "pass": got_space == g["healed_by_class_space"] and got_other == g["healed_by_class_other"]}

    # --- 2. dilation read (det as compliance; pair det = mean of mates) ---
    pdet = 0.5 * (det[0::2] + det[1::2])
    qs = np.percentile(pdet, [0, 1, 5, 25, 50, 75, 95, 99, 100])
    per_class_det = {CLASS_NAMES[c]: {
        "n": int((bcls == c).sum()),
        "mean_det": round(float(pdet[bcls == c].mean()), 6)}
        for c in range(6) if (bcls == c).any()}
    lo_thresh = float(np.percentile(pdet, 5))
    slow = pdet < lo_thresh
    edges = np.diff(slow.astype(np.int8))
    starts = np.flatnonzero(edges == 1) + 1
    ends = np.flatnonzero(edges == -1) + 1
    if slow[0]:
        starts = np.concatenate([[0], starts])
    if slow[-1]:
        ends = np.concatenate([ends, [len(slow)]])
    runs = ends - starts
    dilation = {
        "det_percentiles_pair": {str(p): round(float(v), 6)
                                 for p, v in zip([0, 1, 5, 25, 50, 75, 95, 99, 100], qs)},
        "per_byte_class": per_class_det,
        "slow_zone_p5": {"threshold": round(lo_thresh, 6), "n_zones": int(len(runs)),
                         "max_run_pairs": int(runs.max()) if len(runs) else 0,
                         "mean_run_pairs": round(float(runs.mean()), 2) if len(runs) else 0.0},
    }

    # --- 3. scaffold-dependence: leaf det conditioned on OWN vs MATE nibble class ---
    # within a pair, mates carry hi/lo nibble of one byte — dependence read is
    # cross-PAIR: pair det conditioned on the NEIGHBOR pair's byte class.
    nb_prev = np.empty_like(bcls); nb_prev[1:] = bcls[:-1]; nb_prev[0] = -1
    scaffold = {}
    for c in range(6):
        m = bcls == c
        if not m.any():
            continue
        row = {}
        for cn in range(6):
            mm = m & (nb_prev == cn)
            if mm.sum() >= 100:
                row[CLASS_NAMES[cn]] = round(float(pdet[mm].mean()), 6)
        scaffold[CLASS_NAMES[c]] = {"own_mean": round(float(pdet[m].mean()), 6),
                                    "by_prev_neighbor_class": row}

    return {
        "state": tag, "n_leaves": int(n_leaf), "n_pairs": int(n_pair),
        "all_bonded": bool(bonded.all()), "code_equals_given": intact,
        "tau_ledger": {"tau_total": tau_total, "tau_hist": tau_hist,
                       "healed_by_class": healed_by_class},
        "gates": gate,
        "dilation": dilation,
        "scaffold_dependence": scaffold,
    }


out = {"artifact": "timestep-observables-v0",
       "caveats": ["single-document / lexicon states — corpus-scale dilation tail "
                   "and tau distributions await silas's corpus pour; this run "
                   "validates the INSTRUMENT (gates) and baselines the shapes",
                   "frequency is never computed here — emergent-only per lane spec; "
                   "sampler-shadow check is a separate artifact"],
       "reads": [read_state(t, s) for t, s in STATES.items()]}

gates_pass = all(g["pass"] for r in out["reads"] for g in r["gates"].values())
out["all_gates_pass"] = bool(gates_pass)
p = Path.home() / "shared-brain/exchange/timestep-observables-v0-report.json"
p.write_text(json.dumps(out, indent=1))
print("GATES:", "PASS" if gates_pass else "FAIL")
for r in out["reads"]:
    print(f"-- {r['state']}: tau={r['tau_ledger']['tau_total']} gates={r['gates']}")
    print("   det per class:", {k: v['mean_det'] for k, v in r['dilation']['per_byte_class'].items()})
print("wrote", p)
