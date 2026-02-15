# Entity Catalog: The Sign of the Four (Gutenberg #2097)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 2097 |
| Title | The Sign of the Four |
| Author | Arthur Conan Doyle |
| Author birth | 1859-05-22 |
| Author death | 1930-07-07 |
| First published | 1890 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~43,000 |
| Structure | 12 chapters |

### Classification Signals

- **Content**: Detective novel
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Sherlock Holmes

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Sherlock Holmes |
| Name tokens | `sherlock` + `holmes` |
| Gender | male |
| Occupation | consulting detective |
| Description | The world's first and only consulting detective. Brilliant, eccentric, with extraordinary powers of observation and deduction. Shared across multiple Doyle works. |
| Prominence | major (protagonist) |

#### Dr. John Watson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Dr. Watson |
| Name tokens | `Watson` (label) |
| Gender | male |
| Occupation | physician |
| Description | Holmes's friend, flatmate, and chronicler. Former army surgeon. Falls in love with and marries Mary Morstan during this case. |
| Prominence | major (narrator) |

#### Mary Morstan

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mary Morstan |
| Name tokens | `mary` + `Morstan` (label, new) |
| Gender | female |
| Description | Client who brings the case to Holmes. Has been receiving pearls anonymously for six years. Watson falls in love with her. They marry. |
| Prominence | major |

#### Thaddeus Sholto

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Thaddeus Sholto |
| Name tokens | `Thaddeus` (label) + `Sholto` (label) |
| Gender | male |
| Description | Nervous, eccentric twin brother of Bartholomew. Contacts Mary Morstan to share the Agra treasure. |
| Prominence | supporting |

#### Bartholomew Sholto

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bartholomew Sholto |
| Name tokens | `Bartholomew` (label) + `Sholto` (label) |
| Gender | male |
| Status | dead (murdered) |
| Description | Twin brother of Thaddeus. Finds the Agra treasure but is murdered by Tonga before he can share it. |
| Prominence | supporting |

#### Jonathan Small

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jonathan Small |
| Name tokens | `small` |
| Gender | male |
| Description | One-legged ex-convict and one of the four signatories. Mastermind of the treasure theft who escapes the Andaman Islands to reclaim the Agra treasure. |
| Prominence | supporting |

#### Tonga

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tonga |
| Name tokens | `tonga` |
| Gender | male |
| Status | dead |
| Description | Small's Andaman Islander companion. Tiny, fierce, kills Bartholomew with a poisoned dart. Shot during the Thames chase. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

#### Arthur Conan Doyle

| Name tokens | `Arthur` (label) + `Conan` (label) + `Doyle` (label) |
| Gender | male |
| Born | 1859-05-22, Edinburgh |
| Died | 1930-07-07, Crowborough, East Sussex |
| Occupation | author, physician |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VD | Morstan | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| sherlock | AB.AB.CB.BH.EY | noun |
| holmes | AB.AB.CA.Ce.KA | noun |
| Watson | AB.AB.CA.HU.Xg | label |
| mary | AB.AB.CA.DQ.km | noun |
| Thaddeus | AB.AB.CA.HQ.bc | label |
| Sholto | AB.AB.CA.HL.it | label |
| Bartholomew | AB.AB.CA.GU.Kj | label |
| small | AB.AB.CA.FJ.AY | noun |
| tonga | AB.AB.CA.Fm.DS | noun |
| Arthur | AB.AB.CA.GS.hH | label |
| Conan | AB.AB.CA.Gb.KD | label |
| Doyle | AB.AB.CA.Ge.Qd | label |
| Athelney | AB.AB.CA.GS.xH | label |
| Jones | AB.AB.DA.AA.fb | label |
