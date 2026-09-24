# Attraction Without Frequency: Two Imbalances in a Continuous Field

**Working note, Human Cognome Project. 28 August 2026.**
Companion to `mann_ledger_writeup.md` and `mann_ledger_math.md`. Model code: `field_model.py`. Figure: `field_attraction.png`.

## The claim

Matter is a density imbalance in a continuous field, bounded by the same field. The attraction between any two pieces of matter is the field reducing the net differential across the combined structure. It is a product of the two imbalances, factored quadratically by the distance between them. No frequency, no signaling, no synchronization is involved. Frequency belongs to whatever samples the situation, not to the situation.

## The model

Two zones of surplus density, each a smooth bump of strength D and width s, sit in a field that relaxes by isotropic diffusion. Diffusion is parabolic (Fourier 1822; Fick 1855): it has no wave solutions, and the advancing edge of a relaxing imbalance is a flux pattern with a source, not a waveform. At steady state the relaxed field obeys Laplace's equation with the zones as sources. The force on zone 2 is its own imbalance multiplied by the gradient of zone 1's relaxed field, integrated over zone 2.

Units are arbitrary throughout. Only ratios are reported.

## Results

From the run (`field_model.py`, 40 separations from 0.5 to 60 zone widths, 25,600 sample points per evaluation):

| test | result |
|---|---|
| far-field slope of attraction against separation | −2.00 |
| attraction with D₁ = 2, D₂ = 3 relative to D₁ = D₂ = 1, at d = 20 | 6.0 |
| ratio to pure D₁D₂ / (4π d²) at d = 4, 8 | 0.94, 0.90 (Monte Carlo noise) |
| ratio at d = 1, 2 | 0.08, 0.43 |

The far field is exactly inverse square in distance and exactly proportional to the product of the imbalances. Inside about three zone widths the curve turns over: the zones overlap, the differential is already being reduced by merging, and the notion of a pull between two separate things stops applying. That turnover is the plates fusing.

## Why it comes out this way

The steady state of isotropic diffusion is Laplace's equation. Its point-source solution in three dimensions falls as 1/r, and the gradient of that falls as 1/r² (Carslaw and Jaeger, *Conduction of Heat in Solids*, 2nd ed., §10.2; equivalently Gauss's law, since the flux through any closed surface around a source is fixed and the surface area grows as r²). The exponent 2 is the number of dimensions the flux spreads across, minus one. It is geometry, and it is the only place a square appears in the derivation.

## What it reconciles

**1. Newton's law of gravitation.** F = G m₁m₂ / r². Read against the model: m is the imbalance, G is the step declaration converting field units to the chosen mass and force units, and 1/r² is three-dimensional spreading. The model produces the law with G set to 1/(4π) and nothing else assumed. Newton's own remark that he framed no hypothesis for the cause of the inverse square (*Principia*, General Scholium, 1713) is answered: the cause is the field relaxing across three dimensions.

**2. Coulomb's law.** F = k q₁q₂ / r². Same run with a sign rule: imbalances of like sign repel, of unlike sign attract. The identical form of the two laws, which physics treats as a coincidence of two separate forces, is here one law with two sign conventions.

**3. The Casimir sign reversal.** Lifshitz theory (Lifshitz 1956, Sov. Phys. JETP 2:73; Dzyaloshinskii, Lifshitz and Pitaevskii 1961, Adv. Phys. 10:165) predicts, and Munday, Capasso and Parsegian measured (2009, Nature 457:170), that two bodies across a medium whose response lies between theirs repel rather than attract. In the model's terms: two dense zones with a zone of intermediate density between them have no differential to reduce by merging; the ordering is already the minimum. Attraction appears only when merging lowers the net differential. The ping-pong-ball picture cannot produce a repulsion; the differential picture requires it.

