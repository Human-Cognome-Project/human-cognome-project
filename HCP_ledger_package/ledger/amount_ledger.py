import numpy as np
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
exec(open("amount_ledger.py").read().split("print(f\"{'item'")[0])   # reuse A, cal, NA etc.
NW = NA/18.015   # molecules in 1 g water: the count event
cols={"unit":"k","bio":"#2a9d8f","geo":"#8d6e63","therm":"#f4a261","chem":"#457b9d","part":"#6a4c93"}
pts=[]
print(f"{'item':32s} {'Mann per event':>15s}   event")
for lab,cls,J,ev,src in A:
    m=J/cal
    if ev in ("per molecule","per electron","per bond","per atom","per particle"):
        m*=NW; ev="per water-gram count (3.34e22)"
    if "Hearing" in lab: continue   # per-cycle read; belongs on the RP list, not this strip
    pts.append((lab,cls,m,ev)); print(f"{lab:32s} {m:15.3e}   {ev}")

fig=plt.figure(figsize=(16,5.2)); gs=fig.add_gridspec(1,2,width_ratios=[10,1.2],wspace=0.04)
ax=fig.add_subplot(gs[0]); bx=fig.add_subplot(gs[1])
for a,lo,hi in ((ax,3e-2,3e15),(bx,3e46,5e47)):
    a.set_xscale("log"); a.set_xlim(lo,hi); a.set_ylim(-1.7,1.7); a.axhline(0,color="0.7"); a.set_yticks([]); a.grid(True,axis="x",alpha=.25)
i=0
for lab,cls,m,ev in pts:
    a = bx if m>1e40 else ax
    a.scatter(m,0,s=80,color=cols[cls],edgecolor="k",lw=.6,zorder=3)
    y=(0.4+0.3*((i//2)%4))*(1 if i%2 else -1); a.plot([m,m],[0,y*.85],color="0.7",lw=.4)
    a.text(m,y,lab,fontsize=7.2,ha="center",va="center",color=cols[cls]); i+=1
ax.spines["right"].set_visible(False); bx.spines["left"].set_visible(False); bx.set_yticks([])
for a,xy in ((ax,(1,0)),(bx,(0,0))):
    a.plot([xy[0]-.01,xy[0]+.01],[-.03,.03],transform=a.transAxes,color="k",clip_on=False,lw=1)
bx.text(0.5,0.12,"AXIS BREAK\n31 decades omitted\nsystem-scale event;\nenergy inferred from strain\n(possible scaling issue)",transform=bx.transAxes,ha="center",va="center",fontsize=7,color="0.3")
ax.set_xlabel("Mann (amount).  1 Mann = 1 g water raised 1 K; count event = molecules in 1 g water.   [log display of a linear ledger]")
ax.set_title("Amount ledger from independent reads.  Break at right is a declared device.",loc="left",fontsize=11)
plt.savefig("/mnt/user-data/outputs/amount_ledger.png",dpi=130,bbox_inches="tight")
