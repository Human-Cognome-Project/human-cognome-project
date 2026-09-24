# The Mann Ledger: Separating Frequency from Power

**Working note, Human Cognome Project. 28 August 2026.**

## What this is

Physics joins two different kinds of quantity with one equation: energy equals Planck's constant times frequency. Frequency is a rate, a how-often. Energy is an amount, a how-much. The equation treats them as the same thing measured in different units. This note records how that join was taken apart for 36 measured values, what the values look like once it is gone, and the working unit declared on the result.

## The rule behind every step

A result can only come from the things that go into it. If a number in a formula has nothing behind it, no measurement and no derivation, it does not constrain anything. It is a label, and a label can be set to whatever is convenient.

Two kinds of operation are allowed on the ledger:

- **Changing the step.** Multiplying everything by the same number, or renaming the unit. This changes nothing about how the values sit relative to each other. No justification needed.
- **Removing a square.** This touches every dimension at once. It is only allowed when the square being removed has been identified by name and located in the formula that put it there.

## What went in

Thirty-six values, each with at least one direct measurement behind it. Anything set by convention or produced by a model was left out: radio and microwave band edges, mains and clock frequencies, concert pitch, the expansion rate of the universe, its age, the Planck-scale units, the temperature of the Sun's core. The measured quantity and its source for every entry is in `scale_map_sources.md`.

Twenty-four of the 36 were measured as a frequency (or as a wavelength, which is the same read). Twelve were measured as an amount: four chemical energies, six particle masses, two temperatures.

## The steps, in order

**1. Put everything on one line the way physics does.** Frequencies were turned into energies by Planck's equation. Amounts were turned into frequencies by the same equation run backwards, with masses first turned into energy by Einstein's equation and temperatures by Boltzmann's. This is the join. It was done first so it could be undone on the record.

**2. Set the conversion constants to one.** Planck's, Einstein's and Boltzmann's constants all become 1. Joules, kilograms and kelvins collapse onto the frequency count. Each of the 36 values is now a single number on a single line. This is a step change; it moves nothing relative to anything else.

**3. Take out the square.** Planck's radiation formula carries frequency squared from his count of how many ways a box can hold a wave, and then frequency once more from the energy-per-wave term. That square was named and located, so it is removed from the whole line by taking the square root once. No second root is permitted, because no second square has been named.

**4. Take out what caused the square.** The square in step 3 is there because Planck's constant is used twice in the same formula: once to set how steeply energy rises with frequency, and once more inside an exponential to bend that straight rise into the curve that matches the measured spectrum. Used twice, it is squared through the product. The root in step 3 removed one copy. The other copy is removed here by applying the constant once, as a plain multiplier, to everything. Nothing moves relative to anything else.

**5. Declare the unit from the shape of the ledger.** The base was placed at a round rung, not on any one measurement, because no single read is trusted over the others. The rung chosen falls inside the range of human hearing: the lowest audible tone sits at 0.12, the highest at 3.6, and every heartbeat, breath and brainwave sits below 1. From there it is thirteen whole steps up to the highest energy ever measured. The rung is named **1 Mann**. The name it had on the old axis is retired.

Steps 2 through 5 leave every ratio between every pair of values exactly as measurement gave it. Nothing was smoothed, shifted or fitted. The only thing that changed is the axis they sit on.

## The ledger

