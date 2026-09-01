#!/usr/bin/env python3
"""persistence_criterion_v0 — stability = persistence across ticks (item 2).

Plan line: data-plan §6 (checkpoints/tick-states; the stability read is the
tau/dilation instrument) + §2 (what closes persistently becomes an entry).
Closes write-back v0's named caveat: "stability = p90 det on ONE snapshot
(v0 proxy; real = persistence across ticks)". This IS the real criterion,
computed from silas's corpus-pour-v0 tick-state emission (90 snapshots,
9 docs, every 20 ticks) — P's organic rule made operational:
"resolves-persistently -> closes -> entry; can't-resolve -> opens -> flag."

A byte pair CLOSES over the observation window (last W snapshots) iff:
  1. bonded at every snapshot in the window (no breaks),
  2. identity (both nibble codes) constant across the window,
  3. det not falling across the window (final >= first - EPS).
Everything else stays OPEN, with a signature (broken / identity-oscillating /
dissolving) — open pairs are the flag population, never entries.

No thresholds on det LEVEL anywhere: the criterion reads only persistence.
The old proxy is re-scored against it (agreement + both error directions),
the tau dilation read is taken (closed vs open local time), and the Moby
condensation types are re-derived under the real criterion and written beside
the proxy rows in engine.condensations_v0 (same loop, new provenance) with
resolve-before-mint via the item-3 method (all types resolve to byte codes).

Run: ~/engine/venv/bin/python persistence_criterion_v0.py
"""
import json
import os
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS = os.path.join(HERE, "..", "kernel", "corpus-pour-v0")
WINDOW = 5        # last 5 snapshots = ticks 120..200
EPS = 0.005       # det "not falling" tolerance (f32 settle jitter)


def analyze(doc_dir):
    idx = json.load(open(os.path.join(doc_dir, "states-index.json")))
    snaps = idx["snapshots"][-WINDOW:]
    ticks = [s["tick"] for s in snaps]
    det_w, code_w, bond_w, tau_last = [], [], [], None
    for s in snaps:
        z = np.load(os.path.join(CORPUS, "..", s["file"])
                    if not os.path.isabs(s["file"]) else s["file"])
        det_w.append(z["det"].reshape(-1, 2).mean(axis=1))
        code_w.append(z["code"].reshape(-1, 2))
        bond_w.append(z["bonded"])
        tau_last = z["tau_pair"]
    det_w = np.stack(det_w)            # (W, npairs)
    code_w = np.stack(code_w)          # (W, npairs, 2)
    bond_w = np.stack(bond_w)          # (W, npairs)

    bonded_all = bond_w.all(axis=0)
    id_const = (code_w == code_w[0]).all(axis=(0, 2))
    not_falling = det_w[-1] >= det_w[0] - EPS
    closed = bonded_all & id_const & not_falling
    sig = {"broken": int((~bonded_all).sum()),
           "identity_oscillating": int((bonded_all & ~id_const).sum()),
           "dissolving": int((bonded_all & id_const & ~not_falling).sum())}

    # old proxy on the final snapshot only
    pd_final = det_w[-1]
    thr = float(np.quantile(pd_final[bond_w[-1]], 0.90))
    proxy = bond_w[-1] & (pd_final >= thr)

    return {
        "window_ticks": ticks, "n_pairs": int(closed.size),
        "closed": closed, "proxy": proxy, "pd_final": pd_final,
        "tau": tau_last, "signatures": sig,
        "closed_frac": float(closed.mean()),
        "proxy_in_closed": float(closed[proxy].mean()),
        "tau_closed_mean": float(tau_last[closed].mean()),
        "tau_open_mean": float(tau_last[~closed].mean()) if (~closed).any() else None,
    }


