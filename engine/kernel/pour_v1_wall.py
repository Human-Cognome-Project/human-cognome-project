#!/usr/bin/env python3
"""v1 MOBY-DICK POUR — (b)-ON real run: the full running-text document through
the v0.3.1 identity wall.

The first at-scale RUNNING-TEXT question (the lexicon pour asked "does the
engine hold the English lexicon"; this asks "does it hold a real document"):
215,092 word positions of Moby-Dick in document order, words decoded to bytes
via the v0 instance artifact, joined with space bytes (0x20 — every inter-token
char in the normalized corpus is a non-letter). Every poured cell is GIVEN
(P's solid-wall-of-temporality directive: data can self-organize, not be
directly changed in values) — separators included; they are document structure,
given-by-normalization. gap_before splices need no bond masking here: bonds in
this engine are within-byte (hi,lo) pairs only, never cross-byte (noted, not
hidden).

Physics = the SHIPPED v0.3.1 bytes: pour.tick_v02_ti + Planner's Phase B, twin
proofs run first (pour.twin_tests — T1/T2 vs the numpy reference).

Two-axis acceptance, same as v0.3.1 lexicon:
  IDENTITY   given-drift == 0 (structural — the wall, not luck)
  EXISTENCE  permanent-death == 0 and cum_breaks == cum_completions

Diagnostic-slice context (pour-v1-diag-b-off, 2026-08-31): without the wall,
running text loses 32.3% of node-capable internal light-lo cells to flank
capture and 1019 separator pairs to permanent bond death; word-final cells are
structurally safe (0/50,000). This run measures what the wall buys on the same
terrain.

Run:  .venv-kernel/bin/python pour_v1_wall.py [--ticks 200]
      [--out-prefix pour-v1-wall-mobydick] [--save-state]
Outputs (EXPLICIT names): <prefix>-report.json, <prefix>-state.npz
"""
import argparse
import json
import time

import numpy as np

import pour  # v0.3.1 rig: twin-proved kernel + engine_run (runs ti.init)

STREAM_NPZ = "/home/silas/shared-brain/exchange/word-instance-stream-v1.npz"
SEED_NPZ = "instance-seed-v0.npz"
SEP = 0x20


