#!/usr/bin/env python3
"""Control harness for the HCP field physics engine.

The harness controls what the physics engine is asked to do: construct/load the
population, choose engine implementation and couplings, advance runs, checkpoint
state, observe engine state, and emit the engine's condensation/mint readout.

This is the control surface intended for eventual analyst use. It does not define
the field dynamics themselves.
"""
import json
import os
import time

import numpy as np

from field_engine_load_v0 import load_at_declared_grain
import field_engine_v0 as v0
from field_engine_physics import Pool, Oracle, make_kernel, cells_of, occupied_cells


def resolve(parent, p):
    while parent[p] != p:
        p = parent[p]
    return p


def mint_readout(pool, particles, info):
    """What the engine condensed: each surviving entry with count>1 and the
    range it covers. Read from the engine's own parent map; the identity
    labels are attached only so a reviewer can check them, the engine never
    read them."""
    members = {}
    for p in range(pool.n):
        r = resolve(pool.parent, p)
        members.setdefault(int(r), []).append(p)
    entries = []
    for r, ms in members.items():
        if len(ms) > 1:
            cfg = "".join(f"{v:X}" for v in particles[r]["config"])
            entries.append({
                "entry": r, "config": cfg, "count": int(pool.count[r]),
                "covers": [{"token": ".".join(particles[m]["token"]),
                            "encoding": particles[m]["encoding"],
                            "identity": list(particles[m]["identity"])} for m in ms],
            })
    entries.sort(key=lambda e: e["entry"])
    return {"artifact": "field-engine-mint", "grain": info["grain"],
            "entries_live": int(pool.alive.sum()), "n_particles": pool.n,
            "entries_covering_more_than_one": len(entries), "entries": entries}


def run(a):
    particles, info = load_at_declared_grain(a.grain)
    pool = Pool(particles)
    n, g = pool.n, a.grid
    start, hist = 0, []
    if os.path.exists(a.state):
        st = np.load(a.state, allow_pickle=True)
        pos, start, hist = st["pos"], int(st["tick"]), list(st["hist"])
        assert len(pos) == n
        pool.load_state(st)
    else:
        pos = v0.inject(n, a.box)

    pull, excl, absorb = not a.no_identity_pull, not a.no_exclusion, not a.no_absorb
    if a.arch == "numpy":
        eng = Oracle(pool, g, a.box, a.relax_iters, a.step, pull, excl, absorb, a.periodic,
                     not a.no_cap)
    else:
        eng = make_kernel(pool, g, a.box, a.relax_iters, a.step, pull, excl, absorb, a.arch,
                          a.periodic, not a.no_cap)
        eng.pos.from_numpy(pos)

    t0 = time.time()
    still = 0
    for t in range(start, a.ticks):
        if a.arch == "numpy":
            pos, n_abs = eng.tick(pos)
        else:
            n_abs = eng.tick()
        still = still + 1 if n_abs == 0 else 0
        last = (t + 1 == a.ticks) or (a.until_still and still >= a.until_still)
        if (t + 1) % a.chunk == 0 or last:
            if a.arch != "numpy":
                pos = eng.pos.to_numpy()
                eng.sync_pool()
            idx = cells_of(pos, a.box, g)
            obs = {"tick": t + 1, "entries_live": int(pool.alive.sum()),
                   "occupied_cells": occupied_cells(idx, pool.alive, g),
                   "elapsed_s": round(time.time() - t0, 1)}
            hist.append(obs)
            np.savez(a.state, pos=pos, tick=t + 1, alive=pool.alive,
                     count=pool.count, parent=pool.parent,
                     hist=np.array(hist, dtype=object))
            print(json.dumps(obs), flush=True)
        if last:
            break

    mint = mint_readout(pool, particles, info)
    with open(a.mint, "w") as fh:
        json.dump(mint, fh, indent=1)
    rep = {"artifact": "field-engine", "substrate": "field_engine_v0 (validated)",
           "load": info, "arch": a.arch, "grid": g, "box": a.box,
           "relax_iters": a.relax_iters, "step": a.step,
           "boundary": "periodic (v0 regression)" if a.periodic else "open (P field_model.py)",
           "couplings": {"identity_pull": pull, "exclusion": excl, "condense": absorb,
                         "one_cell_per_tick": not a.no_cap},
           "deterministic": True,
           "entries_first": hist[0]["entries_live"] if hist else None,
           "entries_last": hist[-1]["entries_live"] if hist else None,
           "occupied_cells_last": hist[-1]["occupied_cells"] if hist else None,
           "entries_covering_more_than_one": mint["entries_covering_more_than_one"],
           "history": hist}
    with open(a.report, "w") as fh:
        json.dump(rep, fh, indent=1, default=str)
    print(json.dumps({k: rep[k] for k in ("entries_first", "entries_last",
                                          "occupied_cells_last",
                                          "entries_covering_more_than_one",
                                          "couplings")}, indent=1))
    return 0


# ----------------------------------------------------------------------------
# --check: (A) kernel with every coupling off == field_engine_v0 oracle;
#          (B) numpy oracle == kernel with every coupling on;
#          (C) numpy oracle with every coupling off == field_engine_v0.
# ----------------------------------------------------------------------------
