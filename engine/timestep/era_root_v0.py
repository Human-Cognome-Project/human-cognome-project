#!/usr/bin/env python3
"""era_root_v0 — first temporal-root read over era-tagged English matter.

Plan line: data-plan §A.4 (temporality; P go 2026-09-01 Zulip ~1384/1387/1393).
Role: derived receipt — read-only over substrate (hcp_english), regenerable.

Era roots are SPANS at era precision (bootstrap-tagged scaffold; attestations
refine later): OE c.450-1150, ME c.1150-1500. This read: (1) cohort census by
era and edge-kind (inherited=descent, borrowed=re-entry, derived=formation);
(2) gathering test — do relation edges among era-rooted entries stay within
era beyond the independence expectation? (3) first refusal/width candidates:
dual-era entries (span-wideners) and borrowed-from-era re-entries.
No frequency anywhere. No writes (P owns writes; write-back namespace unsettled).
"""
import csv, io, json, os, subprocess, sys
from collections import Counter, defaultdict

DB = dict(host=os.environ.get("HCP_HOST", "192.168.68.60"), port="5435",
          user="hcp", db="hcp_english", pw=os.environ.get("HCP_PW", "hcp_dev"))

ERA_SPANS = {"OE": (450, 1150), "ME": (1150, 1500)}  # conventional; P may strike
CATS = {  # category name -> (era, edge_kind)
    "English terms inherited from Old English": ("OE", "inherited"),
    "English terms derived from Old English": ("OE", "derived"),
    "English terms borrowed from Old English": ("OE", "borrowed"),
    "English learned borrowings from Old English": ("OE", "borrowed"),
    "English surnames from Old English": ("OE", "onomastic"),
    "English male given names from Old English": ("OE", "onomastic"),
    "English terms inherited from Middle English": ("ME", "inherited"),
    "English terms derived from Middle English": ("ME", "derived"),
    "English surnames from Middle English": ("ME", "onomastic"),
}

def q(sql):
    env = dict(os.environ, PGPASSWORD=DB["pw"])
    r = subprocess.run(["psql", "-h", DB["host"], "-p", DB["port"], "-U", DB["user"],
                        "-d", DB["db"], "-tA", "-F", "\t", "-c", sql],
                       capture_output=True, text=True, env=env, timeout=300)
    if r.returncode != 0:
        sys.exit(f"psql failed: {r.stderr[:400]}")
    return [ln.split("\t") for ln in r.stdout.splitlines() if ln]

names = ",".join("'" + n.replace("'", "''") + "'" for n in CATS)
cohort_rows = q(f"""
  SELECT DISTINCT s.entry_id, e.word, e.token_id, sc.name
  FROM sense_categories sc
  JOIN senses s ON s.id = sc.sense_id
  JOIN entries e ON e.id = s.entry_id
  WHERE sc.name IN ({names})""")

entry_eras = defaultdict(set)   # entry_id -> {era}
entry_kinds = defaultdict(set)  # entry_id -> {edge_kind}
tok2entry, entry_word = {}, {}
for eid, word, tok, cat in cohort_rows:
    era, kind = CATS[cat]
    eid = int(eid)
    entry_eras[eid].add(era); entry_kinds[eid].add(kind)
    entry_word[eid] = word
    if tok: tok2entry[tok] = eid

census = Counter()
for eid, eras in entry_eras.items():
    key = "+".join(sorted(eras))
    census[key] += 1
kind_census = Counter(k for ks in entry_kinds.values() for k in ks)
dual = [entry_word[e] for e, s in entry_eras.items() if len(s) == 2]
reentry = sorted({entry_word[e] for e, ks in entry_kinds.items() if "borrowed" in ks})

ids = ",".join(map(str, entry_eras))
rel_rows = q(f"""
  SELECT entry_id, relation_type, target_token_id
  FROM relations
  WHERE entry_id IN ({ids}) AND target_token_id IS NOT NULL AND target_token_id <> ''""")

def era_of(eid):  # single-era label; dual-era counts to both, handled below
    return sorted(entry_eras[eid])

pair_obs = Counter(); rel_kinds = Counter(); n_in_cohort = 0
marg_src = Counter(); marg_tgt = Counter()
for eid, rtype, ttok in rel_rows:
    eid = int(eid)
    tgt = tok2entry.get(ttok)
    if tgt is None or tgt == eid:
        continue
    n_in_cohort += 1
    rel_kinds[rtype] += 1
    for es in era_of(eid):
        for et in era_of(tgt):
            pair_obs[(es, et)] += 1
            marg_src[es] += 1; marg_tgt[et] += 1

tot = sum(pair_obs.values()) or 1
gathering = {}
for (es, et), obs in sorted(pair_obs.items()):
    exp = marg_src[es] * marg_tgt[et] / tot
    gathering[f"{es}->{et}"] = {"observed": obs, "expected_indep": round(exp, 1),
                                "ratio": round(obs / exp, 3) if exp else None}

report = {
    "artifact": "era-root-v0", "plan_line": "data-plan §A.4 temporality / P go ~1384",
    "role": "derived receipt, read-only, regenerable", "era_spans": ERA_SPANS,
    "cohort_census_by_era": dict(census), "edge_kind_census": dict(kind_census),
    "dual_era_entries": {"count": len(dual), "sample": sorted(dual)[:20]},
    "borrowed_reentry": {"count": len(reentry), "sample": reentry[:20]},
    "relations_read": {"cohort_source_rows": len(rel_rows),
                       "both_ends_in_cohort": n_in_cohort,
                       "relation_type_mix": dict(rel_kinds.most_common(8))},
    "gathering_within_vs_cross_era": gathering,
    "notes": ["attestation refinement (dated docs) = next; not in this read",
              "no frequency computed; no db writes"],
}
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "era-root-v0-report.json")
json.dump(report, open(out, "w"), indent=1)
print(json.dumps(report, indent=1)[:3000])
print("wrote", out)
