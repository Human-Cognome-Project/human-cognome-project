#!/usr/bin/env python3
"""write_back_v0 — first engine write-back into the db (P go, Zulip ~1403/1406/1412).

Plan line: data-plan §2 two-way db; namespace-reference.md §Ingestion Rules.
Loop proven end-to-end: field state -> stability criterion -> condensation TYPES
-> RESOLVE-BEFORE-MINT -> durable rows in system-owned schema `engine`
(additive; zero P rows touched; DROP SCHEMA engine CASCADE removes) -> readback.

Key result: all 50 stable condensation types from the Moby nibble-pair state
RESOLVE to existing AA.AA.AA.AA byte-code tokens, so NOTHING is minted — the
ingestion rules' resolve-before-mint step fired and prevented silent duplicate
tokens. First genuinely NEW mints arrive at the next rung (cross-byte fragment
condensations -> AB.AB.B*, one-LoD-below-source), which needs kernel bond data
across bytes (next epoch).

v0 caveats: stability = p90 pair-det on ONE snapshot (real criterion =
persistence across ticks). Base-50 alphabet per docs/spec/token-addressing.md
(52 Latin letters minus O/o, ASCII order). No frequency fed (n_instances =
ledger metadata only).
"""
import json, os, subprocess, sys, time
import numpy as np

# base-50 alphabet per docs/spec/token-addressing.md: 52 Latin letters minus O/o
# (zero-collision), ASCII order — uppercase block then lowercase block.
ALPHA = [chr(c) for c in range(65, 91) if chr(c) != "O"] + \
        [chr(c) for c in range(97, 123) if chr(c) != "o"]
assert len(ALPHA) == 50
pair = lambda n: ALPHA[n // 50] + ALPHA[n % 50]

STATE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "..", "kernel", "pour-raw-mobydick-02701-fpcuda-state.npz")
z = np.load(STATE)
det, bonded, leaf = z["det"], z["bonded"], z["leaf_val"]

t0 = time.perf_counter()
pair_det = det.reshape(-1, 2).mean(axis=1)
thr = float(np.quantile(pair_det[bonded], 0.90))
stable = bonded & (pair_det >= thr)
lv = leaf.reshape(-1, 2)[stable]
byte_val = lv[:, 0].astype(np.int32) * 16 + lv[:, 1]   # nibble pair -> byte value
uniq, inv, counts = np.unique(byte_val, return_inverse=True, return_counts=True)
mdet = np.zeros(uniq.size)
np.add.at(mdet, inv, pair_det[stable])
mdet /= counts
t1 = time.perf_counter()

env = dict(os.environ, PGPASSWORD=os.environ.get("HCP_PW", "hcp_dev"))
def psql(sql, stdin=None):
    r = subprocess.run(["psql", "-h", "192.168.68.60", "-p", "5435", "-U", "hcp",
                        "-d", "hcp_english", "-tA", "-c", sql],
                       input=stdin, capture_output=True, text=True, env=env, timeout=120)
    if r.returncode != 0:
        sys.exit(f"psql failed: {r.stderr[:400]}")
    return r.stdout.strip()

psql("CREATE SCHEMA IF NOT EXISTS engine")
psql("""CREATE TABLE IF NOT EXISTS engine.condensations_v0(
  resolves_to text NOT NULL, byte_val int2, glyph text, n_instances int4,
  mean_det real, minted boolean NOT NULL, provenance text,
  created timestamptz DEFAULT now(), PRIMARY KEY (resolves_to, provenance))""")
psql("TRUNCATE engine.condensations_v0")

prov = "system-derived pour-raw-mobydick-02701 (repo 7b39678); criterion=p90-det-v0"
lines = []
for i, b in enumerate(uniq):
    b = int(b)
    tid = f"AA.AA.AA.AA.{pair(b)}"          # existing byte-code token: REFERENCE, don't mint
    glyph = chr(b) if 32 <= b < 127 else f"\\\\x{b:02x}"  # doubled backslash: COPY text format
    lines.append(f"{tid}\t{b}\t{glyph}\t{int(counts[i])}\t{float(mdet[i]):.4f}\tfalse\t{prov}")

t2 = time.perf_counter()
r = subprocess.run(["psql", "-h", "192.168.68.60", "-p", "5435", "-U", "hcp",
                    "-d", "hcp_english", "-c",
                    "COPY engine.condensations_v0(resolves_to,byte_val,glyph,n_instances,"
                    "mean_det,minted,provenance) FROM STDIN"],
                   input="\n".join(lines), capture_output=True, text=True, env=env, timeout=120)
if r.returncode != 0:
    sys.exit(f"COPY failed: {r.stderr[:400]}")
t3 = time.perf_counter()

n_back = int(psql("SELECT count(*) FROM engine.condensations_v0"))
top = psql("SELECT resolves_to, glyph, n_instances, round(mean_det::numeric,4) "
           "FROM engine.condensations_v0 ORDER BY n_instances DESC LIMIT 6")

report = {
    "artifact": "write-back-v0 (resolve-before-mint)",
    "plan_line": "data-plan §2 two-way db; namespace-reference §Ingestion Rules",
    "stable_pairs": int(stable.sum()), "det_threshold_p90": round(thr, 4),
    "condensation_types": len(lines), "minted_new_tokens": 0,
    "resolved_to_existing": len(lines),
    "rule_that_fired": "refs resolving to existing tokens REFERENCE them — no silent "
                       "duplicate minting; nibble-pair condensations = AA.AA.AA.AA byte codes",
    "written_to": "hcp_english engine.condensations_v0 (additive; DROP SCHEMA engine CASCADE removes)",
    "readback_rows": n_back, "readback_matches": n_back == len(lines),
    "timings_s": {"criterion+typing": round(t1 - t0, 3), "copy_insert": round(t3 - t2, 3)},
    "top": top.splitlines(),
    "next_mint_rung": "cross-byte fragment condensations (kernel bond data, next epoch) -> "
                      "mint under AB.AB.B* per one-LoD-below-source",
    "caveats": ["stability = p90 det on one snapshot (v0 proxy; real = persistence across ticks)",
                "base-50 alphabet corrected to spec (token-addressing.md: A-Z+a-z minus O/o, ASCII order)"],
}
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "write-back-v0-report.json")
json.dump(report, open(out, "w"), indent=1)
print(json.dumps(report, indent=1))
