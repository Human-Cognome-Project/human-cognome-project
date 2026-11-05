# Private native exchange accounting

Replacement staging now reconciles physical and exact source mass, checks native
momentum, and reports the actual generated centre and its offset from the outgoing
target. This extends [replacement staging](EXCHANGE-PLACEMENT.md), not publication.
The [logical binding view](EXCHANGE-BINDINGS.md) still requires coordinator wiring;
passing scalar accounting does not authenticate incoming source participation.

## Exact mass domain

Each selected committed source mass and each incoming command's `exact_mass` must
be a positive integer, with each selected/incoming total at most `2^53`. Host
integer checks precede upload. Native reduction compares each outgoing physical
mass with its exact source integer converted to f64. Incoming intrinsic masses
must equal command masses; the accounting kernel compares generated row masses
with those requests and their sum with the outgoing total. These positive integer
casts and sums are exact: there is no mass tolerance. Unknown or larger outgoing
source masses return Unsupported; invalid incoming metadata returns InvalidInput;
native mismatch returns InvalidNumerics without successful placements.

The standalone outgoing-state diagnostic retains its broader numerical domain.
Reconciliation is explicitly enabled by exchange staging, not inferred from a
successful diagnostic query. Fractional physical mass is not qualified here.

## Momentum bound

Strict f64 kernels retain outgoing raw momentum `Sout` and the sum of absolute
weighted velocities `Lout`, per axis. After certified placement and initialization,
the balance kernel visits incoming rows in canonical request order, checks that
their velocity equals the native transferred velocity, and computes `Sin`, `Lin`.
No host numerical reconstruction or positional significance filter sets velocity.

With prepared particle capacity `P`, incoming body count `N`, and `u=2^-53`, set

```
K = 16 * (P + N + 4) * u
R = K * (Lout + Lin + abs(Sout) + abs(Sin))
r = Sin - Sout
```

All displayed quantities are computed natively. Acceptance requires finite,
supported values and `abs(r) <= R` on each axis. This is an arithmetic bound, not
a physical activity epsilon. Strict compiler checks reject contracted/fast math.
`R` bounds the accepted computed residual; it is not by itself a claim that the
exact real-valued dot products differ by at most `R`.

For the supported domain, an accepted residual bounds the difference between the
exact dot products of the stored input/output values by `2R`. The usual
`gamma_(2n) = 2n*u/(1-2n*u)` product/sequential-sum bound applies separately to
each dot product and its positive absolute-product sum. Prepared `P,N <= 2^30`
keep these factors small; the factor 16 leaves margin for both dot-product errors,
the rounded positive scale sum, multiplication by K and residual subtraction.
Each dot-product error is bounded conservatively by `4*n*u` times its computed
absolute-product sum. Their combined error is less than half the displayed R;
subtraction rounding plus accepted residual then remains below `2R`.
When R is zero, all weighted velocities are zero and equality is exact.

Outgoing operand guards and integer masses keep nonzero products, cancellation
remainders and divided velocity normal in the fork's FTZ runtime. Original
velocities have magnitude at least `2^-256`; their weighted-sum lattice is no
finer than `2^-308`, and division by mass at most `2^53` leaves a nonzero velocity
above the subnormal range. Upper bounds likewise avoid overflow. Generated values,
residual and bound are explicitly checked. This is a bounded f64 CPU qualification,
not exact momentum arithmetic or a guarantee for unsupported mass ranges.

## Centre and diagnostic layout

The kernel computes `sum(m*x)/sum(m)` from the actual certified stored replacement
coordinates, then subtracts the outgoing target. Nearest-free placement may give
a nonzero offset. The centre is a strict-f64 diagnostic subject to ordinary
weighted-sum/division error, not an interval certificate or a centre-preserving
packing constraint. Existing eight-double outgoing state still contains target
XYZ, transferred velocity XYZ, mass and status.

The additional 32-double attempt-local report contains:

| Index | Meaning |
|---|---|
| 0–2 | Outgoing raw momentum |
| 3–5 | Outgoing sum of absolute weighted velocity |
| 6 | Incoming total mass |
| 7–9 | Actual incoming centre |
| 10–12 | Actual centre minus outgoing target |
| 13–15 | Incoming raw momentum |
| 16–18 | Computed momentum residual |
| 19–21 | Per-axis bound R |
| 22 | Native status, zero on success |
| 23–31 | Reserved zero |

Failed scratch is not consumable; successful scratch is valid only within the
immediate private attempt under exclusive ownership. No public token is added.

## Resources and remaining integration

Preparation adds `32*sizeof(double)` report bytes, `P*sizeof(uint64_t)` source-mass
bytes, two ndarray controls and one six-argument compiled/bound/warmed kernel.
The reducer gains three arguments. Manifest control/binding accounting includes
these additions. Exchange prefix work becomes
`64*(P+1) + commands + 64*bodies + 128`, in addition to normal staging work.
Successful accounting adds 256 diagnostic readback bytes. Scratch writes do not
change source/history/activity/contact state, frame, generation or tick count.
Ordinary rejection permits retry; unexpected execution failure makes the owner
unready under the existing rule.

Logical incoming command/source checks, prospective source/hierarchy/contact
coordination, total enduring-owner resources, coherent mapping lifecycle and
atomic replacement/retirement are still outstanding. No operational or GPU
qualification is claimed. [Source-pinned evidence](../tests/admission/README.md)
covers mass mismatch, missing/unsupported exact source metadata, momentum/velocity
corruption, fractional weighted velocity, nonzero expansion offset, retry and
unchanged live state/backing. The compiler audit now checks 19 kernels and 29
unsafe controls per position profile.