| # | item | class | measured | Mann |
|---|---|---|---|---|
| 1 | Resting breath (15/min) | bio | 0.25 Hz | 0.0129 |
| 2 | Resting heart (72 bpm) | bio | 1.2 Hz | 0.0282 |
| 3 | EEG delta (0.5-4 Hz) midpt | bio | 2 Hz | 0.0364 |
| 4 | EEG theta (4-8 Hz) midpt | bio | 6 Hz | 0.0631 |
| 5 | Schumann fundamental | geo | 7.83 Hz | 0.072 |
| 6 | EEG alpha (8-13 Hz) midpt | bio | 10.5 Hz | 0.0834 |
| 7 | Human hearing floor | bio | 20 Hz | 0.115 |
| 8 | EEG beta (13-30 Hz) midpt | bio | 21.5 Hz | 0.119 |
| 9 | EEG gamma (30-100 Hz) midpt | bio | 65 Hz | 0.208 |
| 10 | GW150914 peak frequency | geo | 250 Hz | 0.407 |
| 11 | Human hearing ceiling | bio | 20000 Hz | 3.64 |
| 12 | Lamb shift (H 2S-2P) | em | 1.05784e+09 Hz | 837 |
| 13 | H 21 cm hyperfine | em | 1.42041e+09 Hz | 970 |
| 14 | Cs-133 hyperfine (SI second) | em | 9.19263e+09 Hz | 2.47e+03 |
| 15 | kT, cosmic background 2.725 K | therm | 2.72548 K | 6.13e+03 |
| 16 | kT, human body 310 K | therm | 310.15 K | 6.54e+04 |
| 17 | ATP hydrolysis (std) | chem | 30.5 kJ/mol | 2.25e+05 |
| 18 | kT, solar photosphere 5772 K | therm | 5772 K | 2.82e+05 |
| 19 | Sr-87 optical clock | em | 4.29228e+14 Hz | 5.33e+05 |
| 20 | H-alpha 656.28 nm | em | 656.281 nm | 5.5e+05 |
| 21 | Na D2 589.0 nm | em | 588.995 nm | 5.81e+05 |
| 22 | Cs work function | chem | 2.14 eV | 5.86e+05 |
| 23 | H-H bond (436 kJ/mol) | chem | 436 kJ/mol | 8.51e+05 |
| 24 | Th-229 nuclear clock isomer | em | 8.35573 eV | 1.16e+06 |
| 25 | H 1S-2S two-photon | em | 2.46606e+15 Hz | 1.28e+06 |
| 26 | Lyman-alpha 121.57 nm | em | 121.567 nm | 1.28e+06 |
| 27 | H ionization 13.598 eV | chem | 13.5984 eV | 1.48e+06 |
| 28 | Cu K-alpha X-ray | em | 8047.8 eV | 3.59e+07 |
| 29 | Electron rest energy | part | 510999 eV | 2.86e+08 |
| 30 | Co-60 gamma 1.332 MeV | em | 1.3325e+06 eV | 4.62e+08 |
| 31 | Muon rest energy | part | 1.05658e+08 eV | 4.11e+09 |
| 32 | Proton rest energy | part | 9.38272e+08 eV | 1.23e+10 |
| 33 | Z boson | part | 9.1188e+10 eV | 1.21e+11 |
| 34 | Higgs boson | part | 1.252e+11 eV | 1.42e+11 |
| 35 | Top quark | part | 1.7257e+11 eV | 1.66e+11 |
| 36 | LHAASO 1.4 PeV photon | em | 1.4e+15 eV | 1.5e+13 |

Classes: bio = body and brain; geo = earth and gravity; em = light and other radiation; chem = chemical and atomic energies; therm = heat (all three placed at kT; heat has no frequency of its own, so no spectral-peak factor is used); part = particle masses.

What the ledger shows, all of it from measurement:

- Body and brain rhythms sit between 0.013 and 0.21 Mann. Hearing runs from 0.12 to 3.6.
- Between 3.6 and 837 Mann there is nothing measured. Everything people put in that range, radio and clocks and mains power, was assigned.
- Between 230,000 and 1,500,000 Mann, heat, chemistry and light all land together: body temperature, the energy that runs cells, the surface of the Sun, every visible colour, every chemical bond, the ionization of hydrogen. It is the only place on the ledger where three different kinds of measurement share a rung.
- The particle masses run from 290 million to 170 billion Mann. The most energetic light ever detected sits at 15 trillion.

## A caution on the amount side

The Mann values above are rational as positions. As amounts they are not yet trustworthy, and heat is the clearest case of why.

Every amount on the ledger was measured through a sampler with a cycle. A sampler integrating over its window returns whatever arrived in that window. A continuous, aperiodic source such as heat is present in every window, so it registers at full scale on every cycle, whatever the cycle is. Read through the weld, that full-scale registration was tagged as a high frequency, and the high frequency was then converted into a high energy. Heat appears near the top of the old ledger not because it carries a large amount per event but because it never stops arriving. The same holds for any radial, aperiodic source: it will show exactly as much energy as the sampler's cycle can hold, because there is no gap in it for the sampler to miss.

Removing the exponent takes the sampler's cycle out of the position. It does not correct the amount, because the amount was never separately read. Where the record does not say whether a patterned emission was detected or a continuous one, the amount entry is the saturation level of the instrument, not a property of the source. Twelve of the 36 entries carry a measured amount; several of those (the thermal entries in particular) were measured this way. The others were carried onto the amount side by conversion and have no amount reading at all.

The amount ledger therefore stands as indicative, not settled. Its values are upper bounds set by the instruments that took them, and any of them may be inflated by the original measurement. Separating the ledgers makes this visible; it does not fix it. Fixing it requires re-reading each amount with a sampler whose cycle is known and stated, and recording whether the source was continuous or patterned during the read.

## The amount ledger, from independent reads

The Mann on the amount side is declared from the same measurement Joule made: **1 Mann is the amount that raises 1 gram of water by 1 kelvin** (4.1855 J, the 15 °C calorie, NIST SP 811 §B.8). The count event that goes with it is the number of molecules in that gram, 3.34 × 10²², so amount, count and volume are tied to one substance at one condition, as gram, millilitre and cubic centimetre are.

