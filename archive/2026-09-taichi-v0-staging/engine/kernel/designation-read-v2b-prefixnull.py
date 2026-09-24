#!/usr/bin/env python3
"""designation-read-v2b — prefix-stratified null (planner timing call, silas endorsed pre-attribution).

Question: is the v2 residual — adj +20.0z, noun -11.67z, num +7.57z, verb -7.02z
under the (final-trigram, len) null — carried by INITIAL morphology (un-, in-,
non-, ... prefix mass)? Three nulls, same closed-form SRSWOR machinery as v2.1:

  F: (final-trigram, len)             — exact re-run; CONSISTENCY GATE vs the
                                         v2 report (must reproduce before P/J
                                         numbers are trusted).
  P: (initial-trigram, len)           — the prefix null proper.
  J: (initial x final trigram, len)   — the decisive null: both edges
                                         controlled. If adj z collapses here
                                         WITH adequate movability, the residual
                                         was edge-morphology mass; if it
                                         survives, designation structure exists
                                         beyond both word edges.

Movability is computed FIRST for each null: a stratum owned by one class
cannot move under shuffle. Where J shatters into single-class strata, the
immobility IS the morphology answer (v2 caveat convention), not a defect —
but then J's z is uninformative and the P null carries the falsification.

Caveat carried: "beyond edge trigrams" != "beyond all morphology" (infixes,
templates, compounds uncontrolled).

Data identical to v2: word bytes from state `code` (code_drift_cells=0 ⇒
code == given), det from pour-state-v031-acceptance.npz.
Planner, 2026-09-01 (FP seat).
"""
import json
import math
from pathlib import Path

import numpy as np

EXCH = Path.home() / "shared-brain" / "exchange"
POS_LABELS = ['adj', 'adv', 'article', 'character', 'conj', 'contraction', 'det',
              'infix', 'intj', 'name', 'noun', 'num', 'particle', 'phrase',
              'postp', 'prefix', 'prep', 'prep_phrase', 'pron', 'suffix',
              'symbol', 'verb']
B_EMP = 200
SEED = 179
V2_GATE = {"adj": 20.0, "noun": -11.67, "num": 7.57, "verb": -7.02}
GATE_TOL = 0.05  # reproduce v2's reported z (rounded to 2dp) within this

z = np.load(EXCH / "snode-chains-v0.npz")
off = z["word_char__off"].astype(np.int64)
pos_off = z["pos__off"].astype(np.int64)
pos_members = z["pos__members"].astype(np.int64)
n_word = len(off) - 1

st = np.load(EXCH / "pour-state-v031-acceptance.npz")
det = st["det"].astype(np.float64).ravel()
code = st["code"].astype(np.int64).ravel()

cs = np.concatenate([[0.0], np.cumsum(det)])
leaf_lo, leaf_hi = 2 * off[:-1], 2 * off[1:]
word_mean = (cs[leaf_hi] - cs[leaf_lo]) / (leaf_hi - leaf_lo)
word_len = (off[1:] - off[:-1]).astype(np.int64)

byte_vals = code[0::2] * 16 + code[1::2]
b_lo, b_hi = off[:-1], off[1:]

# final trigram — verbatim v2 construction
last1 = byte_vals[b_hi - 1]
last2 = np.where(word_len >= 2, byte_vals[np.maximum(b_hi - 2, b_lo)], 256)
last3 = np.where(word_len >= 3, byte_vals[np.maximum(b_hi - 3, b_lo)], 256)
fin_tri = (last3 * 257 + last2) * 257 + last1

# initial trigram — mirror construction from the front
first1 = byte_vals[b_lo]
first2 = np.where(word_len >= 2, byte_vals[np.minimum(b_lo + 1, b_hi - 1)], 256)
first3 = np.where(word_len >= 3, byte_vals[np.minimum(b_lo + 2, b_hi - 1)], 256)
ini_tri = (first1 * 257 + first2) * 257 + first3

lenb = np.clip(word_len, 1, 20)
TRI_SPAN = 257 ** 3  # 16_974_593; joint key stays < 2^63
keys = {
    "F_final": fin_tri * 21 + lenb,
    "P_initial": ini_tri * 21 + lenb,
    "J_joint": (ini_tri * TRI_SPAN + fin_tri) * 21 + lenb,
}

label = np.full(n_word, -1, dtype=np.int64)
for g in range(len(pos_off) - 1):
    label[pos_members[pos_off[g]:pos_off[g + 1]]] = g
labeled = np.flatnonzero(label >= 0)
n_groups = len(pos_off) - 1

wm = word_mean[labeled]
lab = label[labeled]
counts = np.bincount(lab, minlength=n_groups)
obs_mean = np.bincount(lab, weights=wm, minlength=n_groups) / np.maximum(counts, 1)


