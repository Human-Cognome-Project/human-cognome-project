# Entity Catalog: Jane Eyre: An Autobiography (Gutenberg #1260)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AE

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 1260 |
| Title | Jane Eyre: An Autobiography |
| Author | Charlotte Bronte |
| Author birth | 1816-04-21 |
| Author death | 1855-03-31 |
| First published | 1847 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~45,000 |
| Classification | Fiction |
| Word count | ~188,000 |
| Structure | 38 chapters |

### Gutenberg Subject Headings (LCSH)

- Bildungsromans
- Charity-schools -- Fiction
- Country homes -- Fiction
- England -- Fiction
- Fathers and daughters -- Fiction
- Governesses -- Fiction
- Love stories
- Married people -- Fiction
- Mentally ill women -- Fiction
- Orphans -- Fiction
- Young women -- Fiction

### Gutenberg Bookshelves

- Category: British Literature
- Category: Classics of Literature
- Category: Novels
- Category: Romance

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: Romance" → fiction
- **Subjects**: "Love stories", "Bildungsromans" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Jane Eyre

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Jane Eyre |
| Name tokens | `jane` + `eyre` |
| Gender | female |
| Occupation | governess, teacher |
| Description | Orphan raised by her cruel aunt Reed at Gateshead, then educated at Lowood school. Becomes governess at Thornfield Hall, falls in love with Rochester, discovers his secret, and eventually marries him. The narrator. |
| Prominence | major (protagonist/narrator) |

**Key relationships:**
- `spouse_of` → Edward Rochester
- `governess_of` → Adele Varens
- `friend_of` → Helen Burns
- `niece_of` → Mrs. Reed
- `cousin_of` → St. John Rivers, Diana Rivers

---

#### Edward Rochester

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Edward Rochester |
| Name tokens | `Edward` (label) + `Rochester` (label) |
| Gender | male |
| Occupation | gentleman, landowner |
| Description | Master of Thornfield Hall. Dark, brooding, passionate. Secretly married to Bertha Mason. Falls in love with Jane. Blinded and maimed in the fire that destroys Thornfield. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Jane Eyre (eventual)
- `spouse_of` → Bertha Mason (first marriage)
- `guardian_of` → Adele Varens
- `employer_of` → Mrs. Fairfax

---

### 2.2 Supporting Characters

#### St. John Rivers

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | St. John Rivers |
| Name tokens | `St` + `john` + `rivers` |
| Gender | male |
| Occupation | clergyman, missionary |
| Description | Jane's cousin. Handsome, cold, intensely pious. Proposes a loveless marriage to Jane to serve as his missionary companion in India. |
| Prominence | supporting |

---

#### Bertha Mason

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bertha Mason |
| Name tokens | `bertha` + `mason` |
| Gender | female |
| Status | dead (dies in fire) |
| Description | Rochester's first wife, locked in the attic of Thornfield Hall. From a Jamaican Creole family. Her violent madness is the novel's central secret. Sets fire to Thornfield. |
| Prominence | supporting |

---

#### Mrs. Reed

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mrs. Reed |
| Name tokens | `reed` |
| Gender | female |
| Status | dead |
| Description | Jane's aunt by marriage. Cruel and cold. Sends Jane to Lowood. On her deathbed confesses to withholding news of Jane's uncle. |
| Prominence | supporting |

---

#### Helen Burns

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Helen Burns |
| Name tokens | `helen` + `burns` |
| Gender | female |
| Status | dead (dies of consumption at Lowood) |
| Description | Jane's first real friend at Lowood school. Patient, spiritual, and philosophical. Her death profoundly affects Jane. |
| Prominence | supporting |

---

#### Adele Varens

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Adele Varens |
| Name tokens | `adele` + `Varens` (label, new) |
| Gender | female |
| Description | Jane's pupil at Thornfield. French child, possibly Rochester's daughter. Lively and fond of pretty things. |
| Prominence | supporting |

---

#### Mrs. Fairfax

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mrs. Fairfax |
| Name tokens | `Fairfax` (label) |
| Gender | female |
| Occupation | housekeeper |
| Description | Elderly housekeeper at Thornfield Hall. Kind and grandmotherly. Rochester's distant relative. |
| Prominence | supporting |

