#!/usr/bin/env python3
"""Aggregate the 9 per-doc pour reports into one corpus baseline summary.
Plan line: data-plan section 4 one-door + section 6 tick-state emission."""
import glob, json, os
rows, tot = [], {"n_bytes":0,"wall":0,"breaks":0,"recompl":0,"healed":0,"snapshots":0,"state_mb":0.0}
for rp in sorted(glob.glob(os.path.expanduser("~/engine/hcp/engine/kernel/corpus-pour-v0/pour-doc*-report.json")),
                 key=lambda p:int(p.split("doc")[1].split("-")[0])):
    r=json.load(open(rp)); a=r["acceptance"]; f=r["final"]
    d=r.get("one_door",{}) or {}
    idx=r.get("tick_states"); ns=0; mb=0.0
    if idx and not os.path.isabs(idx): idx=os.path.join(os.path.expanduser("~/engine/hcp/engine/kernel"), idx)
    if idx and os.path.exists(idx):
        ix=json.load(open(idx)); ns=len(ix["snapshots"]); mb=sum(s["n_bytes"] for s in ix["snapshots"])/1e6
    rows.append({"doc":(d.get("doc") or {}).get("name","?"),"ledger_event":d.get("ledger_event_id"),
        "n_bytes":r["n_bytes"],"verdict":a["verdict"],"drift":a["identity_given_drift"],
        "perm_death":a["existence_permanent_death"],"breaks":f["cum_breaks"],"recompl":f["cum_completions"],
        "healed_pairs":r["healed_pairs_total"],"healed_both_light":r["healed_pairs_both_light"],
        "bond_survival":f["bond_survival"],"mean_det":f["mean_det"],"frac_locked":f["frac_locked"],
        "wall_s":round(r["wall_seconds"],1),"sha_verified":r.get("sha_verified_against_ledger"),
        "snapshots":ns,"state_mb":round(mb,1)})
    tot["n_bytes"]+=r["n_bytes"]; tot["wall"]+=r["wall_seconds"]; tot["breaks"]+=f["cum_breaks"]
    tot["recompl"]+=f["cum_completions"]; tot["healed"]+=r["healed_pairs_total"]
    tot["snapshots"]+=ns; tot["state_mb"]+=mb
out={"artifact":"corpus-pour-v0 baseline — 9 fic_pbm docs through the one door (cuda f32, 200 ticks, state-every 20)",
     "plan_line":"data-plan section 1.4 grain + section 4 one-door + section 6 tick-state emission",
     "docs":rows,"totals":{k:(round(v,1) if isinstance(v,float) else v) for k,v in tot.items()},
     "all_pass":all(r["verdict"]=="PASS/PASS" for r in rows),
     "all_sha_verified":all(r["sha_verified"] for r in rows)}
p=os.path.expanduser("~/engine/hcp/engine/kernel/corpus-pour-v0/corpus-baseline-report.json")
json.dump(out,open(p,"w"),indent=1); print(json.dumps(out["totals"]));
print("all_pass",out["all_pass"],"all_sha_verified",out["all_sha_verified"],"->",p)
