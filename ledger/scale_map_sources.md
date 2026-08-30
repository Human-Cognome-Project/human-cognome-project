# Scaling map: values and sources

Constants: h, c, k_B, eV, N_A are exact SI (CODATA 2022 / SI 2019).
Conversion used throughout: E = hν. Wien frequency-form peak: ν_max = 2.821 k_B T / h.

| # | item | category | measured | ν [Hz] | E [J] | E [eV] | derivation | source |
|---|---|---|---|---|---|---|---|---|
| 1 | Resting breath (15/min) | bio | 0.25 Hz | 2.5000e-01 | 1.6565e-34 | 1.0339e-15 | Hz->J | Adult resting respiratory rate 12-18/min; Barrett et al., Ganong's Review of Medical Physiology, 26e |
| 2 | Resting heart (72 bpm) | bio | 1.2 Hz | 1.2000e+00 | 7.9513e-34 | 4.9628e-15 | Hz->J | Adult resting HR 60-100 bpm; AHA / Ganong 26e |
| 3 | EEG delta (0.5-4 Hz) midpt | bio | 2 Hz | 2.0000e+00 | 1.3252e-33 | 8.2713e-15 | Hz->J | Niedermeyer & da Silva, Electroencephalography, 6e |
| 4 | EEG theta (4-8 Hz) midpt | bio | 6 Hz | 6.0000e+00 | 3.9756e-33 | 2.4814e-14 | Hz->J | ibid. |
| 5 | Schumann fundamental | geo | 7.83 Hz | 7.8300e+00 | 5.1882e-33 | 3.2382e-14 | Hz->J | Balser & Wagner 1960, Nature 188:638 |
| 6 | EEG alpha (8-13 Hz) midpt | bio | 10.5 Hz | 1.0500e+01 | 6.9574e-33 | 4.3425e-14 | Hz->J | ibid. |
| 7 | Human hearing floor | bio | 20 Hz | 2.0000e+01 | 1.3252e-32 | 8.2713e-14 | Hz->J | ISO 226:2003 (equal-loudness contours, 20 Hz lower bound) |
| 8 | EEG beta (13-30 Hz) midpt | bio | 21.5 Hz | 2.1500e+01 | 1.4246e-32 | 8.8917e-14 | Hz->J | ibid. |
| 9 | EEG gamma (30-100 Hz) midpt | bio | 65 Hz | 6.5000e+01 | 4.3069e-32 | 2.6882e-13 | Hz->J | ibid. |
| 10 | GW150914 peak frequency | geo | 250 Hz | 2.5000e+02 | 1.6565e-31 | 1.0339e-12 | Hz->J | Abbott et al. 2016, PRL 116:061102 (35-250 Hz sweep) |
| 11 | Human hearing ceiling | bio | 20000 Hz | 2.0000e+04 | 1.3252e-29 | 8.2713e-11 | Hz->J | ISO 226:2003 / Rosen & Howell, Signals and Systems for Speech and Hearing |
| 12 | Lamb shift (H 2S-2P) | em | 1.05784e+09 Hz | 1.0578e+09 | 7.0094e-25 | 4.3749e-06 | Hz->J | Lundeen & Pipkin 1981, PRL 46:232; CODATA |
| 13 | H 21 cm hyperfine | em | 1.42041e+09 Hz | 1.4204e+09 | 9.4117e-25 | 5.8743e-06 | Hz->J | Essen et al. 1971, Nature 229:110 |
| 14 | Cs-133 hyperfine (SI second) | em | 9.19263e+09 Hz | 9.1926e+09 | 6.0911e-24 | 3.8018e-05 | Hz->J | BIPM SI Brochure 9e (2019), exact by definition of the second |
| 15 | kT, cosmic background 2.725 K | therm | 2.72548 K | 5.6790e+10 | 3.7629e-23 | 2.3486e-04 | T->both | Fixsen 2009, ApJ 707:916 (FIRAS temperature) |
| 16 | kT, human body 310 K | therm | 310.15 K | 6.4625e+12 | 4.2821e-21 | 2.6727e-02 | T->both | Core temperature 37.0 C; Ganong 26e |
| 17 | ATP hydrolysis (std) | chem | 30.5 kJ/mol | 7.6435e+13 | 5.0646e-20 | 3.1611e-01 | J->Hz | Lehninger Principles of Biochemistry 7e, dG'0 = -30.5 kJ/mol |
| 18 | kT, solar photosphere 5772 K | therm | 5772 K | 1.2027e+14 | 7.9691e-20 | 4.9739e-01 | T->both | IAU 2015 Resolution B3 nominal T_eff |
| 19 | Sr-87 optical clock | em | 4.29228e+14 Hz | 4.2923e+14 | 2.8441e-19 | 1.7751e+00 | Hz->J | Bloom et al. 2014, Nature 506:71 / CIPM 2021 recommended |
| 20 | H-alpha 656.28 nm | em | 656.281 nm | 4.5681e+14 | 3.0268e-19 | 1.8892e+00 | nm->Hz->J | NIST ASD, Balmer alpha (air wavelength) |
| 21 | Na D2 589.0 nm | em | 588.995 nm | 5.0899e+14 | 3.3726e-19 | 2.1050e+00 | nm->Hz->J | NIST ASD |
| 22 | Cs work function | chem | 2.14 eV | 5.1745e+14 | 3.4287e-19 | 2.1400e+00 | J->Hz | CRC Handbook 97e, Table: Electron Work Function of the Elements |
| 23 | H-H bond (436 kJ/mol) | chem | 436 kJ/mol | 1.0926e+15 | 7.2400e-19 | 4.5188e+00 | J->Hz | CRC Handbook 97e, Bond Dissociation Energies |
| 24 | Th-229 nuclear clock isomer | em | 8.35573 eV | 2.0204e+15 | 1.3387e-18 | 8.3557e+00 | J->Hz | Tiedau et al. 2024, PRL 132:182501 |
| 25 | H 1S-2S two-photon | em | 2.46606e+15 Hz | 2.4661e+15 | 1.6340e-18 | 1.0199e+01 | Hz->J | Parthey et al. 2011, PRL 107:203001 |
| 26 | Lyman-alpha 121.57 nm | em | 121.567 nm | 2.4661e+15 | 1.6340e-18 | 1.0199e+01 | nm->Hz->J | NIST ASD |
| 27 | H ionization 13.598 eV | chem | 13.5984 eV | 3.2881e+15 | 2.1787e-18 | 1.3598e+01 | J->Hz | NIST ASD, H I ionization energy |
| 28 | Cu K-alpha X-ray | em | 8047.8 eV | 1.9459e+18 | 1.2894e-15 | 8.0478e+03 | J->Hz | Deslattes et al. 2003, Rev Mod Phys 75:35 |
| 29 | Electron rest energy | part | 510999 eV | 1.2356e+20 | 8.1871e-14 | 5.1100e+05 | J->Hz | CODATA 2022 |
| 30 | Co-60 gamma 1.332 MeV | em | 1.3325e+06 eV | 3.2220e+20 | 2.1349e-13 | 1.3325e+06 | J->Hz | NNDC / Helmer & van der Leun 2000 |
| 31 | Muon rest energy | part | 1.05658e+08 eV | 2.5548e+22 | 1.6928e-11 | 1.0566e+08 | J->Hz | CODATA 2022 |
| 32 | Proton rest energy | part | 9.38272e+08 eV | 2.2687e+23 | 1.5033e-10 | 9.3827e+08 | J->Hz | CODATA 2022 |
| 33 | Z boson | part | 9.1188e+10 eV | 2.2049e+25 | 1.4610e-08 | 9.1188e+10 | J->Hz | PDG 2024 |
| 34 | Higgs boson | part | 1.252e+11 eV | 3.0273e+25 | 2.0059e-08 | 1.2520e+11 | J->Hz | PDG 2024 |
| 35 | Top quark | part | 1.7257e+11 eV | 4.1727e+25 | 2.7649e-08 | 1.7257e+11 | J->Hz | PDG 2024 |
| 36 | LHAASO 1.4 PeV photon | em | 1.4e+15 eV | 3.3852e+29 | 2.2430e-04 | 1.4000e+15 | J->Hz | Cao et al. 2021, Nature 594:33 |

## Excluded as not measured
EM band edges (ITU conventions), mains 50/60 Hz and CPU clocks (assigned), A440 (ISO 16 convention), Hubble rate, age of universe, Planck energy, solar core temperature (model).