def run_null(strata_key_full, tag):
    _, stratum = np.unique(strata_key_full[labeled], return_inverse=True)
    n_strata = int(stratum.max()) + 1

    N_s = np.bincount(stratum, minlength=n_strata).astype(np.float64)
    sum_s = np.bincount(stratum, weights=wm, minlength=n_strata)
    sumsq_s = np.bincount(stratum, weights=wm * wm, minlength=n_strata)
    m_s = sum_s / N_s
    s2_s = np.maximum(sumsq_s / N_s - m_s * m_s, 0.0)

    n_gs = np.bincount(stratum * n_groups + lab,
                       minlength=n_strata * n_groups).reshape(n_strata, n_groups).astype(np.float64)
    fpc = (N_s[:, None] - n_gs) / np.maximum(N_s[:, None] - 1.0, 1.0)
    mu0 = (n_gs * m_s[:, None]).sum(0) / np.maximum(counts, 1)
    var0 = (n_gs * s2_s[:, None] * fpc).sum(0) / np.maximum(counts, 1) ** 2
    sd0 = np.sqrt(var0)
    zscore = (obs_mean - mu0) / np.where(sd0 > 0, sd0, np.inf)

    base_order = np.argsort(stratum, kind="stable")
    strat_f = stratum.astype(np.float64)
    rng = np.random.default_rng(SEED)
    null_means = np.empty((B_EMP, n_groups))
    wm_null = np.empty_like(wm)
    for b in range(B_EMP):
        order = np.argsort(strat_f + rng.random(len(wm)))
        wm_null[base_order] = wm[order]
        null_means[b] = np.bincount(lab, weights=wm_null, minlength=n_groups) / np.maximum(counts, 1)
    pct_emp = (null_means < obs_mean).mean(0)
    emp_mu_maxdiff = float(np.abs(null_means.mean(0) - mu0).max())

    cls_per_stratum = np.zeros(n_strata, dtype=np.int64)
    for g in range(n_groups):
        marks = np.zeros(n_strata, dtype=bool)
        marks[stratum[lab == g]] = True
        cls_per_stratum += marks
    movable = cls_per_stratum[stratum] > 1

    rows = sorted(
        ({"pos": POS_LABELS[g], "n_words": int(counts[g]),
          "movable_frac": round(float(movable[lab == g].mean()), 3),
          "delta": round(float(obs_mean[g] - mu0[g]), 6),
          "z": round(float(zscore[g]), 2),
          "pctile_emp200": round(float(pct_emp[g]), 4)}
         for g in range(n_groups) if counts[g] > 0),
        key=lambda r: -abs(r["z"]))
    return {"null": tag, "n_strata": n_strata,
            "movable_frac_overall": round(float(movable.mean()), 3),
            "emp_vs_exact_mu_maxdiff": emp_mu_maxdiff,
            "per_pos": rows}


results = {k: run_null(v, k) for k, v in keys.items()}

# consistency gate: F must reproduce the v2 report
gate = {}
f_rows = {r["pos"]: r for r in results["F_final"]["per_pos"]}
for pos, zv in V2_GATE.items():
    got = f_rows[pos]["z"]
    gate[pos] = {"v2_reported": zv, "v2b_rerun": got, "match": abs(got - zv) <= GATE_TOL}
gate_pass = all(v["match"] for v in gate.values())

focus = {}
for pos in ("adj", "noun", "num", "verb"):
    focus[pos] = {tag: {"z": {r["pos"]: r for r in res["per_pos"]}[pos]["z"],
                        "movable_frac": {r["pos"]: r for r in res["per_pos"]}[pos]["movable_frac"]}
                  for tag, res in results.items()}

report = {
    "artifact": "designation-read-v2b-prefixnull",
    "version": "1.0-closedform",
    "question": "is the v2 residual (adj +20z et al.) initial-morphology mass?",
    "n_labeled": int(len(labeled)),
    "consistency_gate_v2_reproduction": {"pass": bool(gate_pass), "detail": gate},
    "focus_classes": focus,
    "caveat": ("beyond edge trigrams != beyond all morphology (infixes/templates/"
               "compounds uncontrolled); single-class strata cannot move — "
               "immobility is the morphology answer, and J z-scores are only "
               "meaningful where movable_frac stays adequate"),
    "nulls": results,
}
out = EXCH / "designation-read-v2b-report.json"
out.write_text(json.dumps(report, indent=1))
print("gate(F reproduces v2):", "PASS" if gate_pass else "FAIL", json.dumps(gate))
print(json.dumps({"focus_classes": focus}, indent=1))
print("strata:", {k: (v["n_strata"], v["movable_frac_overall"]) for k, v in results.items()})
print("wrote", out)
