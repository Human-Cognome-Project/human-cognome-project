#!/usr/bin/env python3
"""field_engine_review — EXECUTED-REVIEW tool, outside the engine.

The engine never reads identity labels. This script does, on purpose, after
the fact, over a saved state (field-engine-state.npz): it is the control that
asks whether what the engine condensed and where it put things agree with the
labels the data carries. Silas's lane (P: gate every gathering result through
the disable-coupling control before reporting).

Reads (no physics, no rates):
  purity      live entries whose folded members all share one identity label
  twin_dist   mean periodic distance between LIVE same-configuration pairs vs
              the same number of random live pairs (twins closer than chance
              = the coupling is directed, not luck)
  cell_purity fraction of occupied cells holding exactly one configuration
Run:  python3 field/field_engine_review.py [--state PATH] [--grain endpoint]
"""
import argparse
import json
import os
import sys
from collections import defaultdict

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from field_engine_load_v0 import load_at_declared_grain   # noqa: E402
from field_engine import Pool, resolve, cells_of           # noqa: E402


def pdist(a, b, box):
    d = np.abs(a - b)
    d = np.minimum(d, box - d)
    return np.sqrt((d * d).sum(1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--state", default=os.path.join(HERE, "field-engine-state.npz"))
    ap.add_argument("--grain", default=None)
    ap.add_argument("--box", type=float, default=100.0)
    ap.add_argument("--grid", type=int, default=48)
    a = ap.parse_args()
    particles, info = load_at_declared_grain(a.grain)
    pool = Pool(particles)
    st = np.load(a.state, allow_pickle=True)
    pool.load_state(st)
    pos = st["pos"]
    n = pool.n
    ident = [tuple(p["identity"]) for p in particles]
    cfg = [tuple(p["config"]) for p in particles]

    # purity of folded entries
    members = defaultdict(list)
    for p in range(n):
        members[resolve(pool.parent, p)].append(p)
    folded = {r: ms for r, ms in members.items() if len(ms) > 1}
    mixed = sum(1 for ms in folded.values() if len({ident[m] for m in ms}) > 1)

    # twins still apart: live particles sharing a configuration
    live = np.nonzero(pool.alive)[0]
    by_cfg = defaultdict(list)
    for p in live:
        by_cfg[cfg[p]].append(p)
    pairs = []
    for ms in by_cfg.values():
        if len(ms) > 1:
            for x in range(len(ms) - 1):
                pairs.append((ms[x], ms[x + 1]))
    pairs = np.array(pairs) if pairs else np.zeros((0, 2), dtype=int)
    twin_d = float(pdist(pos[pairs[:, 0]], pos[pairs[:, 1]], a.box).mean()) if len(pairs) else None
    rng = np.random.default_rng(0)  # review-side only; the engine has no rng
    ra, rb = rng.choice(live, len(pairs)), rng.choice(live, len(pairs))
    rand_d = float(pdist(pos[ra], pos[rb], a.box).mean()) if len(pairs) else None

    # cell purity
    idx = cells_of(pos[live], a.box, a.grid)
    flat = (idx[:, 0] * a.grid + idx[:, 1]) * a.grid + idx[:, 2]
    cell_cfgs = defaultdict(set)
    for f, p in zip(flat, live):
        cell_cfgs[int(f)].add(cfg[p])
    pure = sum(1 for s in cell_cfgs.values() if len(s) == 1)

    out = {"artifact": "field-engine-review", "state": os.path.basename(a.state),
           "tick": int(st["tick"]), "grain": info["grain"],
           "entries_live": int(pool.alive.sum()),
           "folded_entries": len(folded), "folded_mixed_identity": mixed,
           "same_config_pairs_still_apart": int(len(pairs)),
           "twin_pair_mean_dist": None if twin_d is None else round(twin_d, 3),
           "random_pair_mean_dist": None if rand_d is None else round(rand_d, 3),
           "occupied_cells": len(cell_cfgs),
           "cells_single_configuration": pure,
           "cell_purity": round(pure / max(1, len(cell_cfgs)), 4)}
    print(json.dumps(out, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
