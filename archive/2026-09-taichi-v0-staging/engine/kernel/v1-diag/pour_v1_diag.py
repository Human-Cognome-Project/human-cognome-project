#!/usr/bin/env python3
"""v1 MOBY-DICK (b)-OFF DIAGNOSTIC SLICE — throwaway unmasked measurement rig.

Purpose: prove the v0.2 drift MECHANISM (cross-byte standing-node capture of
light lo-nibbles) by TRANSITION — same physics (v0.2, no wall), same seed law,
but real document adjacency instead of lexicon packing. In the lexicon pour a
word-final lo cell's right neighbour was the NEXT word's first hi nibble
(always 6 or 7 for [a-z]); in running text it is a separator byte whose hi is
NEVER in {6,7} (space 0x20 -> hi 2; every inter-token char in the normalized
corpus is a non-letter, hi in {0,2,3}). A standing node needs equal locked
flank codes, so the capture channel at word boundaries should VANISH while
persisting word-internally where own-hi == next-hi.

PRE-REGISTERED FALSIFIERS (stated before the run):
  F-A  word-final lo cells (right neighbour = separator): drift SUPPRESSED
       (rate ~0; >=0.5% refutes the node mechanism — something else drives it).
  F-B  word-internal lo cells with own_hi == next_hi (node-capable): drift
       PERSISTS (positive control; ~0 here means the lexicon drift was a
       packing artifact of some other kind and the mechanism is unproven).
  F-C  word-internal lo cells with own_hi != next_hi (no node possible): drift
       ~0 (secondary control, same prediction channel as F-A).
  F-D  drifted cells' landing codes == their flank code (6 or 7),
       predominantly (>=95%) — capture, not random walk.

Physics = v0.2 bytes exactly (local binding.py + pour_v02.py are the
pre-v0.3-wall backups; twin proofs run first, comparing the taichi kernel to
the LOCAL v0.2 numpy reference). No given mask, no bonded code-restriction:
(b) is OFF. Seed law unchanged: amp one-hot per nibble leaf, every byte pair
bonded at t=0. f32, T0=0.02, lam=0.995, floor=0.002 — same schedule as the
v0.2 lexicon discovery run for comparability.

Stream: word-instance-stream-v1.npz (doc order, 215,092 positions). Words are
decoded to bytes via the v0 instance artifact offsets; consecutive words are
joined with ONE space byte 0x20. gap_before=1 boundaries (dropped OOV tokens)
still get a space — the real document had a non-letter there too, so class-A
membership is honest; splice positions are counted in the report but not
excluded.

Run:  ../.venv-kernel/bin/python pour_v1_diag.py [--words 50000] [--ticks 200]
      [--out-prefix pour-v1-diag-b-off]
Outputs (EXPLICIT names): <prefix>-report.json, <prefix>-state.npz
"""
import argparse
import json
import time

import numpy as np

import pour_v02 as rig   # v0.2 twin kernel + engine_run (imports LOCAL v0.2 binding)

STREAM_NPZ = "/home/silas/shared-brain/exchange/word-instance-stream-v1.npz"
SEED_NPZ = "../instance-seed-v0.npz"
SEP = 0x20  # hi=2, lo=0


