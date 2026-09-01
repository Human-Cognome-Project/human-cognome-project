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

ONE-DOOR ENTRANCE (data-plan §4): --manifest <handover.json> consumes an
ingest_v0 handover manifest instead of a bare path — the file at the
manifest's source is sha256-VERIFIED against the ledgered hash before a single
byte pours (hard fail on mismatch), and the report carries the ledger_event_id
so the pour is traceable to its ingestion-initiation event. For a --db-doc
manifest, --verify-record additionally re-enumerates the doc's complete
atomized record against the live db (counts + md5s, read-only) and gates on
record_md5 equality — the independent-view check of the other half.

TICK-STATE EMISSION (data-plan §6 checkpoint direction; feeds planner's
persistence criterion): --state-every N snapshots the live field every N
ticks to <state-dir>/state-tNNNNNN.npz (det, code, bonded, tau_pair
copies; amp too under --state-amp) plus seed.npz (leaf_val) once and a
states-index.json naming every snapshot with tick/T/sha256. Emission is
observation-only — the run's physics and acceptance are byte-identical with
it on or off.

Run:  .venv-kernel/bin/python pour_raw.py <file|--manifest m.json>
      [--ticks 200] [--t0 0.02]
      [--sample-every 20] [--out-prefix <name>] [--save-state]
      [--state-every N] [--state-dir DIR] [--state-amp] [--verify-record]
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


def load_manifest(path):
    """Resolve an ingest_v0 handover manifest (file or --db-doc form) to
    (source_path, expected_sha, meta). The manifest is EMIT-ONLY; the ledger
    row is the store — we carry ledger_event_id forward for traceability."""
    m = json.load(open(path))
    meta = {"ledger_event_id": m.get("ledger_event_id"),
            "manifest_artifact": m.get("artifact"),
            "manifest_plan_line": m.get("plan_line")}
    if "byte_stream" in m:                       # --db-doc entrance
        bs = m["byte_stream"]
        if not bs.get("exists"):
            raise SystemExit(f"manifest byte stream missing on this seat: "
                             f"{bs.get('path')}")
        meta["doc"] = m.get("doc")
        meta["record_md5"] = m.get("record", {}).get("record_md5")
        meta["byte_stream_linkage"] = bs.get("linkage")
        return bs["path"], bs.get("sha256"), meta
    return m["source"], m.get("content_sha256"), meta


