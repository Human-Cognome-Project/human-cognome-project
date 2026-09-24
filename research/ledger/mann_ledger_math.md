# The Mann Ledger: Mathematical Companion

**What was removed, where it sat, and what the formula reduces to.**

This is the working for the procedure in `mann_ledger_writeup.md`. It is written for a reader who can follow an equation and wants to check each move.

## 1. Planck's formula as published

Energy per unit volume per unit frequency interval, in a cavity at temperature T (Planck 1901, Ann. Phys. 4:553):

    u(ν, T) = ( 8π ν² / c³ ) · ( hν ) · 1 / ( exp(hν / kT) − 1 )

It is a product of three factors, each with its own origin:

| factor | what it is | where it comes from |
|---|---|---|
| 8π ν² / c³ | number of standing-wave modes per unit volume per unit frequency | geometry of a three-dimensional box (Rayleigh 1900, Phil. Mag. 49:539; Planck's resonator electrodynamics, 1899) |
| hν | energy assigned to one quantum of a mode at frequency ν | Planck's energy-element hypothesis, ε = hν (1901) |
| 1 / (exp(hν/kT) − 1) | mean number of quanta occupying a mode at temperature T | Planck's entropy count over energy elements (1901), which is where k was introduced |

Where the constants sit:

- **c** appears once, in the mode count, because modes are counted per wavelength and converted to per frequency.
- **k** appears once, in the exponent, setting the ratio of thermal energy to quantum energy.
- **h** appears **twice**: once as the energy per quantum (a linear factor on ν), and once inside the exponential (bending the linear rise into the measured curve).

That double appearance of h is the fact the whole procedure turns on.

## 2. The two limits show what each h does

**Low frequency** (hν much smaller than kT). The exponential expands, exp(x) − 1 ≈ x, and the formula becomes

    u ≈ ( 8π ν² / c³ ) · kT

This is the Rayleigh-Jeans law. **h has vanished entirely.** The two copies of h cancel each other. What is left is the geometric mode count (ν²) times the classical energy per mode (kT). The square on frequency is fully present with no h anywhere.

**High frequency** (hν much larger than kT). The −1 is negligible and the formula becomes

    u ≈ ( 8π h / c³ ) · ν³ · exp( −hν / kT )

This is Wien's law. Here both copies of h are visible at once: one multiplying ν³ in front, one in the exponent. The ν³ is ν² (mode count) times ν (energy per quantum).

So: the first h sets the slope of energy against frequency; the second h, in the exponent, is what turns that slope over. The classical square (ν²) belongs to neither; it is the box.

## 3. Step 2 of the procedure: c = k = 1, then h = 1

With c and k set to 1, frequency and temperature are in the same unit and the formula is

    u = 8π ν² · hν / ( exp(hν / T) − 1 )

Setting h = 1:

    u = 8π ν³ / ( exp(ν / T) − 1 )

This changes the **shape of nothing**. h appears only in the combination hν/T, so its numerical value only fixes what number on the frequency axis corresponds to a given temperature. The peak sits at ν = 2.821 T for any value of h (Wien displacement, frequency form). The spectrum's curve, its width, its integrated area relative to T⁴, all are identical. This is why h is a step declaration and not a physical constant: it names the unit, and a name is set by convenience. (Verified numerically: spectra plotted with h = 1 and with h = 6.626 × 10⁻³⁴ are the same plot with the axis relabelled.)

## 4. Step 3: cut the weld

The weld is the factor hν, "energy per quantum equals frequency." Dividing it out of u leaves a formula that counts quanta instead of summing energy:

    n(ν, T) = u / (hν) = 8π ν² / ( exp(ν / T) − 1 )

This is the photon number spectrum (standard; e.g. Rybicki and Lightman, *Radiative Processes in Astrophysics*, §1.5). Every quantity in it is now a count or a ratio: ν² counts modes, the denominator counts occupancy, ν/T is a ratio of like units. There is no energy in it and no h. **Frequency and power have been separated at the formula level.** The power ledger is what you get by multiplying n back by an amount; the frequency ledger is n itself.

## 5. Step 3 continued: remove the square that remains

What is left in n is a single square on frequency, ν². It is the mode count, and it has a named source: three spatial dimensions of box give a spherical shell of states in wave-number space, whose surface area goes as ν². That is the square "attached to frequency itself."

On the ledger the 36 measured values sit as positions on a frequency axis. The mode-count square means that each position is carrying a geometric factor of ν² relative to a one-dimensional count. Taking the square root once on the whole axis strips that factor from every position uniformly. Because it is a named square with a located source, the operation has warrant. Because there is no second named square, a second root does not.

## 6. Step 4: remove the second copy of h's magnitude

Step 3 dealt with h algebraically (set to 1) and the axis geometrically (root). But the 36 measured numbers were originally read against SI, where h carries a magnitude of 6.626 × 10⁻³⁴, and that magnitude entered the ledger positions twice: once through the conversion of the 12 amount-side reads onto the frequency axis (E/h), and once again by the weld being applied in reverse to make the single line in step 1. The root in step 3 removes one copy of that magnitude from the positions. The remaining copy is removed by multiplying every position by h once. It is a scalar on a linear axis: a step change, warrant-free, ratio-preserving. After it, there is no h on the ledger in any form, magnitude or symbol.

## 7. What the formula reduces to

| starting point | after the procedure |
|---|---|
| u = (8πν²/c³) · hν / (exp(hν/kT) − 1) | n = 8π ν² / (exp(ν/T) − 1) as the frequency-side object; amount is a separate ledger, added back only when a measured amount exists |
| three constants h, c, k | none; c and k were unit names, h was a unit name used twice |
| ν³ in the Wien limit | ν² (box geometry), then ν after the ledger root |
| energy per unit volume per unit frequency | count of oscillations per unit volume per unit frequency |

## 8. The separation rule, stated as an operation

For any inherited formula F that contains both a frequency ν and an amount E joined by h:

1. Write F as a product of factors and identify which are counts/ratios (frequency side) and which are amounts (power side).
2. Set every conversion constant between the two sides to 1. Their magnitudes are unit names.
3. Divide out the weld factor hν wherever it converts a count into an amount.
4. Keep the frequency-side factors on a log axis; keep the amount-side factors on a linear axis; never recombine them by a constant.
5. If a square on frequency remains and its source can be named, remove it once by a root. If it cannot be named, leave it.

The result is two formulas, one for where things sit and one for how much is there, each containing only the dimensions that belong to it.

## Sources

- Planck, M. (1899). Sitzungsberichte Preuss. Akad. Wiss., pp. 440–480. Resonator electrodynamics.
- Planck, M. (1901). Ann. Phys. 4:553. Energy elements, entropy count, introduction of h and k.
- Rayleigh, Lord (1900). Phil. Mag. 49:539. Mode count, ν² per unit volume.
- Rybicki, G. B. and Lightman, A. P. (1979). *Radiative Processes in Astrophysics*. Wiley. §1.5, photon number density.
- CODATA 2022 for all SI values used in `scale_map_sources.md`.