Only entries with an amount read by an instrument that did not go through a frequency are placed. Values are in Mann per event; for molecular and particle reads the event is the water-gram count.

| item | class | Mann | event |
|---|---|---|---|
| 1 g water, 1 K (the unit) | unit | 1 | per gram-kelvin |
| Breathing, work at rest | bio | 0.12 | per breath (West, *Respiratory Physiology*) |
| Heart, stroke work | bio | 0.24 | per beat (Guyton and Hall) |
| kT, cosmic background 2.725 K | therm | 0.30 | per water-gram count |
| kT, human body 310 K | therm | 34 | per water-gram count |
| ATP hydrolysis | chem | 404 | per water-gram count |
| kT, solar photosphere 5772 K | therm | 637 | per water-gram count |
| Cs work function | chem | 2,740 | per water-gram count |
| H-H bond | chem | 5,780 | per water-gram count |
| H ionization | chem | 17,400 | per water-gram count |
| Electron rest energy | part | 6.5 × 10⁸ | per water-gram count |
| Muon rest energy | part | 1.4 × 10¹¹ | per water-gram count |
| Proton rest energy | part | 1.2 × 10¹² | per water-gram count |
| Z boson | part | 1.2 × 10¹⁴ | per water-gram count |
| Higgs boson | part | 1.6 × 10¹⁴ | per water-gram count |
| Top quark | part | 2.2 × 10¹⁴ | per water-gram count |
| GW150914, radiated | geo | 1.3 × 10⁴⁷ | per event; shown across a declared axis break (system-scale event; energy inferred from strain via a fitted model; possible scaling issue) |

Two further reads are held off the strip and belong on the re-reading list: the hearing threshold at 1 kHz, 1.3 × 10⁻²⁰ Mann per cycle at the eardrum (Fletcher and Munson 1933), and a cortical action potential at about 5 × 10⁻¹² Mann (4 × 10⁸ ATP; Attwell and Laughlin 2001). They are per-cycle and per-spike reads and sit eleven to twenty decades below the drive events they gate.

What the amount ledger shows:

- **The lower end is rational at once.** Heart and breath sit at a quarter and an eighth of the unit. Every chemical and thermal read sits between 0.3 and 2 × 10⁴. The body's mechanical events, its heat, its fuel and its bonds are within five decades of each other and of the unit, with no conversion.
- **The compression is real.** The frequency ledger spread the same reads over 30 decades; per water-gram count, chemistry and heat occupy five, and the particle masses six more. The mole is a factor of 18 off and needs no other change.
- **The two tiers of the body.** Drive events (heart, breath) at the unit; control events (spike, hearing threshold) at 10⁻¹² to 10⁻²⁰. The gain between them is what physiology measures, and the frequency ledger placed both tiers within three decades of each other because it was counting how often, not how much.
- **A low count can carry a huge amount.** GW150914 sat at 250 on the frequency ledger, the hearing floor's rung, and carries 10⁴⁷ Mann in one event.

Entries with no independent amount read (EEG bands, Schumann, every spectral line and clock transition, the top photon) are left off. They are unread, and the re-reading protocol in `RP_amount_ledger_rereading.md` is how they get on.

## The separation rule going forward

Frequency and power are now kept on two separate ledgers and are not converted into each other.

**Frequency** is how often. It is a property of sampling and emitting only. It tells you where something sits on the ledger. Frequencies relate by ratio (an octave is a doubling, whatever the starting note), so they are read on a log scale.

**Power** is how much. It is an amount spread across a continuous field. Amounts add: two sources together give the sum of both, and twice the volume at the same density gives twice the total. Amounts are read on a linear scale.

Any inherited formula that mixes the two is cut at the join. The frequency part goes to the frequency ledger, the amount part to the power ledger, and each keeps only the dimensions that belong to it. Any constant whose only job was to convert one into the other has nothing behind it once they are apart, and is dropped.

Of the 36 entries, 12 have a measured amount and 24 have a measured frequency. After the cut, each ledger has open slots where the other kind of measurement was never taken. Those slots are not errors. They are unread. Filling them by conversion is the error this procedure removes.

## Still open

- Twenty-four of the 36 entries have no measured amount and twelve have no measured frequency. Those slots stay empty until read.
- The twelve amounts that were read are instrument saturation levels until re-read against a stated sampler cycle. The whole amount side is up for revision.
- The ledger below 0.013 Mann is empty. The base was chosen expecting more to be found down there than up top.

## Files

- `scale_map.png`: the 36 values as physics plots them, joined by Planck's line.
- `scale_map_unit_sqrt.png`: one axis, after steps 2 and 3.
- `scale_map_times_h.png`: after step 4.
- `scale_map_sources.md`: measured quantity, value and citation for every entry, plus the exclusion list.
- `amount_ledger.png`, `amount_ledger.py`: the amount ledger from independent reads, with the declared break.