**4. The word "vacuum".** Casimir's 1948 calculation (Proc. KNAW 51:793) computes an infinite energy for the space between the plates, subtracts it against another infinity, and keeps the finite boundary-dependent remainder. Jaffe (2005, Phys. Rev. D 72:021301) showed the force can be derived entirely from the plates' charges with the zero-point energy set to zero, and that the force vanishes as the coupling goes to zero. Both facts say the same thing as the model: the effect belongs to the dense zones, and the space between them is a low-density region of the same field, not an absence. A mathematical vacuum admits no boundary and no mode; nothing can be predicated of it. The physical "vacuum" is field.

**5. Heat, and any continuous radiation, as sampled state.** The model contains no period. If a sampler reads the field at intervals, it registers a state each time; change appears only as modulation between reads, and the frequency on the record belongs to the sampler. A continuous, dense input registers on every sample and was historically labeled high-frequency for that reason. FIRAS, the instrument that pinned the thermal spectrum to 50 parts per million (Fixsen et al. 1996, ApJ 473:576), is an interferometer that samples path difference and Fourier-transforms the record: the frequency axis of the cosmic background is the instrument's decomposition of an aperiodic input.

## Polarity as flow

Every body is bound by electromagnetic response, so every body carries a continuous internal flow that rebalances without stopping. For a metal that flow is the conduction band; gold's plasma edge sits near 9 eV (Johnson and Christy 1972, Phys. Rev. B 6:4370), on the same Mann rung as the chemical bonds and the visible lines. Bring two bodies together and each flow adjusts to the other's presence; the adjustment in one is opposite in sense to the adjustment in the other (image charge, in the inherited vocabulary). Opposite senses reduce the combined differential, so they attract. Lifshitz theory computes the force from exactly these response functions and nothing else (Lifshitz 1956).

Charge sign is therefore the sense of a flow across a structure, not a kind of substance. Two bodies whose biases are unlike present unlike faces to the gap; there is a differential between them, merging reduces it, and the field draws them together. Two bodies whose biases are alike present the same face to the gap; there is no differential between them to reduce, and merging would concentrate the shared bias and raise the net imbalance against the surrounding field, so the field keeps them apart. Attraction and repulsion are one rule, the field lowering total imbalance, applied to the two cases. A medium between the bodies with its own flow sense can invert which case applies (Dzyaloshinskii, Lifshitz and Pitaevskii 1961; Munday, Capasso and Parsegian 2009). Gravity has one sign because it is read across the field floor, below which there is no medium and no bias, so every body presents a plain surplus and the opposing sense is never interrupted.

How much of any of this can be expressed is capped. The differential a body can present is bounded by the energetic limits of its band in its medium: a dielectric saturates, a conductor's response ends at its plasma edge, a medium breaks down at a field limit. The reconcilable difference between two bodies is therefore bounded above by the smaller of their bands, whatever their nominal imbalances. The lineage for reading gravity this way runs through Sakharov 1967 (Dokl. Akad. Nauk SSSR 177:70) and Puthoff 1989 (Phys. Rev. A 39:2333); Puthoff's attempt failed on the distance exponent (Carlip 1993, Phys. Rev. A 47:3452), which the next section resolves.

## The exponent ladder

The inverse square of the first run assumes one-way balancing: a surplus in the field of another surplus. As soon as two bodies interact, their adjustments overlap in the middle, and computing the joint result compounds the quadratic. Each level of mutual adjustment folded into the calculation adds to the exponent:

| configuration | what each body sees | law | source |
|---|---|---|---|
| surplus in the field of a surplus | the field itself | 1/r² | first run; Newton, Coulomb |
| balanced body in the field of a surplus | only the differential of the field (its gradient) | 1/r³ | dipole-charge |
| two balanced bodies, each adjusting to the other's adjustment | product of two responses | 1/r⁶ energy, 1/r⁷ force | London 1930, Z. Phys. 63:245 (second-order perturbation, i.e. mutual overlap) |
| the same, with the adjustment's propagation delay included | one more overlap | 1/r⁷ energy, 1/r⁸ force | Casimir and Polder 1948, Phys. Rev. 73:360 |
| two parallel plates of the last kind, summed over their elements | pairwise 1/r⁸ integrated over two half-spaces | 1/d⁴ per unit area | Hamaker 1937; Lamoreaux 1997 measured |

