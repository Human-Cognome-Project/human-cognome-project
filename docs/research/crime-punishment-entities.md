# Entity Catalog: Crime and Punishment (Gutenberg #2554)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AG

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 2554 |
| Title | Crime and Punishment |
| Author | Fyodor Dostoyevsky |
| Author birth | 1821-11-11 |
| Author death | 1881-02-09 |
| First published | 1866 |
| Language | English (en) — translation |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~50,000 |
| Classification | Fiction |
| Word count | ~211,000 |
| Structure | 6 parts + Epilogue |

### Gutenberg Subject Headings (LCSH)

- Crime -- Psychological aspects -- Fiction
- Detective and mystery stories
- Murder -- Fiction
- Psychological fiction
- Saint Petersburg (Russia) -- Fiction

### Gutenberg Bookshelves

- Best Books Ever Listings
- Category: Classics of Literature
- Category: Crime, Thrillers and Mystery
- Category: Novels
- Category: Russian Literature
- Crime Fiction
- Harvard Classics

### Classification Signals

- **Bookshelves**: "Category: Novels", "Crime Fiction", "Category: Russian Literature" → fiction
- **Subjects**: "Psychological fiction", "Murder -- Fiction" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Rodion Romanovich Raskolnikov

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Rodion Raskolnikov |
| Name tokens | `Rodion` (label, new) + `Raskolnikov` (label, new) |
| Full name | Rodion Romanovich Raskolnikov |
| Aliases | Rodya, Rodenka |
| Gender | male |
| Occupation | former student |
| Description | Impoverished former law student in St. Petersburg who murders a pawnbroker and her sister. Tormented by guilt and paranoia. Eventually confesses and is sentenced to Siberian exile. The protagonist. |
| Prominence | major (protagonist) |

**Key relationships:**
- `sibling_of` → Avdotya (Dunya) Raskolnikova
- `child_of` → Pulcheria Alexandrovna
- `friend_of` → Razumikhin
- `loves` → Sonya Marmeladova
- `killed` → Alyona Ivanovna (the pawnbroker)
- `investigated_by` → Porfiry Petrovich

---

#### Sonya Marmeladova

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Sonya Marmeladova |
| Name tokens | `Sonya` (label) + `Marmeladov` (label, new) |
| Full name | Sofya Semyonovna Marmeladova |
| Aliases | Sonya, Sonechka |
| Gender | female |
| Occupation | prostitute (forced by poverty) |
| Description | Gentle, devout young woman forced into prostitution to support her family. Becomes Raskolnikov's moral conscience. Follows him to Siberia. Reads him the story of Lazarus. |
| Prominence | major |

---

#### Porfiry Petrovich

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Porfiry Petrovich |
| Name tokens | `Porfiry` (label, new) + `Petrovich` (label) |
| Gender | male |
| Occupation | police investigator |
| Description | Brilliant, psychologically astute detective who suspects Raskolnikov. Uses cat-and-mouse tactics rather than direct evidence. Eventually persuades Raskolnikov to confess. |
| Prominence | major |

---

#### Arkady Ivanovich Svidrigailov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Arkady Svidrigailov |
| Name tokens | `Arkady` (label, new) + `Svidrigailov` (label, new) |
| Gender | male |
| Status | dead (suicide) |
| Description | Dissolute former employer of Dunya. Wealthy, morally ambiguous. Both generous and predatory. Commits suicide after Dunya definitively rejects him. |
| Prominence | major |

---

### 2.2 Supporting Characters

#### Avdotya Romanovna Raskolnikova (Dunya)

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Avdotya Raskolnikova |
| Name tokens | `Avdotya` (label, new) + `Raskolnikov` (label, new) |
| Aliases | Dunya, Dunechka |
| Gender | female |
| Description | Raskolnikov's proud, beautiful sister. Former governess for Svidrigailov. Nearly marries Luzhin before choosing Razumikhin. |
| Prominence | supporting |

---

#### Dmitri Prokofych Razumikhin

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Razumikhin |
| Name tokens | `Razumikhin` (label, new) |
| Gender | male |
| Occupation | student |
| Description | Raskolnikov's loyal, good-natured friend and fellow student. Eventually marries Dunya. |
| Prominence | supporting |

---

#### Semyon Zakharovich Marmeladov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Semyon Marmeladov |
| Name tokens | `Semyon` (label) + `Marmeladov` (label, new) |
| Gender | male |
| Status | dead (run over by carriage) |
| Occupation | former civil servant |
| Description | Sonya's alcoholic father. His tavern confession to Raskolnikov opens the novel. Dies after being run over. |
| Prominence | supporting |

---