def main():
    t0 = time.perf_counter()
    rep = {"artifact": "persistence-criterion-v0 (stability = persistence across ticks)",
           "plan_line": "data-plan §6 tick-states/tau instrument + §2 "
                        "closes-persistently-becomes-entry; closes write-back v0 proxy caveat",
           "criterion": {"window": f"last {WINDOW} snapshots (ticks 120-200)",
                         "closed_iff": ["bonded at every snapshot (no breaks)",
                                        "identity constant (both nibbles)",
                                        f"det not falling (final >= first - {EPS})"],
                         "no_det_level_threshold": True},
           "docs": {}, "moby": {}}
    per_doc = {}
    for d in range(1, 10):
        doc_dir = os.path.join(CORPUS, f"states-doc{d}")
        a = analyze(doc_dir)
        per_doc[d] = a
        rep["docs"][f"doc{d}"] = {
            "n_pairs": a["n_pairs"], "closed_frac": round(a["closed_frac"], 5),
            "open_signatures": a["signatures"],
            "proxy_stable_actually_persistent": round(a["proxy_in_closed"], 5),
            "tau_mean_closed_vs_open": [round(a["tau_closed_mean"], 2),
                                        round(a["tau_open_mean"], 2)
                                        if a["tau_open_mean"] is not None else None]}

    # Moby (doc 8): condensation types under the REAL criterion vs the proxy
    a = per_doc[8]
    seed = np.load(os.path.join(CORPUS, "states-doc8", "seed.npz"))["leaf_val"]
    byte_val = (seed.reshape(-1, 2)[:, 0].astype(np.int32) * 16
                + seed.reshape(-1, 2)[:, 1])
    types_closed = np.unique(byte_val[a["closed"]])
    types_proxy = np.unique(byte_val[a["proxy"]])
    rep["moby"] = {
        "closed_pairs": int(a["closed"].sum()), "proxy_pairs": int(a["proxy"].sum()),
        "condensation_types_persistence": int(types_closed.size),
        "condensation_types_proxy": int(types_proxy.size),
        "types_only_in_proxy": [int(x) for x in np.setdiff1d(types_proxy, types_closed)],
        "types_only_in_persistence": [int(x) for x in np.setdiff1d(types_closed, types_proxy)],
    }
    # where does the OPEN (dissolving) population live? byte-class read —
    # silas's healing physics predicted both-light concentration; check the
    # slow-dissolve population against the same classes.
    both_light = np.array([bin(b >> 4).count("1") <= 1 and bin(b & 15).count("1") <= 1
                           for b in range(256)])
    open_mask = ~a["closed"]
    rep["moby"]["open_population"] = {
        "open_pairs": int(open_mask.sum()),
        "both_light_frac_open": round(float(both_light[byte_val[open_mask]].mean()), 4),
        "both_light_frac_closed": round(float(both_light[byte_val[a["closed"]]].mean()), 4),
        "top_open_bytes": [
            {"byte": int(b), "glyph": chr(b) if 32 <= b < 127 else hex(b), "n_open": int(n)}
            for b, n in sorted(zip(*np.unique(byte_val[open_mask], return_counts=True)),
                               key=lambda x: -x[1])[:8]],
    }

    # write the persistence-criterion condensation types BESIDE the proxy rows
    # (same loop, new provenance; resolve-before-mint: byte values -> R0)
    import psycopg2
    conn = psycopg2.connect(host="192.168.68.60", port=5435, user="hcp",
                            password=os.environ.get("HCP_PW", "hcp_dev"),
                            dbname="hcp_english")
    conn.autocommit = True
    cur = conn.cursor()
    prov = ("system-derived corpus-pour-v0 doc8 (repo 13768d1); "
            "criterion=persistence-v0 window-ticks-120-200")
    cb = byte_val[a["closed"]]
    pd_closed = a["pd_final"][a["closed"]]
    cur.execute("DELETE FROM engine.condensations_v0 WHERE provenance = %s", (prov,))
    for b in types_closed:
        m = cb == b
        cur.execute("""INSERT INTO engine.condensations_v0
                       (resolves_to, byte_val, glyph, n_instances, mean_det,
                        minted, provenance)
                       VALUES (%s,%s,%s,%s,%s,false,%s)""",
                    ([0, 0, 0, 0, int(b)], int(b),
                     chr(b) if 32 <= b < 127 else f"\\x{b:02x}",
                     int(m.sum()), float(pd_closed[m].mean()), prov))
    cur.execute("SELECT count(*) FROM engine.condensations_v0 WHERE provenance=%s", (prov,))
    rep["moby"]["db_write"] = {
        "rows": cur.fetchone()[0],
        "table": "engine.condensations_v0 (beside proxy rows, new provenance)",
        "minted": 0, "note": "all types resolve to given byte codes (R0)"}
    conn.close()

    rep["wall_s"] = round(time.perf_counter() - t0, 2)
    return rep, per_doc, byte_val


if __name__ == "__main__":
    rep, per_doc, byte_val = main()
    out = os.path.join(HERE, "persistence-criterion-v0-report.json")
    json.dump(rep, open(out, "w"), indent=1)
    print(json.dumps(rep, indent=1))