def build_stream_field(n_words=None):
    """Same construction as v1-diag/pour_v1_diag.py (provenance: diag slice)."""
    z = np.load(STREAM_NPZ)
    ids = z["doc_word__ids"][:n_words] if n_words else z["doc_word__ids"]
    gaps = z["doc_word__gap_before"][:len(ids)]
    s = np.load(SEED_NPZ)
    w_off = s["word_charinst_off"]
    cb_off = s["charinst_byteinst_off"]
    btype = s["byteinst_type"]

    b0 = cb_off[w_off[ids]]
    b1 = cb_off[w_off[ids + 1]]
    lens = (b1 - b0).astype(np.int64)

    n_bytes = int(lens.sum()) + (len(ids) - 1)
    byte_val = np.empty(n_bytes, np.uint8)
    word_of = np.full(n_bytes, -1, np.int64)
    is_final = np.zeros(n_bytes, bool)
    sep_is_splice = np.zeros(n_bytes, bool)
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
    return byte_val, word_of, is_final, sep_is_splice


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--words", type=int, default=None, help="prefix; default=full stream")
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--t0", type=float, default=0.02)
    ap.add_argument("--sample-every", type=int, default=20)
    ap.add_argument("--out-prefix", default="pour-v1-wall-mobydick")
    ap.add_argument("--save-state", action="store_true")
    args = ap.parse_args()

    pour.twin_tests()

    byte_val, word_of, is_final, sep_splice = build_stream_field(args.words)
    n_bytes = len(byte_val)
    n = 2 * n_bytes
    leaf_val = np.empty(n, np.uint8)
    leaf_val[0::2] = byte_val >> 4
    leaf_val[1::2] = byte_val & 15
    n_sep = int((word_of < 0).sum())
    print(f"v1 wall pour: {n_bytes:,} bytes ({n:,} leaves; {n_sep:,} separators, "
          f"{int(sep_splice.sum()):,} splices), (b)-ON v0.3.1 wall, "
          f"{args.ticks} ticks, T0={args.t0}", flush=True)

    amp = np.zeros((n, 16), dtype=np.float32)
    amp[np.arange(n), leaf_val] = 1.0
    bonded = np.ones(n_bytes, dtype=bool)

    samples = []

    def on_sample(s):
        samples.append(s)
        print(f"  t={s['tick']:>4} T={s['T']:.4f} "
              f"bonds={s['bond_survival'] * 100:.4f}% "
              f"breaks={s['cum_breaks']} recompl={s['cum_completions']} "
              f"det μ={s['mean_det']:.4f} min={s['min_det']:.3f} "
              f"{s['ticks_per_sec']:.2f} t/s", flush=True)

    t_start = time.time()
    amp, bonded, tau_pair, det, code, totals = pour.engine_run(
        amp, bonded, args.ticks, T0=args.t0, sample_every=args.sample_every,
        on_sample=on_sample, true_code=leaf_val.astype(np.int32),
        given=leaf_val.astype(np.int32))
    wall_s = time.time() - t_start

    drift = int((code != leaf_val).sum())
    perm_death = int((~bonded).sum())
    balanced = totals["cum_breaks"] == totals["cum_completions"]
    # per-terrain drift readout (should be all-zero; localizes any breach)
    lo_drift = (code != leaf_val)[1::2]
    sep_mask = word_of < 0
    terrain = {
        "wordfinal_lo_drift": int((lo_drift & (word_of >= 0) & is_final).sum()),
        "internal_lo_drift": int((lo_drift & (word_of >= 0) & ~is_final).sum()),
        "separator_lo_drift": int((lo_drift & sep_mask).sum()),
        "separator_death": int(((~bonded) & sep_mask).sum()),
        "letter_death": int(((~bonded) & ~sep_mask).sum()),
    }
    print(f"ACCEPTANCE: IDENTITY given-drift = {drift} "
          f"({'PASS — structural zero' if drift == 0 else 'FAIL — WALL BREACHED'})", flush=True)
    print(f"ACCEPTANCE: EXISTENCE permanent-death = {perm_death}, "
          f"breaks {totals['cum_breaks']} vs re-completions {totals['cum_completions']} "
          f"({'PASS — every break healed, clocked' if perm_death == 0 and balanced else 'FAIL — EXISTENCE LOST'})",
          flush=True)
    print(f"terrain: {terrain}", flush=True)

    report = {
        "artifact": "pour-v1-wall-mobydick (running-text pour, v0.3.1 wall ON)",
        "stream": STREAM_NPZ, "seed_decode": SEED_NPZ,
        "words": int(args.words or 215092), "n_bytes": n_bytes, "n_leaves": n,
        "n_separators": n_sep, "n_splice_separators": int(sep_splice.sum()),
        "ticks": args.ticks, "T0": args.t0, "dtype": "float32",
        "wall_seconds": wall_s,
        "acceptance": {
            "identity_given_drift": drift,
            "existence_permanent_death": perm_death,
            "breaks_eq_completions": balanced,
            "verdict": "PASS/PASS" if (drift == 0 and perm_death == 0 and balanced) else "FAIL",
        },
        "terrain_drift": terrain,
        "final": {
            "bond_survival": float(bonded.mean()),
            "tau_pair_max": int(tau_pair.max()),
            "tau_pair_reticked_pairs": int((tau_pair > 0).sum()),
            "mean_det": float(det.mean()),
            "p01_det": float(np.percentile(det, 1)),
            "min_det": float(det.min()),
            "frac_locked": float((det >= pour.LOCK).mean()),
            **totals,
        },
        "samples": samples,
    }
    rp = f"{args.out_prefix}-report.json"
    with open(rp, "w") as f:
        json.dump(report, f, indent=1)
    if args.save_state:
        np.savez_compressed(f"{args.out_prefix}-state.npz", det=det, code=code,
                            bonded=bonded, tau_pair=tau_pair, leaf_val=leaf_val,
                            word_of=word_of, is_final=is_final)
    print(f"V1 WALL POUR COMPLETE — report: {rp}", flush=True)


if __name__ == "__main__":
    main()