#### Katerina Ivanovna Marmeladova

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Katerina Ivanovna |
| Name tokens | `Katerina` (label) + `Ivanovna` (label) |
| Gender | female |
| Status | dead (consumption) |
| Description | Marmeladov's consumptive, proud second wife. Of noble birth, now destitute. Drives Sonya to prostitution. Goes mad and dies after Marmeladov's death. |
| Prominence | supporting |

---

#### Pyotr Petrovich Luzhin

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Luzhin |
| Name tokens | `Luzhin` (label, new) |
| Gender | male |
| Occupation | lawyer, civil counselor |
| Description | Dunya's pompous, calculating fiancé. Wants a wife who will be grateful and submissive. Tries to frame Sonya as a thief. Rejected by Dunya. |
| Prominence | supporting |

---

#### Pulcheria Alexandrovna Raskolnikova

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Pulcheria Alexandrovna |
| Name tokens | `Pulcheria` (label) + `Alexandrovna` (label, new) |
| Gender | female |
| Description | Raskolnikov and Dunya's devoted mother. Her letter about Dunya's engagement triggers Raskolnikov's crisis. |
| Prominence | minor |

---

#### Andrei Semyonovich Lebezyatnikov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lebezyatnikov |
| Name tokens | `Lebezyatnikov` (label, new) |
| Gender | male |
| Description | Young radical progressive. Luzhin's roommate. Exposes Luzhin's attempt to frame Sonya. |
| Prominence | minor |

---

#### Nastasya

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Nastasya |
| Name tokens | `Nastasya` (label, new) |
| Gender | female |
| Occupation | servant |
| Description | Servant at Raskolnikov's boarding house. Brings him food during his illness. |
| Prominence | minor |

---

#### Zametov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Zametov |
| Name tokens | `Zametov` (label, new) |
| Gender | male |
| Occupation | police clerk |
| Description | Young police clerk. Raskolnikov nearly confesses to him in a tavern. |
| Prominence | minor |

---

#### Zossimov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Zossimov |
| Name tokens | `Zossimov` (label, new) |
| Gender | male |
| Occupation | doctor |
| Description | Young doctor and friend of Razumikhin. Treats Raskolnikov during his illness. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### Fyodor Dostoyevsky

| Name tokens | `Fyodor` (label) + `Dostoyevsky` (label) |
| Gender | male |
| Born | 1821-11-11, Moscow |
| Died | 1881-02-09, St. Petersburg |
| Occupation | novelist, philosopher |

---

## 4. Non-Fiction Place Entities (x*)

St. Petersburg is the primary setting. Already exists from Frankenstein entities as "st_petersburgh" (xA.AA.AA.AA.AF).

---

## 5. Label Token Audit

### 5.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.UP | Raskolnikov | Russian surname, name-only |
| AB.AB.CA.HX.UQ | Marmeladov | Russian surname, name-only |
| AB.AB.CA.HX.UR | Razumikhin | Russian surname, name-only |
| AB.AB.CA.HX.US | Svidrigailov | Russian surname, name-only |
| AB.AB.CA.HX.UT | Porfiry | Russian given name, name-only |
| AB.AB.CA.HX.UU | Luzhin | Russian surname, name-only |
| AB.AB.CA.HX.UV | Lebezyatnikov | Russian surname, name-only |
| AB.AB.CA.HX.UW | Rodion | Russian given name, name-only |
| AB.AB.CA.HX.UX | Romanovich | Russian patronymic, name-only |
| AB.AB.CA.HX.UY | Zakharovich | Russian patronymic, name-only |
| AB.AB.CA.HX.UZ | Avdotya | Russian given name, name-only |
| AB.AB.CA.HX.Ua | Arkady | Russian given name, name-only |
| AB.AB.CA.HX.Ub | Alexandrovna | Russian patronymic, name-only |
| AB.AB.CA.HX.Uc | Nastasya | Russian given name, name-only |
| AB.AB.CA.HX.Ud | Zametov | Russian surname, name-only |
| AB.AB.CA.HX.Ue | Zossimov | Russian surname, name-only |

### 5.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Sonya | AB.AB.CA.HM.nU | label |
| Sofya | AB.AB.CA.HM.hA | label |
| Dunya | AB.AB.CA.Ge.mD | label |
| Katerina | AB.AB.CA.Gr.EK | label |
| Ivanovna | AB.AB.CA.Gp.YL | label |
| Petrovich | AB.AB.CA.HF.BJ | label |
| Semyon | AB.AB.CA.HK.wn | label |
| Pulcheria | AB.AB.CA.HG.dQ | label |
| Nikolai | AB.AB.CA.HC.Gx | label |
| Fyodor | AB.AB.CA.Gi.Bb | label |
| Dostoyevsky | AB.AB.CA.Ge.MA | label |