---

#### Diana Rivers

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Diana Rivers |
| Name tokens | `Diana` (label) + `rivers` |
| Gender | female |
| Description | Jane's cousin. Intelligent and warm. Works as a governess. |
| Prominence | minor |

---

#### Mr. Brocklehurst

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Brocklehurst |
| Name tokens | `Brocklehurst` (label) |
| Gender | male |
| Occupation | clergyman, school superintendent |
| Description | Hypocritical treasurer of Lowood school. Cruel and parsimonious with the students while indulging his own family. |
| Prominence | minor |

---

#### Miss Temple

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Miss Temple |
| Name tokens | `temple` |
| Gender | female |
| Occupation | school superintendent |
| Description | Kind superintendent at Lowood. A nurturing influence on Jane. |
| Prominence | minor |

---

#### Blanche Ingram

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Blanche Ingram |
| Name tokens | `Blanche` (label) + `Ingram` (label) |
| Gender | female |
| Description | Beautiful, haughty socialite who pursues Rochester for his wealth. |
| Prominence | minor |

---

#### Bessie

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bessie |
| Name tokens | `Bessie` (label) |
| Gender | female |
| Occupation | nursemaid |
| Description | Nursemaid at Gateshead who shows Jane intermittent kindness. |
| Prominence | minor |

---

## 3. Fiction Place Entities (t*)

#### Thornfield Hall

| Name tokens | `Thornfield` (label, new) |
| Description | Rochester's country estate. Burns down in the fire set by Bertha. |

#### Lowood School

| Name tokens | `Lowood` (label, new) |
| Description | Charity school where Jane is educated. Harsh conditions; typhus epidemic. |

#### Gateshead Hall

| Name tokens | `Gateshead` (label, new) |
| Description | The Reed family home where Jane spends her miserable childhood. |

#### Ferndean Manor

| Name tokens | `Ferndean` (label, new) |
| Description | Rochester's retreat after Thornfield burns. Where Jane and Rochester reunite. |

---

## 4. Non-Fiction People Entities (y*)

### 4.1 Author

#### Charlotte Bronte

| Name tokens | `charlotte` + `Bronte` (label) |
| Gender | female |
| Born | 1816-04-21, Thornton, Yorkshire |
| Died | 1855-03-31, Haworth, Yorkshire |
| Occupation | novelist, poet, governess |

---

## 5. Label Token Audit

### 5.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.UJ | Varens | Fictional French surname |
| AB.AB.CA.HX.UK | Thornfield | Fictional estate name |
| AB.AB.CA.HX.UL | Lowood | Fictional school name |
| AB.AB.CA.HX.UM | Gateshead | Estate name (also real place but not in DB) |
| AB.AB.CA.HX.UN | Ferndean | Fictional manor name |

### 5.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Rochester | AB.AB.CA.HI.VV | label |
| Brocklehurst | AB.AB.CA.GX.Nl | label |
| Fairfax | AB.AB.CA.Gg.Tr | label |
| Ingram | AB.AB.CA.Gp.Dn | label |
| Blanche | AB.AB.CA.GV.tc | label |
| Bessie | AB.AB.CA.GV.Mu | label |
| Diana | AB.AB.CA.Gd.dl | label |
| Edward | AB.AB.CA.Gf.KG | label |
| Bronte | AB.AB.CA.GX.Rm | label |
| jane | AB.AB.CA.Ct.Tz | noun |
| eyre | AB.AB.CA.Bw.qp | noun |
| helen | AB.AB.CA.Ca.HD | noun |
| burns | AB.AB.CA.Ap.WJ | noun |
| bertha | AB.AB.CA.Ac.IJ | noun |
| mason | AB.AB.CA.DQ.tl | noun |
| reed | AB.AB.CA.Ej.Jj | noun |
| rivers | AB.AB.CA.Ep.cs | noun |
| temple | AB.AB.CA.Ff.jL | noun |
| adele | AB.AB.CA.AD.WK | noun |
| john | AB.AB.CA.Cu.qL | noun |
| St | AB.AB.CA.FR.GW | noun |
| charlotte | AB.AB.CA.Ay.ey | noun |
