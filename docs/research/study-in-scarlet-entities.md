# Entity Catalog: A Study in Scarlet (Gutenberg #244)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 244 |
| Title | A Study in Scarlet |
| Author | Arthur Conan Doyle |
| Author birth | 1859-05-22 |
| Author death | 1930-07-07 |
| First published | 1887 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~43,000 |
| Structure | 2 parts (7 + 7 chapters) |

### Classification Signals

- **Content**: Detective novel (first Sherlock Holmes)
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

Note: Holmes (uA.AA.AA.BA.AA) and Watson (uA.AA.AA.BA.AB) created under Sign of the Four — shared entities across Doyle's Holmes canon.

#### Jefferson Hope

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jefferson Hope |
| Name tokens | `Jefferson` (label) + `hope` |
| Gender | male |
| Status | dead |
| Description | American frontiersman driven by vengeance for the deaths of John and Lucy Ferrier. Murders Drebber and Stangerson. Dies of an aortic aneurysm in custody. |
| Prominence | major |

#### Enoch Drebber

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Enoch Drebber |
| Name tokens | `enoch` + `Drebber` (label, new) |
| Gender | male |
| Status | dead (murdered) |
| Description | Former Mormon elder who forced Lucy Ferrier into marriage. Found murdered in an empty house. |
| Prominence | supporting |

#### Joseph Stangerson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Joseph Stangerson |
| Name tokens | `Stangerson` (label, new) |
| Gender | male |
| Status | dead (murdered) |
| Description | Drebber's companion and fellow Mormon elder. Murdered by Hope at a hotel. |
| Prominence | supporting |

#### Inspector Lestrade

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lestrade |
| Name tokens | `Lestrade` (label, new) |
| Gender | male |
| Occupation | Scotland Yard inspector |
| Description | A Scotland Yard detective. Ferret-like, sallow. One of Holmes's regular police contacts across multiple stories. |
| Prominence | supporting |

#### Lucy Ferrier

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lucy Ferrier |
| Name tokens | `lucy` + `ferrier` |
| Gender | female |
| Status | dead |
| Description | Beautiful young woman raised by John Ferrier among the Mormons. Forced to marry Drebber. Dies of a broken heart. Her death motivates Hope's vengeance. |
| Prominence | supporting |

#### John Ferrier

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | John Ferrier |
| Name tokens | `john` + `ferrier` |
| Gender | male |
| Status | dead |
| Description | An American pioneer rescued by Mormons in the desert. Raises Lucy. Murdered trying to escape with her from the Mormon settlement. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Arthur Conan Doyle

Reused from Sign of the Four catalog (yA.AA.AA.AA.Bf).

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VG | Lestrade | Fictional surname, name-only |
| AB.AB.CA.HX.VH | Drebber | Fictional surname, name-only |
| AB.AB.CA.HX.VI | Stangerson | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Jefferson | AB.AB.CA.Gp.tw | label |
| hope | AB.AB.CA.Cf.Wm | noun |
| enoch | AB.AB.CA.Br.QR | noun |
| lucy | AB.AB.CA.DK.TP | noun |
| ferrier | AB.AB.CA.Bz.jn | noun |
| john | AB.AB.CA.Cu.qL | noun |
| Gregson | AB.AB.CA.Gk.Aa | label |
| Stamford | AB.AB.CA.HN.Ma | label |
