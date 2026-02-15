# Entity Catalog: A Christmas Carol (Gutenberg #46)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 46 |
| Title | A Christmas Carol in Prose; Being a Ghost Story of Christmas |
| Author | Charles Dickens |
| Author birth | 1812-02-07 |
| Author death | 1870-06-09 |
| First published | 1843 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~29,000 |
| Structure | 5 staves |

### Classification Signals

- **Content**: Christmas novella, ghost story
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Ebenezer Scrooge

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Scrooge |
| Name tokens | `scrooge` |
| Full name tokens | `Ebenezer` (label) + `scrooge` |
| Gender | male |
| Occupation | money-lender |
| Description | Miserly, cold-hearted old businessman who is visited by three Christmas spirits and transforms into a generous, kind man. |
| Prominence | major (protagonist) |

#### Bob Cratchit

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bob Cratchit |
| Name tokens | `bob` + `Cratchit` (label, new) |
| Gender | male |
| Occupation | clerk |
| Description | Scrooge's overworked, underpaid clerk. Devoted father of Tiny Tim and several other children. |
| Prominence | supporting |

#### Jacob Marley

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Marley |
| Name tokens | `Marley` (label) |
| Full name tokens | `Jacob` (label) + `Marley` (label) |
| Gender | male |
| Status | dead (ghost) |
| Description | Scrooge's deceased business partner. Appears as a chain-laden ghost to warn Scrooge. |
| Prominence | supporting |

#### Tiny Tim

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tiny Tim |
| Name tokens | `Tim` (label) |
| Gender | male |
| Description | Bob Cratchit's disabled youngest son. His potential death is shown by the Ghost of Christmas Yet to Come. Survives thanks to Scrooge's reform. |
| Prominence | supporting |

#### Mr. Fezziwig

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Fezziwig |
| Name tokens | `Fezziwig` (label, new) |
| Gender | male |
| Occupation | merchant |
| Description | Scrooge's kindly former employer. Shown in the Ghost of Christmas Past's vision as a contrast to Scrooge. |
| Prominence | minor |

#### Belle

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Belle |
| Name tokens | `belle` |
| Gender | female |
| Description | Scrooge's former fiancée who broke off their engagement because of his growing avarice. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

#### Charles Dickens

| Name tokens | `Charles` (label) + `dickens` |
| Gender | male |
| Born | 1812-02-07, Portsmouth |
| Died | 1870-06-09, Higham, Kent |
| Occupation | novelist, social critic |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Ut | Cratchit | Fictional surname, name-only |
| AB.AB.CA.HX.Uu | Fezziwig | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Ebenezer | AB.AB.CA.Gf.BT | label |
| Marley | AB.AB.CA.Gx.bT | label |
| Jacob | AB.AB.CA.Gp.cv | label |
| Tim | AB.AB.CA.HQ.xd | label |
| Charles | AB.AB.CA.GZ.nI | label |
| scrooge | AB.AB.CA.Ex.wQ | noun |
| bob | AB.AB.CA.Ah.iv | noun |
| belle | AB.AB.CA.Ab.Ma | noun |
| dickens | AB.AB.CA.Bc.VS | noun |
