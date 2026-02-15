# Entity Catalog: Cranford (Gutenberg #394)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 394 |
| Title | Cranford |
| Author | Elizabeth Gaskell |
| Author birth | 1810-09-29 |
| Author death | 1865-11-12 |
| First published | 1853 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~70,000 |
| Structure | 16 chapters |

### Classification Signals

- **Content**: Social comedy / Victorian novel
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Miss Matty (Matilda Jenkyns)

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Miss Matty |
| Name tokens | `matty` |
| Alternate name | Matilda Jenkyns |
| Gender | female |
| Description | Gentle, timid, kind-hearted elderly spinster. Central character of Cranford. Loses her savings when the bank fails but is supported by the community. |
| Prominence | major (protagonist) |

#### Miss Deborah Jenkyns

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Deborah Jenkyns |
| Name tokens | `Deborah` (label, new) + `Jenkyns` (label) |
| Gender | female |
| Status | dead |
| Description | Matty's formidable elder sister. Sets the social standards for Cranford. Dies early in the novel. |
| Prominence | supporting |

#### Peter Jenkyns

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Peter Jenkyns |
| Name tokens | `peter` + `Jenkyns` (label) |
| Gender | male |
| Description | Matty's long-lost brother who returns from India at the end to save her from poverty. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Elizabeth Gaskell

| Name tokens | `Elizabeth` (label) + `Gaskell` (label) |
| Gender | female |
| Born | 1810-09-29, London |
| Died | 1865-11-12, Holybourne, Hampshire |
| Occupation | novelist |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VZ | Deborah | First name missing from shard |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| matty | AB.AB.CC.Ab.Ka | adj |
| Jenkyns | AB.AB.CA.Gp.vt | label |
| peter | AB.AB.CA.EI.dT | noun |
| Elizabeth | AB.AB.CA.Gf.XT | label |
| Gaskell | AB.AB.CA.Gi.Ye | label |
| Cranford | AB.AB.CA.Gb.pF | label |
