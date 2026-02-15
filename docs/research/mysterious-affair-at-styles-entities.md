# Entity Catalog: The Mysterious Affair at Styles (Gutenberg #863)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 863 |
| Title | The Mysterious Affair at Styles |
| Author | Agatha Christie |
| Author birth | 1890-09-15 |
| Author death | 1976-01-12 |
| First published | 1920 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~56,000 |
| Structure | 13 chapters |

### Classification Signals

- **Content**: Detective novel (first Poirot)
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Hercule Poirot

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Hercule Poirot |
| Name tokens | `Hercule` (label, new) + `Poirot` (label) |
| Gender | male |
| Occupation | detective |
| Description | Belgian detective. Small, fastidious, with magnificent moustaches and an egg-shaped head. Uses his "little grey cells" to solve crimes. Shared across multiple Christie works. |
| Prominence | major (detective) |

#### Captain Hastings

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Captain Hastings |
| Name tokens | `hastings` |
| Gender | male |
| Description | Poirot's friend, Watson-like narrator. A somewhat dim but loyal companion. Convalescing at Styles when the murder occurs. |
| Prominence | major (narrator) |

#### Emily Inglethorp

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Emily Inglethorp |
| Name tokens | `Emily` (label, new) + `Inglethorp` (label, new) |
| Gender | female |
| Status | dead (murdered) |
| Description | Wealthy mistress of Styles Court. Poisoned with strychnine. Her recent marriage to Alfred Inglethorp and changes to her will provide motives. |
| Prominence | major (victim) |

#### Alfred Inglethorp

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Alfred Inglethorp |
| Name tokens | `Alfred` (label) + `Inglethorp` (label, new) |
| Gender | male |
| Description | Emily's much younger second husband. Black-bearded, fortune-hunter. Primary suspect but protected by Poirot's strategy. |
| Prominence | major |

#### John Cavendish

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | John Cavendish |
| Name tokens | `john` + `cavendish` |
| Gender | male |
| Description | Emily's stepson. Country squire type. Under suspicion for the murder. |
| Prominence | supporting |

#### Mary Cavendish

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mary Cavendish |
| Name tokens | `mary` + `cavendish` |
| Gender | female |
| Description | John's wife. Beautiful, mysterious, with a secret she is desperate to protect. |
| Prominence | supporting |

#### Lawrence Cavendish

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lawrence Cavendish |
| Name tokens | `Lawrence` (label) + `cavendish` |
| Gender | male |
| Description | Emily's younger stepson. Quiet, medical interests. In love with Cynthia. |
| Prominence | supporting |

#### Evelyn Howard

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Evelyn Howard |
| Name tokens | `Evelyn` (label) + `Howard` (label) |
| Gender | female |
| Description | Emily's companion. Blunt, mannish, fiercely loyal — or so it appears. Leaves Styles after quarreling with Emily about Alfred. |
| Prominence | supporting |

#### Cynthia Murdoch

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Cynthia Murdoch |
| Name tokens | `Cynthia` (label) |
| Gender | female |
| Occupation | dispensary worker |
| Description | Emily's protégée who works at the local hospital dispensary. Her access to poisons puts her under suspicion. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Agatha Christie

| Name tokens | `Agatha` (label) + `christie` |
| Gender | female |
| Born | 1890-09-15, Torquay, Devon |
| Died | 1976-01-12, Wallingford, Oxfordshire |
| Occupation | author |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VS | Inglethorp | Fictional surname, name-only |
| AB.AB.CA.HX.VT | Hercule | Fictional first name, name-only |
| AB.AB.CA.HX.VU | Emily | Common first name missing from shard |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Poirot | AB.AB.CA.HF.nQ | label |
| hastings | AB.AB.CA.CY.hR | noun |
| Alfred | AB.AB.CA.GR.Mf | label |
| john | AB.AB.CA.Cu.qL | noun |
| cavendish | AB.AB.CA.Aw.Dj | noun |
| mary | AB.AB.CA.DQ.km | noun |
| Lawrence | AB.AB.CA.Gu.UE | label |
| Evelyn | AB.AB.CA.Gg.Kp | label |
| Howard | AB.AB.CA.Gn.IB | label |
| Cynthia | AB.AB.CA.Gc.Sh | label |
| Agatha | AB.AB.CA.GQ.mV | label |
| christie | AB.AB.CA.BC.CN | noun |
