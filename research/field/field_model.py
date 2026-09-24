# Two dense zones in a continuous field. Field relaxes by isotropic diffusion (parabolic);
# at steady state the density perturbation phi obeys Laplace with the zones as sources.
# Zone i = Gaussian imbalance of strength D_i and width s. Force on zone 2 = -D_2 * grad(phi_1) integrated over zone 2.
# No frequency anywhere. Units arbitrary; only ratios reported.
import numpy as np
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
from scipy.special import erf

s = 1.0                                  # zone width
def phi(r, D):                           # steady field of a Gaussian source in 3D (Poisson Green's fn, Gaussian smoothed)
    r = np.maximum(r, 1e-9)
    return D * erf(r/(s*np.sqrt(2))) / (4*np.pi*r)
def force(d, D1, D2, n=160):
    # integrate -D2 * dphi1/dx over zone 2's Gaussian weight (Monte Carlo over the Gaussian)
    rng = np.random.default_rng(0)
    pts = rng.normal(0, s, (n**2, 3)) + np.array([d,0,0])
    r = np.linalg.norm(pts, axis=1); x = pts[:,0]
    h = 1e-4
    dphidx = (phi(np.linalg.norm(pts+[h,0,0],axis=1),D1) - phi(np.linalg.norm(pts-[h,0,0],axis=1),D1))/(2*h)
    return abs(D2*dphidx.mean())   # magnitude along the line joining the zones

ds = np.logspace(np.log10(0.5), np.log10(60), 40)
F = np.array([force(d,1,1) for d in ds])
far = ds > 6
slope = np.polyfit(np.log(ds[far]), np.log(F[far]), 1)[0]
print("far-field slope of |F| vs d :", round(slope,3))
print("product test: F(D1=2,D2=3)/F(1,1) at d=20 =", round(force(20,2,3)/force(20,1,1),3))
print("direction: along the line joining the zones (gradient of the other zone's field)")
print("near/far ratio to pure inverse square at d=1,2,4,8:", [round(F[np.argmin(abs(ds-d))]*d**2*4*np.pi,3) for d in (1,2,4,8)])

fig,ax=plt.subplots(figsize=(8,5.5))
ax.loglog(ds, F, "o", ms=4, label="model: two dense zones, continuous field")
ax.loglog(ds, 1/(4*np.pi*ds**2), "--", color="0.5", label="D₁D₂ / (4π d²)")
ax.axvspan(0.5, 3*s, color="0.9", label="zones overlap (merging)")
ax.set_xlabel("separation d  (zone widths)"); ax.set_ylabel("|attraction|  (arbitrary)")
ax.set_title(f"Attraction between two imbalances, no frequency term.  far-field slope = {slope:.2f}")
ax.legend(); ax.grid(alpha=.3)
plt.tight_layout(); plt.savefig("/mnt/user-data/outputs/field_attraction.png", dpi=160)