def verify_record(manifest_path):
    """Independent-view check of the atomized half: re-enumerate every
    hcp_fic_pbm table for the doc (read-only, same deterministic md5 method as
    the ingest side) and gate on record_md5 equality. Lazy imports so the
    kernel venv only needs psycopg2 when this flag is used."""
    import importlib.util
    m = json.load(open(manifest_path))
    doc_pk = int(m["source"].split("id=")[1])
    ing = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "..", "ingest", "ingest_v0.py")
    spec = importlib.util.spec_from_file_location("ingest_v0", ing)
    iv = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(iv)
    conn = iv.connect("hcp_fic_pbm")
    cur = conn.cursor()
    cur.execute("SELECT ns||'.'||p2||'.'||p3||'.'||p4||'.'||p5 "
                "FROM pbm_documents WHERE id = %s", (doc_pk,))
    token = cur.fetchone()[0]
    tables = {}
    for tname, q in iv.DOC_TABLES.items():
        cur.execute(f"SELECT count(*), md5(string_agg(t::text, E'\\n' "
                    f"ORDER BY t::text)) FROM ({q}) s(t)",
                    {"d": doc_pk, "tok": token})
        n, h = cur.fetchone()
        tables[tname] = {"rows": int(n), "md5": h}
    conn.close()
    got = hashlib.md5("".join(
        f"{t}:{v['md5']}" for t, v in sorted(tables.items())
        if v["md5"]).encode()).hexdigest()
    want = m["record"]["record_md5"]
    if got != want:
        raise SystemExit(f"RECORD VERIFY FAIL doc id={doc_pk}: live db "
                         f"record_md5 {got} != manifest {want}")
    n_rows = sum(v["rows"] for v in tables.values())
    print(f"  record verify PASS doc id={doc_pk}: record_md5 {got} matches "
          f"manifest; {len(tables)} tables, {n_rows:,} rows live", flush=True)
    return got


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file", nargs="?")
    ap.add_argument("--manifest", default=None,
                    help="ingest_v0 handover manifest json (the one door)")
    ap.add_argument("--ticks", type=int, default=200)
    ap.add_argument("--t0", type=float, default=0.02)
    ap.add_argument("--sample-every", type=int, default=20)
    ap.add_argument("--out-prefix", default=None)
    ap.add_argument("--save-state", action="store_true")
    ap.add_argument("--state-every", type=int, default=0,
                    help="emit live-field snapshots every N ticks (0 = off)")
    ap.add_argument("--state-dir", default=None)
    ap.add_argument("--state-amp", action="store_true",
                    help="include amp (16 floats/leaf) in each snapshot")
    ap.add_argument("--verify-record", action="store_true",
                    help="--db-doc manifests: re-enumerate the atomized "
                         "record against the live db before pouring")
    args = ap.parse_args()
    if bool(args.file) == bool(args.manifest):
        ap.error("exactly one of <file> or --manifest")

    expected_sha, man_meta = None, {}
    if args.manifest:
        src, expected_sha, man_meta = load_manifest(args.manifest)
        args.file = src
        if args.verify_record:
            if "record_md5" not in man_meta:
                raise SystemExit("--verify-record needs a --db-doc manifest")
            verify_record(args.manifest)
    prefix = args.out_prefix or ("pour-raw-" + os.path.basename(args.file).split(".")[0]
                                 .lower().replace(" ", "-")[:40])

    pour.twin_tests()

    raw = open(args.file, "rb").read()
    sha = hashlib.sha256(raw).hexdigest()
    if expected_sha is not None:
        if sha != expected_sha:
            raise SystemExit(f"SHA VERIFY FAIL: file {sha} != ledgered "
                             f"{expected_sha} — refusing to pour")
        print(f"  one-door sha verify PASS: {sha[:16]}… matches ledger event "
              f"{man_meta.get('ledger_event_id')}", flush=True)
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

    state_index = []
    state_dir = None
    if args.state_every:
        state_dir = args.state_dir or f"{prefix}-states"
        os.makedirs(state_dir, exist_ok=True)
        seed_path = os.path.join(state_dir, "seed.npz")
        np.savez_compressed(seed_path, leaf_val=leaf_val)
        print(f"  tick-state emission ON: every {args.state_every} ticks -> "
              f"{state_dir}/ (seed.npz written)", flush=True)

    def on_state(tick, arr):
        sp = os.path.join(state_dir, f"state-t{tick:06d}.npz")
        payload = {"det": arr["det"].copy(), "code": arr["code"].copy(),
                   "bonded": arr["bonded"].copy(),
                   "tau_pair": arr["tau_pair"].copy()}
        if args.state_amp:
            payload["amp"] = arr["amp"].copy()
        np.savez_compressed(sp, **payload)
        fsha = hashlib.sha256(open(sp, "rb").read()).hexdigest()
        state_index.append({"tick": tick, "T": arr["T"], "file": sp,
                            "sha256": fsha,
                            "n_bytes": os.path.getsize(sp)})
        print(f"  state t={tick:>4} -> {sp} "
              f"({os.path.getsize(sp) / 1e6:.1f} MB)", flush=True)

    t_start = time.time()
    amp, bonded, tau_pair, det, code, totals = pour.engine_run(
        amp, bonded, args.ticks, T0=args.t0, sample_every=args.sample_every,
        on_sample=on_sample, true_code=leaf_val.astype(np.int32),
        given=leaf_val.astype(np.int32),
        on_state=on_state if args.state_every else None,
        state_every=args.state_every)
    wall_s = time.time() - t_start

    if state_dir:
        idx = {
            "artifact": f"{prefix} tick-state emission (observation-only)",
            "plan_line": "data-plan §6 checkpoint-direction + §4 one-door "
                         "consumption; feeds planner's persistence criterion",
            "source_file": args.file, "source_sha256": sha,
            "ledger_event_id": man_meta.get("ledger_event_id"),
            "state_every": args.state_every, "ticks": args.ticks,
            "includes_amp": bool(args.state_amp),
            "seed": os.path.join(state_dir, "seed.npz"),
            "arrays": ["det", "code", "bonded", "tau_pair"]
                      + (["amp"] if args.state_amp else []),
            "snapshots": state_index,
        }
        with open(os.path.join(state_dir, "states-index.json"), "w") as f:
            json.dump(idx, f, indent=1)

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
        "plan_line": "data-plan §1.4 seeding grain + §4 one-door full-file "
                     "consumption + §6 tick-state emission",
        "one_door": man_meta or None,
        "sha_verified_against_ledger": expected_sha is not None,
        "tick_states": (os.path.join(state_dir, "states-index.json")
                        if state_dir else None),
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
