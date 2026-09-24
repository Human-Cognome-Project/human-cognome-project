#!/usr/bin/env python3
"""POS-carrier geometry count — no anneal, pure host-side read.

Question (planner's designation read v1 + silas sign-flip hypothesis): is the
per-POS millidet det structure carried by MORPHOLOGY-as-physics — i.e. does
class-typical byte composition (WEIGHT of given nibbles) plus standing-node
flank geometry reproduce the per-class hot/cold pattern (adv +25.4z HOT,
adj -26.8z COLD, ...) without any appeal to designation-beyond-spelling?

Per POS class (22 groups, ids only — planner holds the id->name map):
  mean_det        — from pour-state-v031-acceptance (the field's actual read)
  mean_weight     — mean WEIGHT[given] per cell (composition profile; WEIGHT =
                    popcount+1, the self-consolidation strength wsus pulls with)
  frac_eqflank    — fraction of interior cells whose lattice flanks carry EQUAL
                    codes (node-capable geometry; everything is locked in the
                    seeded pour, so geometry ~= standing node)
  frac_selfnode   — subset where the node code == the cell's OWN given (runs of
                    three equal nibbles): consolidation aligned with identity
  frac_foreignnode— node code != own given: the wall masks it; restricted
                    radiation is code-filtered -> the starve channel
  mean_lo_weight  — lo-cell-only composition (the drift-relevant subset)

Correlation report at the end: class-level Pearson r of mean_det against each
candidate carrier, word-count-weighted. If composition/geometry reproduce the
det ordering, morphology-carrier is CONFIRMED with mechanism named; the
residual (classes off the fit line) is the designation-beyond-morphology
candidate list for planner's suffix-stratified v2.

Run: .venv-kernel/bin/python pos-carrier-count.py   (writes
     pos-carrier-count-v1.json + drops a copy in shared exchange)
"""
import json

import numpy as np

SEED = "instance-seed-v0.npz"
CHAINS = "/home/silas/shared-brain/exchange/snode-chains-v0.npz"
STATE = "pour-state-v031-acceptance.npz"
OUT = "pos-carrier-count-v1.json"
EXCHANGE_OUT = "/home/silas/shared-brain/exchange/pos-carrier-count-v1.json"

WEIGHT = (1.0 + np.array([bin(c).count("1") for c in range(16)])).astype(np.float64)

s = np.load(SEED)
leaf_val = s["leaf_val"].astype(np.int32)
w_off = s["word_charinst_off"]
cb_off = s["charinst_byteinst_off"]
z = np.load(CHAINS)
pos_off, pos_members = z["pos__off"], z["pos__members"]
st = np.load(STATE)
det = st["det"].astype(np.float64)

n = len(leaf_val)
nw = len(w_off) - 1
# per-word leaf ranges (contiguous, depth-first)
leaf_start = 2 * cb_off[w_off[:-1]]
leaf_stop = 2 * cb_off[w_off[1:]]
lens = (leaf_stop - leaf_start).astype(np.int64)

# per-cell carriers (global lattice)
wt = WEIGHT[leaf_val]
eqflank = np.zeros(n, bool)
eqflank[1:-1] = leaf_val[:-2] == leaf_val[2:]
selfnode = np.zeros(n, bool)
selfnode[1:-1] = eqflank[1:-1] & (leaf_val[:-2] == leaf_val[1:-1])
is_lo = np.zeros(n, bool)
is_lo[1::2] = True

# per-word sums via reduceat (ranges are contiguous and ordered)
def wsum(x):
    return np.add.reduceat(x.astype(np.float64), leaf_start)[:nw] if False else \
        np.add.reduceat(np.asarray(x, np.float64), leaf_start)

sum_det = np.add.reduceat(det, leaf_start)
sum_wt = np.add.reduceat(wt, leaf_start)
sum_eq = np.add.reduceat(eqflank.astype(np.float64), leaf_start)
sum_self = np.add.reduceat(selfnode.astype(np.float64), leaf_start)
sum_lo_wt = np.add.reduceat((wt * is_lo), leaf_start)
lo_cnt = lens // 2

rows = []
for g in range(len(pos_off) - 1):
    members = pos_members[pos_off[g]:pos_off[g + 1]]
    if len(members) == 0:
        continue
    m = members
    cells = float(lens[m].sum())
    row = {
        "pos_id": int(g),
        "words": int(len(m)),
        "cells": int(cells),
        "mean_det": float(sum_det[m].sum() / cells),
        "mean_weight": float(sum_wt[m].sum() / cells),
        "mean_lo_weight": float(sum_lo_wt[m].sum() / lo_cnt[m].sum()),
        "frac_eqflank": float(sum_eq[m].sum() / cells),
        "frac_selfnode": float(sum_self[m].sum() / cells),
    }
    row["frac_foreignnode"] = row["frac_eqflank"] - row["frac_selfnode"]
    rows.append(row)

rows.sort(key=lambda r: -r["words"])
glob_det = float(det.mean())
print(f"global mean det {glob_det:.6f}; {len(rows)} POS groups\n")
hdr = f"{'id':>3} {'words':>7} {'Δdet(milli)':>11} {'meanW':>6} {'loW':>6} {'eqflank%':>8} {'self%':>6} {'foreign%':>8}"
print(hdr)
for r in rows:
    print(f"{r['pos_id']:>3} {r['words']:>7} {(r['mean_det']-glob_det)*1000:>11.4f} "
          f"{r['mean_weight']:>6.3f} {r['mean_lo_weight']:>6.3f} "
          f"{r['frac_eqflank']*100:>8.3f} {r['frac_selfnode']*100:>6.3f} "
          f"{r['frac_foreignnode']*100:>8.3f}")

# class-level correlations (word-weighted) of mean_det vs carriers
d = np.array([r["mean_det"] for r in rows])
wts = np.array([r["words"] for r in rows], np.float64)
corr = {}
for key in ("mean_weight", "mean_lo_weight", "frac_eqflank", "frac_selfnode", "frac_foreignnode"):
    x = np.array([r[key] for r in rows])
    mx = np.average(x, weights=wts)
    md = np.average(d, weights=wts)
    cov = np.average((x - mx) * (d - md), weights=wts)
    r_ = cov / np.sqrt(np.average((x - mx) ** 2, weights=wts) *
                       np.average((d - md) ** 2, weights=wts))
    corr[key] = float(r_)
print("\nword-weighted class-level correlation of mean_det with:")
for k, v in corr.items():
    print(f"  {k:>16}: r = {v:+.4f}")

out = {"artifact": "pos-carrier-count-v1", "state": STATE,
       "global_mean_det": glob_det, "groups": rows, "class_level_r": corr,
       "note": ("ids only — planner holds id->name; join against designation-read "
                "z table. If composition/geometry reproduce the det ordering, "
                "morphology-carrier confirmed w/ mechanism (WEIGHT + node geometry); "
                "residual classes = suffix-v2 candidates.")}
for path in (OUT, EXCHANGE_OUT):
    with open(path, "w") as f:
        json.dump(out, f, indent=1)
print(f"\nwritten: {OUT} + {EXCHANGE_OUT}")
