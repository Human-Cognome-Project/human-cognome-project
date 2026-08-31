#!/usr/bin/env python3
"""RAW POUR — verbatim byte-level ingestion of ANY file. ZERO CURATION.

P's doctrine, restated as code (2026-08-31, thread 1296): the file's bytes ARE
the given matter. No marker stripping, no lowercasing, no tokenization, no
drops. BOM, Gutenberg header, punctuation, line breaks — all poured, all
sealed. The field NOTICING the common header is the point of byte-level
ingestion. Structure (tokens, entries, classes) is DERIVED host-side for
measurement only and never touches the poured matter.

Physics = the shipped v0.3.2 rig (pour.py: given anchor; twin proofs T1/T2/F9
run first). Seed law unchanged: amp one-hot per nibble, every byte pair bonded
at t=0, every cell GIVEN (the whole file is bedrock).

Acceptance (two-axis, v0.3.2): given-drift == 0 (raw census — valid because
deaths must be 0) AND permanent-death == 0 AND breaks == re-completions.
Byte-class terrain readout (derived, measurement-only) localizes any breach
and maps clocked healing onto matter classes — prediction: healing events
concentrate in BOTH-LIGHT byte classes (both nibbles popcount<=1: space,
!, ", $, (, @, A, B, D, H, ...), same physics as the space story.

Run:  .venv-kernel/bin/python pour_raw.py <file> [--ticks 200] [--t0 0.02]
      [--sample-every 20] [--out-prefix <name>] [--save-state]
GPU:  identical invocation on a CUDA seat once pour.py's ti.init targets cuda
      (per-arch twin pass required first — f32 GPU determinism gets its own
      tolerance read; host-fed noise mode exists for exact comparison).
"""
import argparse
import hashlib
import json
import os
import time

import numpy as np

import pour  # v0.3.2 rig: twin-proved kernel + engine_run (runs ti.init)

LIGHT = {0, 1, 2, 4, 8}   # popcount <= 1 (WEIGHT <= 2): the measured victim set


def byte_class(b):
    if b == 0x20:
        return "space"
    if 0x61 <= b <= 0x7A:
        return "lower"
    if 0x41 <= b <= 0x5A:
        return "upper"
    if 0x30 <= b <= 0x39:
        return "digit"
    if b in (0x0A, 0x0D):
        return "newline"
    if 0x21 <= b <= 0x2F or 0x3A <= b <= 0x40 or 0x5B <= b <= 0x60 or 0x7B <= b <= 0x7E:
        return "punct"
    return "other"   # BOM bytes, high-bit chars — poured all the same


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--t0", type=float, default=0.02)
    ap.add_argument("--sample-every", type=int, default=20)
    ap.add_argument("--out-prefix", default=None)
    ap.add_argument("--save-state", action="store_true")
    args = ap.parse_args()
    prefix = args.out_prefix or ("pour-raw-" + os.path.basename(args.file).split(".")[0]
                                 .lower().replace(" ", "-")[:40])

    pour.twin_tests()

    raw = open(args.file, "rb").read()
    sha = hashlib.sha256(raw).hexdigest()
    byte_val = np.frombuffer(raw, dtype=np.uint8)
    n_bytes = len(byte_val)
    n = 2 * n_bytes
    leaf_val = np.empty(n, np.uint8)
    leaf_val[0::2] = byte_val >> 4
    leaf_val[1::2] = byte_val & 15
    classes = np.array([byte_class(int(b)) for b in byte_val])
    both_light = np.array([(int(b) >> 4) in LIGHT and (int(b) & 15) in LIGHT
                           for b in byte_val])
    print(f"raw pour: {args.file}\n  sha256 {sha}\n  {n_bytes:,} bytes VERBATIM "
          f"({n:,} leaves), all given, {args.ticks} ticks, T0={args.t0}", flush=True)
    cl_counts = {c: int((classes == c).sum()) for c in np.unique(classes)}
    print(f"  byte classes: {cl_counts}; both-light bytes: {int(both_light.sum()):,}",
          flush=True)

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
    healed = tau_pair > 0
    heal_by_class = {c: int((healed & (classes == c)).sum())
                     for c in np.unique(classes) if (healed & (classes == c)).any()}
    drift_cells = (code != leaf_val)
    drift_by_class = {c: int((drift_cells[0::2] | drift_cells[1::2])[classes == c].sum())
                      for c in np.unique(classes)
                      if ((drift_cells[0::2] | drift_cells[1::2])[classes == c]).any()}
    print(f"ACCEPTANCE: IDENTITY given-drift = {drift} "
          f"({'PASS' if drift == 0 else 'FAIL — WALL BREACHED'})", flush=True)
    print(f"ACCEPTANCE: EXISTENCE permanent-death = {perm_death}, "
          f"breaks {totals['cum_breaks']} vs re-completions {totals['cum_completions']} "
          f"({'PASS' if perm_death == 0 and balanced else 'FAIL — EXISTENCE LOST'})", flush=True)
    print(f"healed (tau>0) pairs by class: {heal_by_class}", flush=True)
    print(f"healed pairs both-light: {int((healed & both_light).sum())} of {int(healed.sum())}",
          flush=True)

    report = {
        "artifact": f"{prefix} (VERBATIM raw-byte pour, zero curation, v0.3.2)",
        "source_file": args.file, "source_sha256": sha,
        "n_bytes": n_bytes, "n_leaves": n, "byte_classes": cl_counts,
        "n_both_light_bytes": int(both_light.sum()),
        "ticks": args.ticks, "T0": args.t0, "dtype": "float32",
        "wall_seconds": wall_s,
        "acceptance": {
            "identity_given_drift": drift,
            "existence_permanent_death": perm_death,
            "breaks_eq_completions": balanced,
            "verdict": "PASS/PASS" if (drift == 0 and perm_death == 0 and balanced) else "FAIL",
        },
        "healed_pairs_by_class": heal_by_class,
        "healed_pairs_both_light": int((healed & both_light).sum()),
        "healed_pairs_total": int(healed.sum()),
        "drift_bytes_by_class": drift_by_class,
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
    rp = f"{prefix}-report.json"
    with open(rp, "w") as f:
        json.dump(report, f, indent=1)
    if args.save_state:
        np.savez_compressed(f"{prefix}-state.npz", det=det, code=code,
                            bonded=bonded, tau_pair=tau_pair, leaf_val=leaf_val)
    print(f"RAW POUR COMPLETE — report: {rp}", flush=True)


if __name__ == "__main__":
    main()
