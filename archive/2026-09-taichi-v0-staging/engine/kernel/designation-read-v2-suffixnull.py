#!/usr/bin/env python3
"""designation-read-v2 — suffix-stratified null (P GO, Zulip 1288/1289; rewrite v2.1).

Question: does the POS structure found by v1 survive when the null controls
for MORPHOLOGY (final trigram + length), not just length? If z-scores collapse,
v1's signal was carried by class-typical endings (aggregate-by-spelling-geometry
— new languages bootstrap free). If they survive, there is designation structure
beyond the ending (aggregator needs designation chains as first-class objects).

v2.1 rewrite (first attempt timed out at B=1000 lexsort-per-iteration):
the null's moments are CLOSED-FORM — within-stratum label shuffle makes each
group's stratum draw a simple random sample without replacement, so
  E[S_gs]   = n_gs * m_s
  Var[S_gs] = n_gs * s2_s * (N_s - n_gs) / (N_s - 1)
independent across strata. z is exact (B = infinity); a B=200 empirical pass
(single-key argsort, random tiebreak) cross-checks the normal percentile.

Caveat carried on the result: "beyond final trigram" != "beyond all morphology"
(prefixes/infixes/templates uncontrolled). Strata where one class owns the
ending (e.g. -ly ~ adv) cannot move under shuffle — that non-movement IS the
morphology answer, not an instrument failure.

Words' bytes come from state `code` (valid: code_drift_cells=0 => code == given).
Planner, 2026-08-31.
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
SEED = 178

z = np.load(EXCH / "snode-chains-v0.npz")
off = z["word_char__off"].astype(np.int64)
pos_off = z["pos__off"].astype(np.int64)
pos_members = z["pos__members"].astype(np.int64)
n_word = len(off) - 1

st = np.load(EXCH / "pour-state-v031-acceptance.npz")
det = st["det"].astype(np.float64).ravel()
code = st["code"].astype(np.int64).ravel()

# per-word mean det (leaves [2*off[w], 2*off[w+1]))
cs = np.concatenate([[0.0], np.cumsum(det)])
leaf_lo, leaf_hi = 2 * off[:-1], 2 * off[1:]
word_mean = (cs[leaf_hi] - cs[leaf_lo]) / (leaf_hi - leaf_lo)
word_len = (off[1:] - off[:-1]).astype(np.int64)

# bytes from nibble code (hi at even leaf, lo at odd); final trigram key
byte_vals = code[0::2] * 16 + code[1::2]          # (n_byte_instances,)
b_lo, b_hi = off[:-1], off[1:]                     # word's byte range == char range
last1 = byte_vals[b_hi - 1]
last2 = np.where(word_len >= 2, byte_vals[np.maximum(b_hi - 2, b_lo)], 256)
last3 = np.where(word_len >= 3, byte_vals[np.maximum(b_hi - 3, b_lo)], 256)
trigram = (last3 * 257 + last2) * 257 + last1
strata_key = trigram * 21 + np.clip(word_len, 1, 20)   # (final-trigram, len-bucket)

label = np.full(n_word, -1, dtype=np.int64)
for g in range(len(pos_off) - 1):
    label[pos_members[pos_off[g]:pos_off[g + 1]]] = g
labeled = np.flatnonzero(label >= 0)
n_groups = len(pos_off) - 1

# work only over labeled words
wm = word_mean[labeled]
lab = label[labeled]
_, stratum = np.unique(strata_key[labeled], return_inverse=True)
n_strata = int(stratum.max()) + 1

counts = np.bincount(lab, minlength=n_groups)
obs_mean = np.bincount(lab, weights=wm, minlength=n_groups) / np.maximum(counts, 1)

# ---- closed-form null moments ----
N_s = np.bincount(stratum, minlength=n_strata).astype(np.float64)
sum_s = np.bincount(stratum, weights=wm, minlength=n_strata)
sumsq_s = np.bincount(stratum, weights=wm * wm, minlength=n_strata)
m_s = sum_s / N_s
s2_s = np.maximum(sumsq_s / N_s - m_s * m_s, 0.0)      # population variance

n_gs = np.bincount(stratum * n_groups + lab,
                   minlength=n_strata * n_groups).reshape(n_strata, n_groups).astype(np.float64)
fpc = (N_s[:, None] - n_gs) / np.maximum(N_s[:, None] - 1.0, 1.0)
mu0 = (n_gs * m_s[:, None]).sum(0) / np.maximum(counts, 1)
var0 = (n_gs * s2_s[:, None] * fpc).sum(0) / np.maximum(counts, 1) ** 2
sd0 = np.sqrt(var0)
zscore = (obs_mean - mu0) / np.where(sd0 > 0, sd0, np.inf)
pct_norm = np.array([0.5 * (1.0 + math.erf(v / math.sqrt(2.0))) if np.isfinite(v) else 0.5
                     for v in zscore])

# ---- empirical cross-check (B=200): shuffle wm within strata, labels fixed ----
base_order = np.argsort(stratum, kind="stable")
strat_f = stratum.astype(np.float64)
rng = np.random.default_rng(SEED)
null_means = np.empty((B_EMP, n_groups))
wm_null = np.empty_like(wm)
for b in range(B_EMP):
    order = np.argsort(strat_f + rng.random(len(wm)))  # stratum-major, random within
    wm_null[base_order] = wm[order]
    null_means[b] = np.bincount(lab, weights=wm_null, minlength=n_groups) / np.maximum(counts, 1)
pct_emp = (null_means < obs_mean).mean(0)
emp_mu_maxdiff = float(np.abs(null_means.mean(0) - mu0).max())

# movability: fraction of words whose stratum contains >1 distinct class
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
      "pctile_normal": round(float(pct_norm[g]), 4),
      "pctile_emp200": round(float(pct_emp[g]), 4)}
     for g in range(n_groups) if counts[g] > 0),
    key=lambda r: -abs(r["z"]))

report = {
    "artifact": "designation-read-v2-suffixnull",
    "version": "2.1-closedform",
    "null": "shuffle within (final-trigram, len-bucket) strata",
    "estimator": "closed-form SRSWOR moments per stratum (exact z); B=200 empirical cross-check",
    "n_labeled": int(len(lab)), "n_strata": n_strata, "B_emp": B_EMP,
    "emp_vs_exact_mu_maxdiff": emp_mu_maxdiff,
    "caveat": "beyond-final-trigram, not beyond-all-morphology; single-class strata cannot move (that immobility is the morphology answer)",
    "per_pos": rows,
}
out = EXCH / "designation-read-v2-report.json"
out.write_text(json.dumps(report, indent=1))
for r in rows:
    print(f"{r['pos']:>12} n={r['n_words']:>6} movable={r['movable_frac']:.2f} "
          f"Δ={r['delta']:+.5f} z={r['z']:+8.2f} pct_n={r['pctile_normal']:.3f} "
          f"pct_e={r['pctile_emp200']:.3f}")
print(f"strata={n_strata}  emp-vs-exact mu maxdiff={emp_mu_maxdiff:.2e}  report: {out}")