def build_stream_field(n_words):
    z = np.load(STREAM_NPZ)
    ids = z["doc_word__ids"][:n_words]
    gaps = z["doc_word__gap_before"][:n_words]
    s = np.load(SEED_NPZ)
    w_off = s["word_charinst_off"]
    cb_off = s["charinst_byteinst_off"]
    btype = s["byteinst_type"]

    # word w's bytes = btype[cb_off[w_off[w]] : cb_off[w_off[w+1]]]
    b0 = cb_off[w_off[ids]]
    b1 = cb_off[w_off[ids + 1]]
    lens = (b1 - b0).astype(np.int64)

    n_bytes = int(lens.sum()) + (len(ids) - 1)  # one separator between words
    byte_val = np.empty(n_bytes, np.uint8)
    word_of = np.full(n_bytes, -1, np.int64)    # -1 = separator
    is_final = np.zeros(n_bytes, bool)          # last byte of a word
    sep_is_splice = np.zeros(n_bytes, bool)     # separator at a gap boundary
    p = 0
    for i in range(len(ids)):
        if i > 0:
            byte_val[p] = SEP
            sep_is_splice[p] = bool(gaps[i])
            p += 1
        L = lens[i]
        byte_val[p:p + L] = btype[b0[i]:b1[i]]
        word_of[p:p + L] = i
        is_final[p + L - 1] = True
        p += L
    assert p == n_bytes
    return byte_val, word_of, is_final, sep_is_splice, ids


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--words", type=int, default=50000)
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--t0", type=float, default=0.02)
    ap.add_argument("--sample-every", type=int, default=20)
    ap.add_argument("--out-prefix", default="pour-v1-diag-b-off")
    args = ap.parse_args()

    rig.twin_tests()   # v0.2 taichi kernel == v0.2 numpy reference, or die

    byte_val, word_of, is_final, sep_splice, ids = build_stream_field(args.words)
    n_bytes = len(byte_val)
    n = 2 * n_bytes
    leaf_val = np.empty(n, np.uint8)
    leaf_val[0::2] = byte_val >> 4
    leaf_val[1::2] = byte_val & 15
    print(f"v1 diag pour: {args.words:,} word positions -> {n_bytes:,} bytes "
          f"({n:,} leaves), (b)-OFF v0.2 physics, {args.ticks} ticks, "
          f"T0={args.t0}", flush=True)

    amp = np.zeros((n, 16), dtype=np.float32)
    amp[np.arange(n), leaf_val] = 1.0
    bonded = np.ones(n_bytes, dtype=bool)

    samples = []

    def on_sample(s):
        samples.append(s)
        print(f"  t={s['tick']:>4} T={s['T']:.4f} "
              f"bonds={s['bond_survival'] * 100:.3f}% "
              f"det μ={s['mean_det']:.4f} min={s['min_det']:.3f} "
              f"foreign_locks={s['foreign_locks']} "
              f"{s['ticks_per_sec']:.2f} t/s", flush=True)

    t0 = time.time()
    amp, bonded, tau_pair, det, code, totals = rig.engine_run(
        amp, bonded, args.ticks, T0=args.t0, sample_every=args.sample_every,
        on_sample=on_sample, true_code=leaf_val.astype(np.int32))
    wall = time.time() - t0

    # ── diagnostic classification (the point of this run) ──────────────────
    drift = code != leaf_val
    lo_idx = np.arange(1, n, 2)          # lo leaf of byte b = 2b+1
    b = np.arange(n_bytes)
    is_letter = word_of >= 0
    own_hi = leaf_val[0::2].astype(np.int32)
    next_hi = np.full(n_bytes, -1, np.int32)
    next_hi[:-1] = own_hi[1:]            # right neighbour of lo(b) = hi(b+1)

    lo_drift = drift[lo_idx]
    lo_code = code[lo_idx]

    cls_A = is_letter & is_final                                  # word-final
    cls_B = is_letter & ~is_final & (next_hi == own_hi)           # node-capable
    cls_C = is_letter & ~is_final & (next_hi != own_hi)           # internal, no node
    cls_S = ~is_letter                                            # separators

    def bucket(mask, name):
        nn = int(mask.sum())
        dd = int((lo_drift & mask).sum())
        return {"class": name, "cells": nn, "drifted": dd,
                "rate": (dd / nn) if nn else 0.0}

    buckets = [bucket(cls_A, "A_word_final_lo"),
               bucket(cls_B, "B_internal_nodecapable_lo"),
               bucket(cls_C, "C_internal_nonode_lo"),
               bucket(cls_S, "S_separator_lo")]

    # hi-leaf drift (was 0 in the lexicon run) + separator-hi capture channel
    hi_drift_total = int(drift[0::2].sum())
    sep_hi_drift = int((drift[0::2] & cls_S).sum())

    # F-D: landing codes of drifted node-capable cells vs their flank code
    bd = cls_B & lo_drift
    fd_total = int(bd.sum())
    fd_captured = int((bd & (lo_code == own_hi)).sum())
    landing_hist = ({int(c): int((lo_code[lo_drift] == c).sum())
                     for c in np.unique(lo_code[lo_drift])} if lo_drift.any() else {})
    # drift-by-lo-value (v0.2 victims were {0,1,2,4,8})
    by_val = {}
    for v in range(16):
        m = (leaf_val[lo_idx] == v) & is_letter
        nn, dd = int(m.sum()), int((m & lo_drift).sum())
        if nn:
            by_val[v] = {"cells": nn, "drifted": dd, "rate": dd / nn}

    rA, rB, rC = buckets[0]["rate"], buckets[1]["rate"], buckets[2]["rate"]
    verdicts = {
        "F-A_wordfinal_suppressed": "PASS" if rA < 0.005 else "FAIL",
        "F-B_internal_persists": "PASS" if (buckets[1]["drifted"] > 0 and rB > 10 * max(rA, 1e-9)) else "FAIL",
        "F-C_internal_nonode_suppressed": "PASS" if rC < 0.005 else "FAIL",
        "F-D_capture_not_random": ("PASS" if fd_total and fd_captured / fd_total >= 0.95
                                   else ("N/A no drifted node-capable cells" if not fd_total else "FAIL")),
    }
    for k, v in verdicts.items():
        print(f"FALSIFIER {k}: {v}", flush=True)
    for bk in buckets:
        print(f"  {bk['class']:>28}: {bk['drifted']:>6}/{bk['cells']:<8} = {bk['rate']*100:.3f}%")
    print(f"  hi-leaf drift total={hi_drift_total} (separator-hi={sep_hi_drift})")
    print(f"  F-D capture fraction: {fd_captured}/{fd_total}")

    report = {
        "artifact": "pour-v1-diag-b-off (throwaway unmasked rig, v0.2 physics)",
        "stream": STREAM_NPZ, "seed_decode": SEED_NPZ,
        "words": args.words, "n_bytes": n_bytes, "n_leaves": n,
        "ticks": args.ticks, "T0": args.t0, "dtype": "float32",
        "separator": "0x20 between consecutive positions; gap_before splices get one too (counted below)",
        "n_splice_separators": int(sep_splice.sum()),
        "wall_seconds": wall,
        "falsifier_verdicts": verdicts,
        "lo_drift_buckets": buckets,
        "hi_leaf_drift_total": hi_drift_total,
        "separator_hi_drift": sep_hi_drift,
        "fd_capture": {"drifted_node_capable": fd_total, "landed_on_flank_code": fd_captured},
        "lo_drift_landing_code_hist": landing_hist,
        "drift_by_lo_value": by_val,
        "final": {
            "bond_survival": float(bonded.mean()),
            "mean_det": float(det.mean()), "min_det": float(det.min()),
            "frac_locked": float((det >= rig.LOCK).mean()),
            "total_drift_cells": int(drift.sum()),
            **totals,
        },
        "samples": samples,
    }
    rp = f"{args.out_prefix}-report.json"
    with open(rp, "w") as f:
        json.dump(report, f, indent=1)
    np.savez_compressed(f"{args.out_prefix}-state.npz", det=det, code=code,
                        bonded=bonded, tau_pair=tau_pair, leaf_val=leaf_val,
                        word_of=word_of, is_final=is_final)
    print(f"DIAG COMPLETE — report: {rp}", flush=True)


if __name__ == "__main__":
    main()
