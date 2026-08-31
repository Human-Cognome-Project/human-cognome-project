#!/usr/bin/env python3
"""designation-read-v1 — P's question (Zulip 1281): is the Kaikki field showing
internal organization along the CROSS-CUTTING DESIGNATIONS (POS), beyond what
raw recurrence/composition explains?

Substrate: v0.3.1 acceptance state (det is the only varying observable —
bond_survival=1.0, drift=0, tau=0) + snode-chains-v0 (pos chains pass through
the seed unchanged; word w owns leaves [2*off[w], 2*off[w+1])).

Method: per-word mean det, conditioned on POS, tested against a
LENGTH-STRATIFIED shuffLE NULL (B=1000): POS labels permuted only within
word-length strata, so length/composition effects cancel and only
designation-linked structure can produce signal. Report: per-POS delta vs null
(z, percentile) + global between-group variance test.

Planner, 2026-08-31. Run: python3 designation-read-v1.py [state.npz]
"""
import json
import sys
from pathlib import Path

import numpy as np

EXCH = Path.home() / "shared-brain" / "exchange"
CHAINS = EXCH / "snode-chains-v0.npz"
STATE = Path(sys.argv[1]) if len(sys.argv) > 1 else EXCH / "pour-state-v031-acceptance.npz"
POS_LABELS = ['adj', 'adv', 'article', 'character', 'conj', 'contraction', 'det',
              'infix', 'intj', 'name', 'noun', 'num', 'particle', 'phrase',
              'postp', 'prefix', 'prep', 'prep_phrase', 'pron', 'suffix',
              'symbol', 'verb']  # snode-chains-v0.manifest.json chains[pos].group_labels
B = 1000
SEED = 178  # for the WP that taught us to distinguish measured from couldn't-look

z = np.load(CHAINS)
off = z["word_char__off"].astype(np.int64)          # (n_word+1,) char-instance offsets
pos_off = z["pos__off"].astype(np.int64)
pos_members = z["pos__members"].astype(np.int64)
n_word = len(off) - 1

st = np.load(STATE)
det = st["det"].astype(np.float64).ravel()
n_leaves = det.size
assert n_leaves == 2 * off[-1], f"leaf count {n_leaves} != 2*char_instances {2*off[-1]}"

# per-word mean det over its contiguous leaf range [2*off[w], 2*off[w+1])
cs = np.concatenate([[0.0], np.cumsum(det)])
leaf_lo, leaf_hi = 2 * off[:-1], 2 * off[1:]
word_nleaf = (leaf_hi - leaf_lo).astype(np.float64)
word_mean = (cs[leaf_hi] - cs[leaf_lo]) / word_nleaf
word_min = np.minimum.reduceat(det, leaf_lo)        # segment mins (ranges contiguous)
word_len = (off[1:] - off[:-1]).astype(np.int64)    # chars

# POS label per word (-1 = unlabeled)
label = np.full(n_word, -1, dtype=np.int64)
for g in range(len(pos_off) - 1):
    label[pos_members[pos_off[g]:pos_off[g + 1]]] = g
labeled = label >= 0
n_groups = len(pos_off) - 1
assert n_groups == len(POS_LABELS), (n_groups, len(POS_LABELS))

def group_means(lbl):
    s = np.bincount(lbl[labeled], weights=word_mean[labeled], minlength=n_groups)
    c = np.bincount(lbl[labeled], minlength=n_groups)
    return s / np.maximum(c, 1), c

obs_mean, counts = group_means(label)
obs_var = np.var(obs_mean[counts > 0])

# length strata: exact length 1..19, 20+ pooled
strata = np.clip(word_len, 1, 20)
rng = np.random.default_rng(SEED)
null_means = np.empty((B, n_groups))
null_var = np.empty(B)
lbl_shuf = label.copy()
stratum_idx = [np.flatnonzero(labeled & (strata == sv)) for sv in np.unique(strata[labeled])]
for b in range(B):
    for idx in stratum_idx:
        lbl_shuf[idx] = lbl_shuf[idx[rng.permutation(len(idx))]]
    m, _ = group_means(lbl_shuf)
    null_means[b] = m
    null_var[b] = np.var(m[counts > 0])

mu0, sd0 = null_means.mean(0), null_means.std(0)
zscore = (obs_mean - mu0) / np.where(sd0 > 0, sd0, np.inf)
pct = (null_means < obs_mean).mean(0)
var_pct = float((null_var < obs_var).mean())

rows = sorted(
    ({"pos": POS_LABELS[g], "n_words": int(counts[g]),
      "mean_det": round(float(obs_mean[g]), 6),
      "null_mean": round(float(mu0[g]), 6),
      "delta": round(float(obs_mean[g] - mu0[g]), 6),
      "z": round(float(zscore[g]), 2), "pctile_vs_null": round(float(pct[g]), 4)}
     for g in range(n_groups) if counts[g] > 0),
    key=lambda r: -abs(r["z"]))

report = {
    "artifact": "designation-read-v1",
    "question": "field organization along POS designations beyond length-stratified recurrence",
    "state": STATE.name, "n_words": int(n_word), "n_labeled": int(labeled.sum()),
    "n_leaves": int(n_leaves), "B": B,
    "global": {"between_group_var": float(obs_var),
               "null_var_mean": float(null_var.mean()),
               "var_pctile_vs_null": var_pct,
               "reading": ("ORGANIZED along POS beyond length-null" if var_pct > 0.99
                           else "no POS organization beyond length-null" if var_pct < 0.95
                           else "marginal")},
    "per_pos": rows,
    "word_det_global": {"mean": float(word_mean.mean()), "min": float(word_min.min())},
}
out = EXCH / "designation-read-v1-report.json"
out.write_text(json.dumps(report, indent=1))
print(json.dumps(report["global"], indent=1))
for r in rows:
    print(f"{r['pos']:>12} n={r['n_words']:>6} det={r['mean_det']:.4f} "
          f"Δvs-null={r['delta']:+.5f} z={r['z']:+6.2f} pct={r['pctile_vs_null']:.3f}")
print(f"report: {out}")