The higher powers are not additional forces. They are the geometric quadratic compounded by however many mutual adjustments the calculation folds into a single number. The plate law is the bottom row: the measured 1/d⁴ is the two-body 1/r⁸ summed over plate geometry, and 1/r⁸ is inverse square with three overlaps counted.

The compounding is not confined to the bodies. Any energetic state of the field itself, independent of the bodies, enters the same way. Lifshitz theory carries an intervening medium as a third response function; the temperature of the field is another. At separations beyond the field's thermal wavelength the plate force changes from 1/d⁴ to 1/d³, because the field's own thermal density takes over the interaction (Bordag, Klimchitskaya, Mohideen and Mostepanenko 2009, Rev. Mod. Phys. 81:1827; measured by Sushkov, Kim, Dalvit and Lamoreaux 2011, Nature Phys. 7:230). The field's state changes the exponent, not only the coefficient.

## The view and the quadratic are inverses

From a fixed vantage at height d above a surface, a spot at lateral offset x is seen at angle θ = arctan(x/d). As the spot approaches, passes underneath and recedes, θ runs smoothly from most lateral to most vertical and back. The rate of that change is dθ/dx = d/(d² + x²): an inverse quadratic in the offset, peaking directly beneath the observer and falling symmetrically either side. The view is the integral of the perspective quadratic; the quadratic is the slope of the view. Each is the other's inverse operation. (Statistics carries the same pair as the Cauchy density and its arctangent cumulative.)

This is the shape of the model's near field: the gradient of a passing source is this curve, and the turnover inside three zone widths in `field_attraction.png` is its peak. It is also why an inverse-square field, sampled by an observer moving past it, reads as a smooth rise and fall with no discontinuity anywhere: the sampler sees the integral, and the integral of an inverse quadratic is bounded and smooth.

## Propagation: one law for the middle and the front

The first run used linear diffusion, which is Fick's law with a constant diffusivity: flux = D ∇φ. It gives the rate at which imbalance moves through any point as proportional to the differential between origin and point, falling with distance as the gradient does, 1/r² for a point source. That is the mid-state, and it is the same exponent as the attraction.

Linear diffusion has no front: the disturbance is nonzero everywhere from the first instant, and its characteristic radius grows as √(Dt) regardless of magnitude. A field that is denser where it is more imbalanced does not behave that way. Writing the diffusivity as a function of the local density, flux = D(φ) ∇φ with D rising with φ, gives the porous-medium equation (Barenblatt 1952; Vázquez, *The Porous Medium Equation*, 2007). It has a finite front, the front moves at a speed set by the magnitude arriving at it, and in the interior where φ is near the floor it reduces to linear Fick. One dependency added, no second law: the speed of propagation of any imbalance toward diffusion at a point is set by the magnitude expressed between origin and that point, and it falls off with the same exponent as the diffusion itself.

## What remains

- **Magnitude.** The ladder fixes every exponent. The coefficient at each rung is where the boundary ratio (the imbalance set at each body's boundary with the field) enters, and it has not been specified numerically. Lamoreaux's five-percent number is the target.
- **Which rung dominates at a given separation.** Every body carries a plain surplus and an internal flow at once. The crossover between the surplus law and the flow laws as separation changes is a boundary-ratio question and is not yet computed.
- **Retardation.** The rung that adds propagation delay uses c, which on this ledger is the field's change-propagation rate set to 1. With density-dependent diffusivity that rate is itself a function of local density, so the retardation rung and the propagation section above are one calculation. It has been argued, not run.

## Files

- `field_model.py`: the run. Two Gaussian sources, analytic steady field, Monte Carlo integration of the gradient over the second zone.
- `field_attraction.png`: attraction against separation, log-log, with the inverse-square reference and the merging region marked.
